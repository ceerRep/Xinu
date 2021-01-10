.code16

.global code16_source_start
.global code16_source_int10
.global code16_source_end

code16_source_start:

code16_source_int10:
int $0x10
hlt
iret

code16_source_end:
