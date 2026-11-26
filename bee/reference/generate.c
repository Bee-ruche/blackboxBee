/* The generator of cases of blackboxBee (docs/reference-judge.md of Bee-ruche/hive).
 *
 *   generate <unit> <count> [CASES]     the seed (64 hexadecimal digits) on its input
 *
 * Writes exactly <count> cases of <unit> to CASES (by default /cases/cases, where the hive's judge wants them), framed
 * as the judge's kind says: <u32 length, little-endian><bytes>, each within the unit's bound in
 * bee/reference/project.json. The same seed gives the same cases. Each unit has a few fixed cases (its edges, as the
 * project's builders saw them) and draws the rest; the fixed ones are shuffled in among the drawn ones.
 *
 * This file knows rung 1's units. Every other unit of the manifest (rung 2 on: units/UNITS.md) has its generator inside
 * the black box: this program starts the black box beside it in its place (forward, below), with the same arguments
 * and the seed still unread on its input.
 *
 * What this file knows is the SHAPE of each unit's inputs (lengths, bytes, the grammar a parser is tried on), never
 * an answer: the answers are the reference's alone (reference/blackbox).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* ------------------------------------------------------------------------------- the draws: xoshiro256**, seeded */

static uint64_t S[4];

static uint64_t rotl(uint64_t x, int k)
{
    return (x << k) | (x >> (64 - k));
}

static uint64_t next(void)
{
    uint64_t result = rotl(S[1] * 5, 7) * 9, t = S[1] << 17;
    S[2] ^= S[0];
    S[3] ^= S[1];
    S[1] ^= S[2];
    S[0] ^= S[3];
    S[2] ^= t;
    S[3] = rotl(S[3], 45);
    return result;
}

static uint32_t below(uint32_t k)
{
    return (uint32_t)(next() % k);
}

static int chance(unsigned percent)
{
    return below(100) < percent;
}

static uint8_t byte(void)
{
    return (uint8_t)next();
}

/* Bytes of one of a few kinds: uniform, all 0xFF, all zero, a small alphabet, text. */
static void fill(uint8_t *c, size_t n)
{
    static const uint8_t SMALL[] = {0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF};
    unsigned mode = below(10);
    size_t i;
    for (i = 0; i < n; i++)
        c[i] = mode < 5 ? byte() : mode == 5 ? 0xFF : mode == 6 ? 0x00 : mode == 7 ? SMALL[below(sizeof SMALL)] : (uint8_t)(' ' + below(95));
}

/* A length up to room: zero now and then, short ones often, long ones, and the round sizes around 256 and 512. */
static size_t length(size_t room)
{
    static const size_t ROUND[] = {1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129, 255, 256, 257, 511, 512, 513};
    unsigned r = below(100);
    size_t n;
    if (r < 3)
        return 0;
    if (r < 40)
        return 1 + below(room < 16 ? (uint32_t)room : 16);
    if (r < 80)
        return 1 + below((uint32_t)room);
    n = ROUND[below(sizeof ROUND / sizeof ROUND[0])];
    return n > room ? room : n;
}

/* ------------------------------------------------------------------------------------------------ the units */

static size_t draw_sum16(uint8_t *c, size_t room)
{
    size_t n = length(room);
    fill(c, n);
    return n;
}

static size_t draw_scramble(uint8_t *c, size_t room)
{
    size_t n = length(room);
    fill(c, n);
    if (n > 0) {
        unsigned r = below(100);
        if (r < 8)
            c[0] = 0;
        else if (r < 14)
            c[0] = 1;
        else if (r < 20)
            c[0] = 0xFF;
    }
    return n;
}

static size_t draw_rle(uint8_t *c, size_t room)
{
    static const size_t LONG[] = {62, 63, 64, 65, 66, 127, 128, 129, 130, 192, 193};
    size_t n = 0, target, run, i;
    uint8_t b = 0;
    unsigned r;
    if (chance(2))
        return 0;
    target = chance(30) ? 1 + below(24) : 1 + below((uint32_t)room);
    while (n < target) {
        r = below(100);
        b = r < 25 ? 0xC5 : r < 40 ? 0x00 : r < 48 ? b : r < 56 ? 0xC4 : byte();
        r = below(100);
        run = r < 30 ? 1 : r < 50 ? 2 : r < 60 ? 3 : r < 75 ? 4 + below(60) : r < 90 ? LONG[below(sizeof LONG / sizeof LONG[0])] : 1 + below(200);
        for (i = 0; i < run && n < target; i++)
            c[n++] = b;
    }
    return n;
}

static size_t draw_hash32(uint8_t *c, size_t room)
{
    size_t n = length(room);
    fill(c, n);
    return n;
}

static size_t draw_pack32(uint8_t *c, size_t room)
{
    size_t n;
    unsigned r = below(100);
    if (r < 3)
        n = 0;
    else if (r < 10)
        n = 161 + below((uint32_t)(room - 160));
    else if (r < 50)
        n = 1 + below(12);
    else
        n = 1 + below(160);
    fill(c, n);
    return n;
}

static size_t draw_numfmt(uint8_t *c, size_t room)
{
    size_t n, i;
    unsigned r = below(100), mode;
    if (r < 3)
        return 0;
    n = r < 8 ? 10 + below((uint32_t)(room - 9)) : 1 + below(9);
    c[0] = byte();
    mode = below(8);
    for (i = 1; i < n; i++)
        c[i] = mode < 3 ? byte() : mode == 3 ? 0xFF : mode == 4 ? 0x00 : (uint8_t)below(16);
    if (n > 1 && mode == 5)
        c[n - 1] = 0x80;  /* the smallest of its length, when the rest are small */
    if (n > 1 && mode == 6)
        c[n - 1] = chance(50) ? 0x7F : 0x80;
    return n;
}

/* record: a text of fields « key = value » separated by « ; » — and texts a little off that shape. */

struct text {
    uint8_t *c;
    size_t n, room;
};

static void put(struct text *t, uint8_t b)
{
    if (t->n < t->room)
        t->c[t->n++] = b;
}

static void puts_(struct text *t, const char *s)
{
    while (*s)
        put(t, (uint8_t)*s++);
}

static void blank(struct text *t, unsigned percent)
{
    if (chance(percent)) {
        unsigned k = 1 + below(3);
        while (k--)
            put(t, ' ');
    }
}

static void key(struct text *t, char *kept, size_t *keptlen)
{
    static const char LETTERS[] = "abcdefghijklmnopqrstuvwxyz_";
    static const char WRONG[] = "AZ09-.\"=";
    unsigned r = below(100);
    size_t n = r < 1 ? 0 : r < 4 ? 9 + below(3) : r < 30 ? 1 : 1 + below(8), i;
    for (i = 0; i < n; i++) {
        char ch = chance(1) ? WRONG[below(sizeof WRONG - 1)] : chance(30) ? LETTERS[below(4)] : LETTERS[below(sizeof LETTERS - 1)];
        put(t, (uint8_t)ch);
        if (i < 15)
            kept[i] = ch;
    }
    *keptlen = n < 15 ? n : 15;
}

static void number(struct text *t)
{
    static const char *const EDGES[] = {"0", "-0", "2147483647", "2147483648", "-2147483648", "-2147483649", "4294967296",
                                        "9999999999", "10000000000", "-", "00", "007", "-01", "1", "-1", "99999999999"};
    char digits[16];
    unsigned r = below(100), k, i;
    if (r < 10) {
        puts_(t, EDGES[below(sizeof EDGES / sizeof EDGES[0])]);
        return;
    }
    if (chance(30))
        put(t, '-');
    k = r < 60 ? 1 + below(4) : 1 + below(11);
    for (i = 0; i < k; i++)
        digits[i] = (char)('0' + below(10));
    if (k > 1 && digits[0] == '0' && !chance(5))
        digits[0] = (char)('1' + below(9));
    for (i = 0; i < k; i++)
        put(t, (uint8_t)digits[i]);
}

