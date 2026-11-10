/* Hai task giữ pattern khác nhau trong từng thanh ghi FP khi tick preempt.
 * Hàm tuân thủ ilp32f: lưu/khôi phục fs0..fs11 và fcsr của bên gọi. */
    .text
    .balign 4
    .global fx1_smoke_fpu_probe
    .type fx1_smoke_fpu_probe, @function
fx1_smoke_fpu_probe:
    addi sp, sp, -64
    fsw fs0, 0(sp)
    fsw fs1, 4(sp)
    fsw fs2, 8(sp)
    fsw fs3, 12(sp)
    fsw fs4, 16(sp)
    fsw fs5, 20(sp)
    fsw fs6, 24(sp)
    fsw fs7, 28(sp)
    fsw fs8, 32(sp)
    fsw fs9, 36(sp)
    fsw fs10, 40(sp)
    fsw fs11, 44(sp)
    frcsr t1
    sw t1, 48(sp)

    .irp n,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    addi t1, a0, \n
    fmv.w.x f\n, t1
    .endr
    fscsr a1
    lw t0, 0(a2)
.Lwait:
    lw t1, 0(a2)
    sub t1, t1, t0
    bltu t1, a3, .Lwait

    .irp n,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    li t4, (\n + 1)
    addi t2, a0, \n
    fmv.x.w t1, f\n
    bne t1, t2, .Lbad
    .endr
    li t4, 33
    frcsr t1
    bne t1, a1, .Lbad
    li a0, 0
    j .Lrestore
.Lbad:
    mv a0, t4
.Lrestore:
    flw fs0, 0(sp)
    flw fs1, 4(sp)
    flw fs2, 8(sp)
    flw fs3, 12(sp)
    flw fs4, 16(sp)
    flw fs5, 20(sp)
    flw fs6, 24(sp)
    flw fs7, 28(sp)
    flw fs8, 32(sp)
    flw fs9, 36(sp)
    flw fs10, 40(sp)
    flw fs11, 44(sp)
    lw t1, 48(sp)
    fscsr t1
    addi sp, sp, 64
    ret
    .size fx1_smoke_fpu_probe, .-fx1_smoke_fpu_probe
