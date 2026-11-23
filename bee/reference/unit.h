/* What a unit's file is given (units/UNITS.md). bee/reference/build-unit compiles a unit's file with this header
 * included before its first line (« -include »): a unit's file holds no preprocessor line of its own.
 *
 * A unit is one function, bee_answer: the input is in[0..n); it writes its answer to out (BEE_OUT_MAX bytes of room),
 * sets *len to the answer's length and returns 0; or it refuses the input and returns 1. Any other return value, or an
 * answer longer than BEE_OUT_MAX, stops the driver (the run fails).
 */
typedef unsigned char u8;
typedef signed char i8;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned int u32;
typedef signed int i32;
typedef unsigned long long u64;
typedef signed long long i64;
typedef unsigned long usize;

enum { BEE_OUT_MAX = 4096 };

int bee_answer(const u8 *in, usize n, u8 *out, usize *len);