static void string(struct text *t)
{
    static const char *const ESCAPES[] = {"\\\"", "\\\\", "\\n", "\\t"};
    static const char *const WRONG[] = {"\\q", "\\0", "\\x41", "\\u0041", "\\", "\t", "\n", "\x01"};
    unsigned r = below(100);
    size_t n = r < 10 ? 0 : r < 18 ? 30 + below(6) : below(29), i;
    put(t, '"');
    for (i = 0; i < n; i++) {
        r = below(100);
        if (r < 15)
            puts_(t, ESCAPES[below(4)]);
        else if (r < 16)
            puts_(t, WRONG[below(sizeof WRONG / sizeof WRONG[0])]);
        else if (r < 22)
            put(t, (uint8_t)(0x80 + below(128)));
        else
            put(t, (uint8_t)(r < 30 ? ' ' + below(95) : 'a' + below(26)));
    }
    if (!chance(3))
        put(t, '"');
}

static size_t draw_record(uint8_t *c, size_t room)
{
    static const char *const WORDS[] = {"true", "false", "true", "false", "true", "false", "tru", "True", "falsey", "nul", "truefalse"};
    struct text t = {c, 0, room};
    char keys[16][15];
    size_t lens[16], fields, f, i;
    unsigned r = below(100);
    fields = r < 3 ? 0 : r < 60 ? 1 + below(4) : r < 85 ? 1 + below(12) : 11 + below(4);
    blank(&t, 10);
    for (f = 0; f < fields; f++) {
        if (f > 0) {
            blank(&t, 15);
            put(&t, chance(1) ? ',' : chance(1) ? ' ' : ';');
            blank(&t, 15);
        }
        if (f > 0 && chance(6)) {  /* a key given twice */
            i = below((uint32_t)f);
            memcpy(keys[f], keys[i], lens[i]);
            lens[f] = lens[i];
            for (r = 0; r < lens[f]; r++)
                put(&t, (uint8_t)keys[f][r]);
        } else {
            key(&t, keys[f], &lens[f]);
        }
        blank(&t, 15);
        if (!chance(1))
            put(&t, '=');
        blank(&t, 15);
        r = below(100);
        if (r < 40)
            number(&t);
        else if (r < 80)
            string(&t);
        else if (r < 98)
            puts_(&t, WORDS[below(sizeof WORDS / sizeof WORDS[0])]);
        else if (r < 99)
            put(&t, (uint8_t)("x'{[+."[below(6)]));
        /* else: no value at all */
    }
    if (fields > 0 && chance(4))
        put(&t, ';');
    blank(&t, 10);
    return t.n;
}

/* vm: byte code, mostly of the shapes a small program has, now and then a byte at random. */

