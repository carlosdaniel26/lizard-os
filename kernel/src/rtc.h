#pragma once

#include <types.h>

typedef struct RTCTimer {
    u8 seconds;       /* 00*/
    u8 seconds_alarm; /* 01*/
    u8 minutes;       /* 02*/
    u8 minutes_alarm; /* 03*/
    u8 hours;         /* 04*/
    u8 hours_alarm;   /* 05*/
    u8 day_in_week;   /* 06*/
    u8 day_of_month;  /* 07*/
    u8 month;         /* 08*/
    u8 year;          /* 09*/

    u8 status_register_A; /* A*/
    u8 status_register_B; /* B*/
    u8 status_register_C; /* C*/
    u8 status_register_D; /* D*/
} RTCTimer;

void rtc_write(const RTCTimer *in);
void rtc_read(RTCTimer *out);
