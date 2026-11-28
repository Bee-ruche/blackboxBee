static const u8 alphabet[32] = "0123456789abcdefghjkmnpqrstvwxyz";

int bee_answer(const u8 *in, usize n, u8 *out, usize *len)
{
    if (n == 0 || n > 160) {
        return 1;
    }

    u32 pending = 0;
    unsigned bits = 0;
    usize written = 0;
    u32 check = 0;

    for (usize i = 0; i < n; ++i) {
        pending = (pending << 8) | in[i];
        bits += 8;
        while (bits >= 5) {
            bits -= 5;
            u32 digit = (pending >> bits) & 31u;
            out[written++] = alphabet[digit];
            check += digit * written;
        }
    }

    // Pad the last digit with low zero bits and include it in the weighted sum.
    if (bits != 0) {
        u32 digit = (pending << (5 - bits)) & 31u;
        out[written++] = alphabet[digit];
        check += digit * written;
    }

    out[written++] = alphabet[(check ^ n) & 31u];
    *len = written;
    return 0;
}