static size_t draw_vm(uint8_t *c, size_t room)
{
    static const uint16_t COUNTS[] = {1, 2, 3, 15, 16, 17, 63, 64, 65, 250, 255, 256, 257, 300, 332, 333, 334, 400};
    size_t n = 0, steps, k;
    int depth = 0;
    unsigned r, count;
    uint16_t m;
    if (chance(2))
        return 0;
    count = 1 + below(40);
    for (k = 0; k < count && n + 12 <= room; k++) {
        r = below(100);
        if (depth < 2 || r < 25) {
            r = below(10);
            if (r < 6) {
                c[n++] = (uint8_t)below(16);
            } else if (r < 9) {
                c[n++] = 0x10;
                c[n++] = byte();
            } else {
                c[n++] = 0x11;
                c[n++] = byte();
                c[n++] = byte();
            }
            depth++;
        } else if (r < 35) {
            c[n++] = (uint8_t)(0x18 + below(4));
            depth += c[n - 1] == 0x1A ? -1 : c[n - 1] == 0x19 ? 0 : 1;
        } else if (r < 60) {
            c[n++] = (uint8_t)(0x20 + below(12));
            depth -= c[n - 1] < 0x2A ? 1 : 0;
        } else if (r < 78) {
            c[n++] = (uint8_t)(0x30 + below(2));
            depth--;
        } else if (r < 85) {
            c[n++] = (uint8_t)(0x40 + below(3));
            c[n++] = (uint8_t)(int8_t)((int)below(15) - 7);
            depth -= c[n - 2] == 0x42 ? 0 : 1;
        } else if (r < 89) {
            /* a counted loop: push m; L: dup emit push1 sub dup jnz L */
            m = COUNTS[below(sizeof COUNTS / sizeof COUNTS[0])];
            if (chance(50))
                m = (uint16_t)(1 + below(40));
            c[n++] = 0x11;
            c[n++] = (uint8_t)m;
            c[n++] = (uint8_t)(m >> 8);
            c[n++] = 0x18;
            c[n++] = chance(85) ? 0x30 : 0x31;
            c[n++] = 0x01;
            c[n++] = 0x21;
            c[n++] = 0x18;
            c[n++] = 0x41;
            c[n++] = (uint8_t)(int8_t)-7;
            depth++;
        } else if (r < 91) {
            c[n++] = 0xFF;
        } else if (r < 94) {
            c[n++] = byte();
        } else if (r < 96) {
            c[n++] = (uint8_t)below(2);  /* zero or one, then a division */
            c[n++] = (uint8_t)(0x28 + below(2));
        } else if (r < 97) {
            for (steps = 0; steps < 9 && n < room; steps++)
                c[n++] = (uint8_t)below(16);
            depth += 9;
        } else {
            c[n++] = 0x42;
            c[n++] = chance(50) ? 0xFE : (uint8_t)(0x70 + below(16));  /* to itself, or far off */
        }
        if (depth < 0)
            depth = 0;
    }
    while (depth > 0 && n < room && !chance(15)) {
        c[n++] = chance(75) ? 0x30 : 0x31;
        depth--;
    }
    return n;
}

/* ------------------------------------------------------------------------------------------- the fixed cases */

struct fixed {
    const char *bytes;
    size_t len;
};

#define F(s) {s, sizeof(s) - 1}

static const struct fixed SUM16[] = {F(""), F("\0"), F("a"), F("\xff\xff\xff\xff"), F("\x01\x02")};
static const struct fixed SCRAMBLE[] = {F(""), F("\0"), F("\0abc"), F("\x01"), F("\x01\0\0\0\0\0\0\0\0\0"), F("\xff\xff\xff\xff\xff\xff\xff\xff\xff")};
static const struct fixed RLE[] = {F(""), F("\xc5"), F("\xc5\xc5"), F("\xc5\xc5\xc5"), F("aa"), F("aaa"), F("ab"), F("\0\0\0\0"), F("a\xc5" "a")};
static const struct fixed HASH32[] = {F(""), F("\0"), F("abc"), F("abcd"), F("abcde")};
static const struct fixed PACK32[] = {F(""), F("\0"), F("\xff"), F("\0\0\0\0\0"), F("\xff\xff\xff\xff\xff"), F("abcd"), F("abcdef")};
static const struct fixed NUMFMT[] = {F(""), F("\0"), F("\x0c\x05"), F("\x0c"), F("\x08"), F("\x08\0"), F("\x04\xff"), F("\x01\0\0\0\0\0\0\0\x80"),
                                      F("\x03\xff\xff\xff\xff\xff\xff\xff\x7f"), F("\xf0\x01"), F("\xf1\x01"), F("\xf3\x01"), F("\x00\0\0\0\0\0\0\0\0\0"),
                                      F("\x05\x80"), F("\x0a\x2a"), F("\x06\xff\xff")};
