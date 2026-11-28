enum {
    STACK_LIMIT = 16,
    OUTPUT_LIMIT = 256,
    INSTRUCTION_LIMIT = 2000,
};

int bee_answer(const u8 *in, usize n, u8 *out, usize *len)
{
    u32 stack[STACK_LIMIT];
    usize depth = 0;
    usize pc = 0;
    usize written = 0;
    unsigned steps = 0;

    while (pc < n) {
        if (steps == INSTRUCTION_LIMIT) {
            return 1;
        }
        ++steps;

        u8 opcode = in[pc++];
        if (opcode == 0xff) {
            break;
        }
        if (opcode <= 0x0f) {
            if (depth == STACK_LIMIT) {
                return 1;
            }
            stack[depth++] = opcode;
            continue;
        }
        if (opcode >= 0x20 && opcode <= 0x29) {
            if (depth < 2) {
                return 1;
            }
            u32 right = stack[--depth];
            u32 *left = &stack[depth - 1];
            if ((opcode == 0x28 || opcode == 0x29) && right == 0) {
                return 1;
            }
            switch (opcode) {
            case 0x20: *left += right; break;
            case 0x21: *left -= right; break;
            case 0x22: *left *= right; break;
            case 0x23: *left &= right; break;
            case 0x24: *left |= right; break;
            case 0x25: *left ^= right; break;
            case 0x26: *left <<= right & 31u; break;
            case 0x27: *left >>= right & 31u; break;
            case 0x28: *left /= right; break;
            case 0x29: *left %= right; break;
            }
            continue;
        }

        switch (opcode) {
        case 0x10:
        case 0x11: {
            // Push an unsigned byte or a little-endian 16-bit word.
            usize width = opcode == 0x10 ? 1 : 2;
            if (depth == STACK_LIMIT || n - pc < width) {
                return 1;
            }
            u32 value = in[pc++];
            if (width == 2) {
                value |= (u32)in[pc++] << 8;
            }
            stack[depth++] = value;
            break;
        }
        case 0x18:
        case 0x1b: {
            // Copy the first or second word from the top.
            usize distance = opcode == 0x18 ? 1 : 2;
            if (depth < distance || depth == STACK_LIMIT) {
                return 1;
            }
            stack[depth] = stack[depth - distance];
            ++depth;
            break;
        }
        case 0x19: {
            if (depth < 2) {
                return 1;
            }
            u32 top = stack[depth - 1];
            stack[depth - 1] = stack[depth - 2];
            stack[depth - 2] = top;
            break;
        }
        case 0x1a:
            if (depth == 0) {
                return 1;
            }
            --depth;
            break;
        case 0x2a:
        case 0x2b: {
            if (depth == 0) {
                return 1;
            }
            u32 value = stack[depth - 1];
            stack[depth - 1] = opcode == 0x2a ? ~value : 0u - value;
            break;
        }
        case 0x30:
        case 0x31: {
            usize width = opcode == 0x30 ? 1 : 4;
            if (depth == 0 || written + width > OUTPUT_LIMIT) {
                return 1;
            }
            u32 value = stack[--depth];
            for (usize byte = 0; byte < width; ++byte) {
                out[written++] = (u8)(value >> (8 * byte));
            }
            break;
        }
        case 0x40:
        case 0x41:
        case 0x42: {
            if (pc == n) {
                return 1;
            }
            int offset = in[pc++];
            if (opcode != 0x42) {
                if (depth == 0) {
                    return 1;
                }
                int is_zero = stack[--depth] == 0;
                if (is_zero != (opcode == 0x40)) {
                    break;
                }
            }
            // The displacement is signed and relative to the following byte.
            if (offset >= 128) {
                offset -= 256;
            }
            i64 target = (i64)pc + offset;
            if (target < 0 || target > n) {
                return 1;
            }
            pc = (usize)target;
            break;
        }
        default:
            return 1;
        }
    }

    *len = written;
    return 0;
}
