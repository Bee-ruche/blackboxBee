int bee_answer(const u8 *in, usize n, u8 *out, usize *len)
{
    if (n == 0 || in[0] == 0)
        return 1;

    u8 state = in[0];
    u8 bias = 0;
    for (usize i = 1; i < n; ++i) {
        u32 byte = (u8)((in[i] ^ state) + bias);
        u32 shift = (state + i) & 7u;
        out[i - 1] = (u8)((byte << shift) | (byte >> (8u - shift)));
        state = (u8)(5 * state + 1);
        bias = (u8)(bias + 7);
    }
    *len = n - 1;
    return 0;
}
