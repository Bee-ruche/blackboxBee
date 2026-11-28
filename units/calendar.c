enum {
    CALENDAR_DAY_FIRST = 1,
    CALENDAR_WEEKDAY = 2,
    CALENDAR_SHORT_YEAR = 4,
    CALENDAR_ORDINAL = 8,
    CALENDAR_CYCLE_YEARS = 500,
    CALENDAR_CYCLE_DAYS = 182621
};

static int leap_year(i32 year)
{
    return year % 4 == 0 && (year % 100 != 0 || year % 500 == 0);
}

static u8 *write_decimal(u8 *out, i32 value, usize width)
{
    u8 digits[10];
    usize count = 0;
    u32 magnitude = (u32)value;

    if (value < 0) {
        *out++ = '-';
        magnitude = 0u - magnitude;
    }
    do {
        digits[count++] = (u8)('0' + magnitude % 10);
        magnitude /= 10;
    } while (magnitude != 0);
    while (count < width)
        digits[count++] = '0';
    while (count > 0)
        *out++ = digits[--count];
    return out;
}

int bee_answer(const u8 *in, usize n, u8 *out, usize *len)
{
    static const u8 march_months[] = {
        31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31
    };
    static const u8 weekdays[7][3] = {
        "Thu", "Fri", "Sat", "Sun", "Mon", "Tue", "Wed"
    };

    if (n < 2 || n > 9 || in[0] > 15)
        return 1;
    const u8 flags = in[0];

    /* The bytes after the flags are a signed little-endian day number. */
    i64 day_number = in[n - 1];
    if (day_number >= 128)
        day_number -= 256;
    for (usize i = n - 1; i > 1; --i)
        day_number = day_number * 256 + in[i - 1];
    if (day_number < -50000000 || day_number > 50000000)
        return 1;

    /* Day zero is 1700-03-01. Move back to the cycle starting in March 1500. */
    i32 shifted_day = (i32)day_number + 73048;
    i32 cycle = shifted_day / CALENDAR_CYCLE_DAYS;
    i32 remaining = shifted_day % CALENDAR_CYCLE_DAYS;
    if (remaining < 0) {
        --cycle;
        remaining += CALENDAR_CYCLE_DAYS;
    }
    i32 year = 1500 + cycle * CALENDAR_CYCLE_YEARS;

    /* A March-based year ends with February of the next calendar year. */
    i32 year_length = 365 + leap_year(year + 1);
    while (remaining >= year_length) {
        remaining -= year_length;
        ++year;
        year_length = 365 + leap_year(year + 1);
    }

    i32 march_offset = remaining;
    usize month_index = 0;
    while (month_index < sizeof march_months && remaining >= march_months[month_index])
        remaining -= march_months[month_index++];
    i32 month = (i32)month_index + 3;
    i32 day = remaining + 1;
    i32 ordinal;
    if (month > 12) {
        month -= 12;
        ++year;
        ordinal = day + (month == 2 ? 31 : 0);
    } else {
        ordinal = march_offset + 60 + leap_year(year);
    }

    usize year_width = 4;
    if (flags & CALENDAR_SHORT_YEAR) {
        if (year < 1950 || year > 2049)
            return 1;
        year %= 100;
        year_width = 2;
    }

    u8 *p = out;
    if (flags & CALENDAR_ORDINAL) {
        p = write_decimal(p, year, year_width);
        *p++ = '.';
        p = write_decimal(p, ordinal, 3);
    } else if (flags & CALENDAR_DAY_FIRST) {
        p = write_decimal(p, day, 2);
        *p++ = '/';
        p = write_decimal(p, month, 2);
        *p++ = '/';
        p = write_decimal(p, year, year_width);
    } else {
        p = write_decimal(p, year, year_width);
        *p++ = '-';
        p = write_decimal(p, month, 2);
        *p++ = '-';
        p = write_decimal(p, day, 2);
    }

    if (flags & CALENDAR_WEEKDAY) {
        i32 weekday = (i32)day_number % 7;
        if (weekday < 0)
            weekday += 7;
        *p++ = ' ';
        for (usize i = 0; i < sizeof weekdays[0]; ++i)
            *p++ = weekdays[weekday][i];
    }
    *len = (usize)(p - out);
    return 0;
}
