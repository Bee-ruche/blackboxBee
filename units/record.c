enum {
    RECORD_MAX_FIELDS = 12,
    RECORD_MAX_KEY = 8,
    RECORD_MAX_STRING = 32
};

struct record_field {
    const u8 *key;
    usize key_len;
    u8 tag;
    u32 number;
    u8 string[RECORD_MAX_STRING];
    usize string_len;
};

void *memcpy(void *dest, const void *src, usize n);
int memcmp(const void *a, const void *b, usize n);

static const u8 *skip_spaces(const u8 *p, const u8 *end)
{
    while (p != end && *p == ' ')
        ++p;
    return p;
}

static int key_byte(u8 ch)
{
    return (ch >= 'a' && ch <= 'z') || ch == '_';
}

static int decimal_digit(u8 ch)
{
    return ch >= '0' && ch <= '9';
}

static const u8 *read_string(const u8 *p, const u8 *end,
                             struct record_field *field)
{
    field->tag = 's';
    field->string_len = 0;
    while (p != end) {
        u8 ch = *p++;

        if (ch == '"')
            return p;
        if (ch < ' ' || field->string_len == RECORD_MAX_STRING)
            return 0;
        if (ch == '\\') {
            if (p == end)
                return 0;
            ch = *p++;
            switch (ch) {
            case 'n': ch = '\n'; break;
            case 't': ch = '\t'; break;
            case '"':
            case '\\': break;
            default: return 0;
            }
        }
        field->string[field->string_len++] = ch;
    }
    return 0;
}

static const u8 *read_integer(const u8 *p, const u8 *end,
                              struct record_field *field)
{
    int negative = *p == '-';
    u64 magnitude = 0;
    usize digits = 0;

    p += negative;
    if (p == end || !decimal_digit(*p))
        return 0;
    if (*p == '0' && p + 1 != end && decimal_digit(p[1]))
        return 0;

    do {
        if (++digits > 10)
            return 0;
        magnitude = magnitude * 10 + (*p++ - '0');
    } while (p != end && decimal_digit(*p));

    if (magnitude > (negative ? 2147483648u : 2147483647u))
        return 0;
    field->tag = 'i';
    field->number = negative ? 0u - (u32)magnitude : (u32)magnitude;
    return p;
}

static const u8 *read_value(const u8 *p, const u8 *end,
                            struct record_field *field)
{
    if (p == end)
        return 0;
    if (*p == '"')
        return read_string(p + 1, end, field);
    if (*p == '-' || decimal_digit(*p))
        return read_integer(p, end, field);

    field->tag = 'b';
    if (end - p >= 4 && memcmp(p, "true", 4) == 0) {
        field->number = 1;
        return p + 4;
    }
    if (end - p >= 5 && memcmp(p, "false", 5) == 0) {
        field->number = 0;
        return p + 5;
    }
    return 0;
}

static int compare_keys(const struct record_field *a,
                        const struct record_field *b)
{
    usize common = a->key_len < b->key_len ? a->key_len : b->key_len;
    int order = memcmp(a->key, b->key, common);

    if (order != 0)
        return order;
    return (int)a->key_len - (int)b->key_len;
}

int bee_answer(const u8 *in, usize n, u8 *out, usize *len)
{
    struct record_field fields[RECORD_MAX_FIELDS];
    const struct record_field *ordered[RECORD_MAX_FIELDS];
    const u8 *end = in + n;
    const u8 *p = skip_spaces(in, end);
    usize count = 0;

    while (p != end) {
        if (count == RECORD_MAX_FIELDS)
            return 1;
        struct record_field *field = &fields[count];
        field->key = p;
        while (p != end && key_byte(*p))
            ++p;
        field->key_len = (usize)(p - field->key);
        if (field->key_len == 0 || field->key_len > RECORD_MAX_KEY)
            return 1;

        p = skip_spaces(p, end);
        if (p == end || *p != '=')
            return 1;
        p = read_value(skip_spaces(p + 1, end), end, field);
        if (!p)
            return 1;

        /* Keep keys in byte order and reject any repeated key. */
        usize i = count;
        while (i > 0) {
            int order = compare_keys(ordered[i - 1], field);
            if (order == 0)
                return 1;
            if (order < 0)
                break;
            ordered[i] = ordered[i - 1];
            --i;
        }
        ordered[i] = field;
        ++count;

        p = skip_spaces(p, end);
        if (p == end)
            break;
        if (*p != ';')
            return 1;
        p = skip_spaces(p + 1, end);
        if (p == end)
            return 1;
    }

    u8 *answer = out;
    *answer++ = (u8)count;
    for (usize i = 0; i < count; ++i) {
        const struct record_field *field = ordered[i];

        *answer++ = (u8)field->key_len;
        memcpy(answer, field->key, field->key_len);
        answer += field->key_len;
        *answer++ = field->tag;
        switch (field->tag) {
        case 's':
            *answer++ = (u8)field->string_len;
            memcpy(answer, field->string, field->string_len);
            answer += field->string_len;
            break;
        case 'i':
            *answer++ = (u8)(field->number >> 24);
            *answer++ = (u8)(field->number >> 16);
            *answer++ = (u8)(field->number >> 8);
            *answer++ = (u8)field->number;
            break;
        case 'b':
            *answer++ = (u8)field->number;
            break;
        }
    }
    *len = (usize)(answer - out);
    return 0;
}
