int bee_answer(const u8 *in, usize n, u8 *out, usize *len)
{
    if (n == 0)
        return 1;

    u32 sum = 1;
    u32 running = 0;
    for (usize i = 0; i < n; ++i) {
        sum = (sum + in[i] + (u8)i) % 251u;
        running = (running + sum) % 253u;
    }

    out[0] = (u8)running;
    out[1] = (u8)(sum ^ n);
    *len = 2;
    return 0;
}
