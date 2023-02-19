#include "lib-header/stdtype.h"
#include "lib-header/gdt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to GDT definition in Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */

struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            /* Implementation */
            .segment_low = 0,
            .segment_high = 0,

            .base_low = 0,
            .base_mid = 0,
            .base_high = 0,

            .type_bit = 0,

            .non_system = 0,
            .dpl = 0,
            .present_bit = 0,
            .long_mode = 0,
            .big = 0,
            .gran = 0
        },
        {
            /* Implementation */
            .segment_low = 0xFFFF,
            .segment_high = 0xF,

            .base_low = 0,
            .base_mid = 0,
            .base_high = 0,

            .type_bit = 0xA,

            .non_system = 1,
            .dpl = 0,
            .present_bit = 1,
            .long_mode = 0,
            .big = 1,
            .gran = 1
        },
        {
            /* Implementation */
            .segment_low = 0xFFFF,
            .segment_high = 0xF,

            .base_low = 0,
            .base_mid = 0,
            .base_high = 0,

            .type_bit = 0x2,

            .non_system = 1,
            .dpl = 0,
            .present_bit = 1,
            .long_mode = 0,
            .big = 1,
            .gran = 1
        }}};

/**
 * _gdt_gdtr, predefined system GDTR.
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    /* Implementation */

    .size = sizeof(global_descriptor_table) - 1,
    .address = &global_descriptor_table
};