static const struct fixed RECORD[] = {
    F(""), F("   "), F("a=1"), F("a=1;"), F("a = 1 ; b = \"x\""), F("a=1;a=2"), F("abcdefgh=1"), F("abcdefghi=1"),
    F("a=2147483647"), F("a=2147483648"), F("a=-2147483648"), F("a=-2147483649"), F("a=-0"), F("a=007"), F("a=0"), F("a=-"),
    F("a=\"\\q\""), F("a=\"\\n\\t\\\\\\\"\""), F("a=\"0123456789abcdef0123456789abcdef\""), F("a=\"0123456789abcdef0123456789abcdefg\""),
    F("a=true;b=false"), F("a=True"), F("z=1;a=2;m=3;_=4"), F("ab=1;a=2"), F("a=1 b=2"), F("a=\"open"), F("=1"), F("A=1"),
    F("a=1;b=2;c=3;d=4;e=5;f=6;g=7;h=8;i=9;j=10;k=11;l=12"), F("a=1;b=2;c=3;d=4;e=5;f=6;g=7;h=8;i=9;j=10;k=11;l=12;m=13"),
    F("a=\"\xe9t\xe9\""), F("a=\"\x7f\""), F("a=1;;b=2"), F(" a=1 "), F("a=1;\x00")};
static const struct fixed VM[] = {
    F(""), F("\x05\x30"), F("\x30"), F("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x01"),
    F("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x01\x02"), F("\x05\x00\x28"), F("\x05\x00\x29"), F("\x42\x7f"),
    F("\x42\x00"), F("\x42\x01"), F("\x42\xfe"), F("\xff\x05\x30"), F("\x10"), F("\x11\x01"), F("\x12"), F("\x40"), F("\x00\x40\x01\xff"),
    F("\x11\x00\x01\x18\x30\x01\x21\x18\x41\xf9"), F("\x11\x01\x01\x18\x30\x01\x21\x18\x41\xf9"), F("\x11\x4d\x01\x18\x1a\x01\x21\x18\x41\xf9"),
    F("\x11\x4e\x01\x18\x1a\x01\x21\x18\x41\xf9"), F("\x05\x03\x26\x31"), F("\x01\x2b\x31"), F("\x01\x2a\x31"), F("\x01\x1f\x26\x02\x27\x31"),
    F("\x07\x02\x19\x21\x31"), F("\x07\x02\x1b\x31\x31\x31")};

/* ------------------------------------------------------------------------------------------------ the program */

struct unit {
    const char *name;
    size_t room;  /* the unit's case_bytes in bee/reference/project.json: a case is never longer */
    size_t (*draw)(uint8_t *c, size_t room);
    const struct fixed *fixed;
    size_t fixed_count;
};

#define U(name, room, fixed) {#name, room, draw_##name, fixed, sizeof fixed / sizeof fixed[0]}

static const struct unit UNITS[] = {
    U(sum16, 600, SUM16), U(scramble, 300, SCRAMBLE), U(rle, 512, RLE), U(hash32, 300, HASH32),
    U(pack32, 200, PACK32), U(numfmt, 12, NUMFMT), U(record, 400, RECORD), U(vm, 128, VM),
};

static int hex(int ch)
{
    return ch >= '0' && ch <= '9' ? ch - '0' : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 : ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : -1;
}

static void put32(uint8_t *at, uint32_t v)
{
    at[0] = (uint8_t)v;
    at[1] = (uint8_t)(v >> 8);
    at[2] = (uint8_t)(v >> 16);
    at[3] = (uint8_t)(v >> 24);
}

/* A unit this file does not know (rung 2 on: units/UNITS.md) has its generator inside the black box: the black box,
 * beside this program (its folder read from /proc/self/exe: /ref/reference in the judge, the reference copied by
 * bee/check.sh), is started in its place, given « --generate <unit> <count> [CASES] » and this program's input, the seed
 * not yet read. */
