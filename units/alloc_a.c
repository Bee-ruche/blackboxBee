/* The blank unit: the project's stand-in for any unit, in the judge's control (docs/reference-judge.md of
   Bee-ruche/hive). It builds in a unit's place and answers nothing: its status stops the driver on the first case. */
int bee_answer(const u8 *in, usize n, u8 *out, usize *len)
{
    (void)in;
    (void)n;
    (void)out;
    (void)len;
    return -1;
}
