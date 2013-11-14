
.section ".text.boot"
.globl Start
Start:
    ldr pc,reset_handler
    ldr pc,undefined_handler
    ldr pc,swi_handler
    ldr pc,prefetch_handler
    ldr pc,data_handler
    ldr pc,unused_handler
    ldr pc,irq_handler
    ldr pc,fiq_handler
reset_handler:      .word reset
undefined_handler:  .word hang
swi_handler:        .word hang
prefetch_handler:   .word hang
data_handler:       .word hang
unused_handler:     .word hang
irq_handler:        .word irq
fiq_handler:        .word hang

reset:

	;@ Move interrupt vector to 0x0000
    mov r0,#0x8000
    mov r1,#0x0000
    ldmia r0!,{r2,r3,r4,r5,r6,r7,r8,r9}
    stmia r1!,{r2,r3,r4,r5,r6,r7,r8,r9}
    ldmia r0!,{r2,r3,r4,r5,r6,r7,r8,r9}
    stmia r1!,{r2,r3,r4,r5,r6,r7,r8,r9}

	;@ Setup the stacks
	;@ IRQ
	mov r0, #0xD2
	msr cpsr_c, r0
	mov sp, #0x8000

	;@ FIQ
	mov r0, #0xD1
	msr cpsr_c, r0
	mov sp, #0x4000

	;@ SVC
	mov r0, #0xD3
	msr cpsr_c, r0
	mov sp, #0x80000
 
	;@ Clear out bss.
	ldr	r4, =_bss_start
	ldr	r9, =_bss_end
	mov	r5, #0
	mov	r6, #0
	mov	r7, #0
	mov	r8, #0
        b       2f
 
1:
	;@ store multiple at r4.
	stmia	r4!, {r5-r8}
 
	;@ If we are still below bss_end, loop.
2:
	cmp	r4, r9
	blo	1b
 
	;@ ~~~~~~~~~ Call cmain ~~~~~~~~~~~
	ldr	r3, =cmain
	blx	r3
 
	;@ hang
hang:
	wfe
	b	hang

.globl enable_irq
enable_irq:
    mrs r0,cpsr
    bic r0,r0,#0x80
    msr cpsr_c,r0
    bx lr

irq:
    push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
    bl c_irq_handler
    pop  {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
    subs pc,lr,#4
