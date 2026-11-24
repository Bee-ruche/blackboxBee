/* The four memory functions a unit's module is given (units/UNITS.md): a unit is compiled for WebAssembly with no C
 * library, and the compiler calls these on its own for copies and fills. bee/reference/build-unit compiles this file
 * into every unit's module, with -fno-builtin, so that these loops are not turned back into calls to themselves.
 * Plain C, as a unit is: nothing here reaches outside the module.
 */
typedef unsigned long usize;

void *memcpy(void *to, const void *from, usize n)
{
    unsigned char *a = to;
    const unsigned char *b = from;
    while (n--)
        *a++ = *b++;
    return to;
}

void *memmove(void *to, const void *from, usize n)
{
    unsigned char *a = to;
    const unsigned char *b = from;
    if (a < b) {
        while (n--)
            *a++ = *b++;
    } else {
        a += n;
        b += n;
        while (n--)
            *--a = *--b;
    }
    return to;
}

void *memset(void *to, int c, usize n)
{
    unsigned char *a = to;
    while (n--)
        *a++ = (unsigned char)c;
    return to;
}

int memcmp(const void *one, const void *other, usize n)
{
    const unsigned char *a = one, *b = other;
    for (; n; n--, a++, b++)
        if (*a != *b)
            return *a < *b ? -1 : 1;
    return 0;
}