static void forward(char **argv)
{
    static char path[4096];
    char *args[6] = {path, "--generate", argv[1], argv[2], argv[3], NULL};
    ssize_t size = readlink("/proc/self/exe", path, sizeof path - sizeof "reference");
    char *slash;
    if (size <= 0 || (size_t)size >= sizeof path - sizeof "reference") {
        errno = size < 0 ? errno : ENAMETOOLONG;
        return;
    }
    path[size] = 0;
    if (!(slash = strrchr(path, '/'))) {
        errno = ENOENT;
        return;
    }
    strcpy(slash + 1, "reference");
    execv(path, args);
}

int main(int argc, char **argv)
{
    const struct unit *unit = NULL;
    uint8_t seed[32], *cases, frame[4];
    size_t *lengths, *order, count, i, j, swap;
    char *end;
    FILE *out;
    int ch, high;
    if (argc == 3 || argc == 4)
        for (i = 0; i < sizeof UNITS / sizeof UNITS[0]; i++)
            if (strcmp(argv[1], UNITS[i].name) == 0)
                unit = &UNITS[i];
    if (!unit && (argc == 3 || argc == 4)) {
        forward(argv);  /* returns only if the black box cannot be started */
        perror("generate: the black box beside this program cannot be started");
        return 3;
    }
    if (!unit) {
        fputs("usage: generate <unit> <count> [CASES]   (the seed, 64 hexadecimal digits, on the input)\n", stderr);
        return 2;
    }
    count = strtoul(argv[2], &end, 10);
    if (*end || count < unit->fixed_count || count > 100000) {
        fprintf(stderr, "generate: a count from %zu to 100000\n", unit->fixed_count);
        return 2;
    }
    for (i = 0; i < 64; i++) {
        if ((ch = hex(getchar())) < 0) {
            fputs("generate: the seed is 64 hexadecimal digits on the input\n", stderr);
            return 2;
        }
        high = ch;
        if (i % 2)
            seed[i / 2] = (uint8_t)(seed[i / 2] << 4 | high);
        else
            seed[i / 2] = (uint8_t)high;
    }
    for (i = 0; i < 4; i++) {
        S[i] = 0;
        for (j = 0; j < 8; j++)
            S[i] |= (uint64_t)seed[8 * i + j] << (8 * j);
    }
    if (!(S[0] | S[1] | S[2] | S[3]))
        S[0] = 1;
    for (i = 0; i < 16; i++)
        next();
    cases = malloc(count * unit->room);
    lengths = malloc(count * sizeof *lengths);
    order = malloc(count * sizeof *order);
    if (!cases || !lengths || !order) {
        fputs("generate: out of memory\n", stderr);
        return 3;
    }
    for (i = 0; i < count; i++) {
        uint8_t *c = cases + i * unit->room;
        if (i < unit->fixed_count) {
            lengths[i] = unit->fixed[i].len;
            memcpy(c, unit->fixed[i].bytes, lengths[i]);
        } else {
            lengths[i] = unit->draw(c, unit->room);
        }
        if (lengths[i] > unit->room) {
            fputs("generate: a case longer than its bound (a defect of this file)\n", stderr);
            return 3;
        }
        order[i] = i;
    }
    for (i = count - 1; i > 0; i--) {
        j = below((uint32_t)(i + 1));
        swap = order[i];
        order[i] = order[j];
        order[j] = swap;
    }
    out = fopen(argc == 4 ? argv[3] : "/cases/cases", "wb");
    if (!out) {
        fputs("generate: cannot open the file of cases\n", stderr);
        return 3;
    }
    for (i = 0; i < count; i++) {
        put32(frame, (uint32_t)lengths[order[i]]);
        if (fwrite(frame, 1, 4, out) != 4 || fwrite(cases + order[i] * unit->room, 1, lengths[order[i]], out) != lengths[order[i]]) {
            fputs("generate: cannot write the cases\n", stderr);
            return 3;
        }
    }
    if (fclose(out) != 0) {
        fputs("generate: cannot write the cases\n", stderr);
        return 3;
    }
    free(cases);
    free(lengths);
    free(order);
    return 0;
}
