struct bit_stream {
    const u8 *next;
    const u8 *end;
    unsigned used;
};

static int read_bits(struct bit_stream *stream, unsigned count)
{
    unsigned value = 0;
    for (unsigned i = 0; i < count; ++i) {
        if (stream->next == stream->end)
            return -1;
        unsigned bit = (*stream->next >> stream->used) & 1u;
        value |= bit << i;
        if (++stream->used == 8) {
            stream->used = 0;
            ++stream->next;
        }
    }
    return (int)value;
}

int bee_answer(const u8 *in, usize n, u8 *out, usize *len)
{
    if (n == 0)
        return 1;
    unsigned distance_bits = in[0] & 15u;
    unsigned minimum = in[0] >> 4;
    if (distance_bits < 8 || distance_bits > 12 || minimum < 2 || minimum > 5)
        return 1;

    struct bit_stream stream = { in + 1, in + n, 0 };
    unsigned end_code = (1u << distance_bits) - 1;
    usize written = 0;
    for (;;) {
        int tag = read_bits(&stream, 1);
        if (tag < 0)
            return 1;
        if (tag == 1) {
            int byte = read_bits(&stream, 8);
            if (byte < 0 || written == BEE_OUT_MAX)
                return 1;
            out[written++] = (u8)byte;
            continue;
        }

        /* A zero prefix selects the width of the length suffix. */
        unsigned prefix = 0;
        for (;;) {
            int bit = read_bits(&stream, 1);
            if (bit < 0)
                return 1;
            if (bit == 1)
                break;
            if (++prefix > 11)
                return 1;
        }
        int suffix = read_bits(&stream, prefix);
        int code = read_bits(&stream, distance_bits);
        if (suffix < 0 || code < 0)
            return 1;
        usize count = minimum + (1u << prefix) - 1 + (unsigned)suffix;

        if ((unsigned)code == end_code && count == minimum) {
            if (stream.used != 0) {
                if (*stream.next >> stream.used)
                    return 1;
                ++stream.next;
            }
            if (stream.next != stream.end)
                return 1;
            *len = written;
            return 0;
        }

        usize distance = (unsigned)code + 1;
        if (distance > written || count > BEE_OUT_MAX - written)
            return 1;
        for (usize i = 0; i < count; ++i) {
            out[written] = out[written - distance];
            ++written;
        }
    }
}
