.global fib_iter_a
.func fib_iter_a

fib_iter_a:

        /* r0 = n */

        mov r1, #0 /* r1 = a, where a = n-1 */
        mov r2, #0 /* r2 = b, where b = n */
        mov r3, #2 /* r3 = i for index */
        mov r4, #0 /* r4 = temp variable */

        cmp r0, #0
        beq done

        cmp r0, #1
        beq done

loop:

        cmp r3, #2
        addeq r2, r2, #1

        cmp r3, #3
        addeq r1, r1, #1

        movgt r4, r2
        addgt r2, r2, r1
        movgt r1, r4

        cmp r0, r3
        beq done_add

        add r3, r3, #1
        b loop

done_add:

        add r0, r1, r2
        bx lr

done:

        mov r0, r0
        bx lr


