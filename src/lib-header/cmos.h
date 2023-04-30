#include "stdtype.h"
#include "portio.h"

struct time {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

void cmos_read_rtc(struct time *t)
{
    uint8_t status;

    // Disable non-maskable interrupts
    asm volatile("cli");

    // Set the index register to 0x0B to select the RTC status register B
    out(0x70, 0x0B);

    // Read the current value of the RTC status register B
    status = in(0x71);

    // Set the index register to 0x00 to select the RTC seconds register
    out(0x70, 0x00);

    // Read the current value of the RTC seconds register
    t->second = in(0x71);

    // Set the index register to 0x02 to select the RTC minutes register
    out(0x70, 0x02);

    // Read the current value of the RTC minutes register
    t->minute = in(0x71);

    // Set the index register to 0x04 to select the RTC hours register
    out(0x70, 0x04);

    // Read the current value of the RTC hours register
    t->hour = in(0x71);

    // Set the index register to 0x07 to select the RTC day of the month register
    out(0x70, 0x07);

    // Read the current value of the RTC day of the month register
    t->day = in(0x71);

    // Set the index register to 0x08 to select the RTC month register
    out(0x70, 0x08);

    // Read the current value of the RTC month register
    t->month = in(0x71);

    // Set the index register to 0x09 to select the RTC year register
    out(0x70, 0x09);

    // Read the current value of the RTC year register
    t->year = in(0x71);

    // Convert BCD-encoded values to binary values
    if (!(status & 0x04)) {
        t->second = (t->second & 0x0F) + ((t->second >> 4) * 10);
        t->minute = (t->minute & 0x0F) + ((t->minute >> 4) * 10);
        t->hour = ((t->hour & 0x0F) + (((t->hour & 0x70) >> 4) * 10)) | (t->hour & 0x80);
        
        // convert hour to GMT + 7
        t->hour += 7;

        t->day = (t->day & 0x0F) + ((t->day >> 4) * 10);
        t->month = (t->month & 0x0F) + ((t->month >> 4) * 10);
        t->year = (t->year & 0x0F) + ((t->year >> 4) * 10);
    }

    // Enable non-maskable interrupts
    asm volatile("sti");
}