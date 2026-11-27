int bee_answer(const u8 *in, usize n, u8 *out, usize *len)
{
    enum { escape = 0xc5, max_run = 64 };
    usize written = 0;

    for (usize pos = 0; pos < n;) {
        const u8 value = in[pos];
        const usize remaining = n - pos;
        usize run = 1;

        while (run < max_run && run < remaining && in[pos + run] == value)
            ++run;

        /* Short runs are literal, except when the value is the escape byte. */
        const int encoded = run >= 3 || value == escape;
        const usize size = encoded ? 3 : run;
        if (size > BEE_OUT_MAX - written)
            return 1;

        if (encoded) {
            out[written] = escape;
            out[written + 1] = (u8)(run - 1);
            out[written + 2] = value;
        } else {
            out[written] = value;
            if (run == 2)
                out[written + 1] = value;
        }

        written += size;
        pos += run;
    }

    *len = written;
    return 0;
}
