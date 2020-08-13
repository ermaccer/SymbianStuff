.align 2

.global __StackExtender_ReadSP
.global __StackExtender_WriteSP

__StackExtender_ReadSP:
    mov     r0,sp
    bx      lr

__StackExtender_WriteSP:
    mov     sp,r0
    bx      lr
