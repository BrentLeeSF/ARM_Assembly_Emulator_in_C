.global find_max_a
.func find_max_a

find_max_a:

        /* r0 = array, r1 = size */

        sub sp, sp, #4
        str lr, [sp]
        sub sp, sp, #4
        str r4, [sp]
        sub sp, sp, #4
        str r5, [sp]
        sub sp, sp, #4

        mov r2, #0 /* index */
        ldr r5, [r0]
        add r0, r0, #4
        add r2, r2, #1

loop:

        cmp r2, r1
        beq done

        ldr r4, [r0]
        add r0, r0, #4

        cmp r4, r5
        movgt r5, r4
        add r2, r2, #1

        b loop

done:

        mov r0, r5

        add sp, sp, #4
        ldr r5, [sp]
        add sp, sp, #4
        ldr r4, [sp]
        add sp, sp, #4
        ldr lr, [sp]
        add sp, sp, #4

        bx lr
