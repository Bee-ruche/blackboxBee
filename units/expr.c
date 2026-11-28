struct expression {
    const u8 *cursor;
    const u8 *end;
    unsigned depth;
    unsigned first_base;
    int failed;
};

enum precedence {
    PREC_NONE,
    PREC_XOR,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_RELATION,
    PREC_SUM,
    PREC_PRODUCT
};

static u8 peek(struct expression *expr)
{
    while (expr->cursor != expr->end && *expr->cursor == ' ')
        ++expr->cursor;
    return expr->cursor == expr->end ? 0 : *expr->cursor;
}

static unsigned digit_value(u8 ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return 16;
}

static u32 read_literal(struct expression *expr)
{
    unsigned base = 10;
    u64 limit = 2147483647u;
    if (expr->end - expr->cursor >= 2 && expr->cursor[0] == '0' && expr->cursor[1] == 'x') {
        expr->cursor += 2;
        base = 16;
        limit = 0xffffffffu;
    }

    const u8 *start = expr->cursor;
    u64 value = 0;
    while (expr->cursor != expr->end) {
        unsigned digit = digit_value(*expr->cursor);
        if (digit >= base)
            break;
        value = value * base + digit;
        if (value > limit) {
            expr->failed = 1;
            return 0;
        }
        ++expr->cursor;
    }
    if (expr->cursor == start)
        expr->failed = 1;
    if (expr->first_base == 0)
        expr->first_base = base;
    return (u32)value;
}

static u32 read_expression(struct expression *expr, enum precedence minimum);

static u32 read_operand(struct expression *expr)
{
    u8 ch = peek(expr);
    const u8 *prefix = expr->cursor;
    while (ch == '-' || ch == '~' || ch == '!') {
        ++expr->cursor;
        ch = peek(expr);
    }
    const u8 *atom = expr->cursor;
    u32 value;
    if (ch == '(') {
        if (expr->depth == 16) {
            expr->failed = 1;
            return 0;
        }
        ++expr->depth;
        ++expr->cursor;
        value = read_expression(expr, PREC_XOR);
        if (expr->failed || peek(expr) != ')') {
            expr->failed = 1;
            return 0;
        }
        ++expr->cursor;
        --expr->depth;
    } else {
        value = read_literal(expr);
    }

    /* The prefix operators apply from the operand back to the left. */
    while (atom != prefix) {
        switch (*--atom) {
        case '-': value = 0u - value; break;
        case '~': value = ~value; break;
        case '!': value = value == 0; break;
        }
    }
    return value;
}

static enum precedence operator_precedence(u8 op)
{
    switch (op) {
    case '^': return PREC_XOR;
    case '|': return PREC_OR;
    case '&': return PREC_AND;
    case '=':
    case '!': return PREC_EQUALITY;
    case '<':
    case '>': return PREC_RELATION;
    case '+':
    case '-': return PREC_SUM;
    case '*':
    case '/':
    case '%': return PREC_PRODUCT;
    default: return PREC_NONE;
    }
}

static i64 signed_word(u32 value)
{
    return value <= 0x7fffffffu ? (i64)value : (i64)value - 4294967296LL;
}

static u32 apply_operator(struct expression *expr, u8 op, u32 left, u32 right)
{
    switch (op) {
    case '^': return left ^ right;
    case '|': return left | right;
    case '&': return left & right;
    case '=': return left == right;
    case '!': return left != right;
    case '<': return signed_word(left) < signed_word(right);
    case '>': return signed_word(left) > signed_word(right);
    case '+': return left + right;
    case '-': return left - right;
    }

    i64 a = signed_word(left);
    i64 b = signed_word(right);
    i64 result;
    if (op == '*') {
        result = a * b;
    } else {
        if (b == 0) {
            expr->failed = 1;
            return 0;
        }
        i64 remainder = a % b;
        int round_down = remainder != 0 && ((remainder < 0) != (b < 0));
        result = op == '%' ? remainder + (round_down ? b : 0) : a / b - round_down;
    }
    if (result < -2147483648LL || result > 2147483647LL)
        expr->failed = 1;
    return (u32)result;
}

static u32 read_expression(struct expression *expr, enum precedence minimum)
{
    u32 left = read_operand(expr);
    enum precedence previous = PREC_NONE;
    while (!expr->failed) {
        u8 op = peek(expr);
        enum precedence precedence = operator_precedence(op);
        if (precedence < minimum)
            break;

        usize width = 1;
        if (precedence == PREC_EQUALITY) {
            if (expr->end - expr->cursor < 2 || expr->cursor[1] != '=')
                break;
            width = 2;
        }
        if (precedence == previous && (precedence == PREC_RELATION || precedence == PREC_EQUALITY)) {
            expr->failed = 1;
            break;
        }
        expr->cursor += width;
        u32 right = read_expression(expr, precedence + 1);
        if (!expr->failed)
            left = apply_operator(expr, op, left, right);
        previous = precedence;
    }
    return left;
}

int bee_answer(const u8 *in, usize n, u8 *out, usize *len)
{
    struct expression expr = { in, in + n, 0, 0, 0 };
    u32 result = read_expression(&expr, PREC_XOR);
    if (expr.failed)
        return 1;
    peek(&expr);
    if (expr.cursor != expr.end)
        return 1;

    u8 *p = out;
    if (expr.first_base == 16) {
        static const char hex[] = "0123456789abcdef";
        *p++ = '0';
        *p++ = 'x';
        for (int shift = 28; shift >= 0; shift -= 4)
            *p++ = hex[(result >> shift) & 15u];
    } else {
        if (result > 0x7fffffffu) {
            *p++ = '-';
            result = 0u - result;
        }
        u8 digits[10];
        usize count = 0;
        do {
            digits[count++] = (u8)('0' + result % 10u);
            result /= 10u;
        } while (result);
        while (count)
            *p++ = digits[--count];
    }
    *len = (usize)(p - out);
    return 0;
}
