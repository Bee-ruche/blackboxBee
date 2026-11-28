static const u8 digits[16] = "0123456789abcdef";

int bee_answer(const u8 *in, usize n, u8 *out, usize *len)
{
    if (n == 0 || n > 9) {
        return 1;
    }

    u8 flags = in[0];
    if ((flags & 0x0c) == 0x0c) {
        return 1;
    }

    u64 magnitude = 0;
    for (usize i = 1; i < n; ++i) {
        magnitude |= (u64)in[i] << (8 * (i - 1));
    }
    int negative = n > 1 && (in[n - 1] & 0x80) != 0;
    if (negative) {
        if (n < 9) {
            magnitude |= ~(u64)0 << (8 * (n - 1));
        }
        magnitude = 0 - magnitude;
    }

    int hexadecimal = (flags & 2) != 0;
    int parenthesized = negative && (flags & 4) != 0;
    usize written = 0;
    if (negative) {
        out[written++] = parenthesized ? '(' : '-';
    } else if ((flags & 8) != 0 && magnitude != 0) {
        out[written++] = '+';
    }
    if (hexadecimal) {
        out[written++] = '0';
        out[written++] = 'x';
    }

    u32 base = hexadecimal ? 16 : 10;
    u8 reversed[20];
    usize count = 0;
    do {
        reversed[count++] = digits[magnitude % base];
        magnitude /= base;
    } while (magnitude != 0);
    while (count < (usize)(flags >> 4)) {
        reversed[count++] = '0';
    }

    usize group = hexadecimal ? 4 : 3;
    u8 separator = hexadecimal ? '_' : ',';
    while (count != 0) {
        out[written++] = reversed[--count];
        if ((flags & 1) != 0 && count != 0 && count % group == 0) {
            out[written++] = separator;
        }
    }
    if (parenthesized) {
        out[written++] = ')';
    }

    *len = written;
    return 0;
}
