/* hash32: the black box's bit-mixing hash, written again from its machine code.
   The state starts from the input's length mixed with a fixed constant; each byte is folded in by an xor and a
   multiplication, and after every fourth byte the state is rotated left by 5; a final avalanche of shifts, xors and
   multiplications mixes it. The answer is the 32-bit state, little-endian. */

static u32 rotl32(u32 x, unsigned r)
{
    return (x << r) | (x >> (32u - r));
}

int bee_answer(const u8 *in, usize n, u8 *out, usize *len)
{
    u32 h = (u32)n ^ 0x811c9dc5u;
    usize i;

    for (i = 0; i < n; i++) {
        h ^= in[i];
        h *= 0x01000193u;
        if ((i & 3u) == 3u)
            h = rotl32(h, 5);
    }
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;

    out[0] = (u8)h;
    out[1] = (u8)(h >> 8);
    out[2] = (u8)(h >> 16);
    out[3] = (u8)(h >> 24);
    *len = 4;
    return 0;
}
