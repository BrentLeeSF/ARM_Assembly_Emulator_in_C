.global sum_array_a
.func sum_array_a

/* r0 = array, r1 = size */

sum_array_a:

        mov r2, #0 /* index */
        mov r3, #0 /* sum */

loop:

        cmp r2, r1
        beq done

        ldr r4, [r0]
	add r0, r0, #4

        add r3, r3, r4
        add r2, r2, #1

        b loop

done:

        mov r0, r3

        bx lr
