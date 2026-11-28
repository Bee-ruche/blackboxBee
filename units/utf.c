void *memmove(void *dest, const void *src, usize n);

static u32 read_codepoint(const u8 **cursor, const u8 *end)
{
    const u8 *p = *cursor;
    u32 first = *p++;
    if (first < 0x80) {
        *cursor = p;
        return first;
    }

    usize extra;
    u32 value, minimum;
    if (first >= 0xc0 && first <= 0xdf) {
        extra = 1;
        value = first & 0x1f;
        minimum = 0x80;
    } else if (first >= 0xe0 && first <= 0xef) {
        extra = 2;
        value = first & 0x0f;
        minimum = 0x800;
    } else if (first >= 0xf0 && first <= 0xf7) {
        extra = 3;
        value = first & 7u;
        minimum = 0x10000;
    } else {
        return 0;
    }
    if ((usize)(end - p) < extra)
        return 0;
    for (usize i = 0; i < extra; ++i) {
        u8 byte = *p++;
        if ((byte & 0xc0) != 0x80)
            return 0;
        value = (value << 6) | (byte & 0x3f);
    }
    if (value < minimum || value > 0x10ffff ||
        (value >= 0xd800 && value <= 0xdfff) ||
        (value >= 0xfdd0 && value <= 0xfdef) || (value & 0xfffe) == 0xfffe)
        return 0;

    *cursor = p;
    return value;
}

static u32 fold_case(u32 value)
{
    if ((value >= 'A' && value <= 'Z') ||
        (value >= 0x00c0 && value <= 0x00de && value != 0x00d7) ||
        (value >= 0x0391 && value <= 0x03a9 && value != 0x03a2) ||
        (value >= 0x0410 && value <= 0x042f))
        return value + 0x20;
    if (value >= 0x0400 && value <= 0x040f)
        return value + 0x50;
    if (value == 0x0130)
        return 'i';
    if (value == 0x1e9e)
        return 0x00df;
    return value;
}

static void write_word(u8 *out, u32 value)
{
    out[0] = (u8)value;
    out[1] = (u8)(value >> 8);
}

int bee_answer(const u8 *in, usize n, u8 *out, usize *len)
{
    const u8 *end = in + n;
    const u8 *cursor = in;
    usize written = 0;
    int needs_bom = 0;

    while (cursor != end) {
        u32 value = read_codepoint(&cursor, end);
        if (value == 0)
            return 1;
        value = fold_case(value);
        if (value > 0x07ff)
            needs_bom = 1;

        usize width = value > 0xffff ? 4 : 2;
        if (width > BEE_OUT_MAX - written)
            return 1;
        if (width == 4) {
            value -= 0x10000;
            write_word(out + written, 0xd800 | (value >> 10));
            write_word(out + written + 2, 0xdc00 | (value & 0x3ff));
        } else {
            write_word(out + written, value);
        }
        written += width;
    }

    if (needs_bom) {
        if (written > BEE_OUT_MAX - 2)
            return 1;
        memmove(out + 2, out, written);
        write_word(out, 0xfeff);
        written += 2;
    }
    *len = written;
    return 0;
}
