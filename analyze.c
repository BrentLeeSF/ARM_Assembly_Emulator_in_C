/* Example program of how to analyize ARM machine code */

#include <stdio.h>

//int add(int a, int b);
int addsub(int a, int b);

int main(int argc, char **argv) {

// op 00 = data processing (add, subtract, move, etc)
    unsigned int iw;
    unsigned int *pc;
    unsigned int opcode, rn, rd, rm;

    /* Get the address of the first instruction of the addsub assembly
       function in memory. */
    pc = (unsigned int *) addsub;
    printf("pc = %X\n", (unsigned) pc);

    /* Get the 32 bit instruction work at the pc address. */
    iw = *pc;

    /* Show hex representation of iw. */
    printf("iw = %X\n", iw);

    /* Extract the opcode. */
    opcode = (iw >> 21) & 0b1111;

    printf("opcode = %d\n", opcode);

    /* Extract the register operand numbers. */
    rn = (iw >> 16) & 0b1111;
    rd = (iw >> 12) & 0b1111;
    rm = iw & 0b1111;

    printf("rn = %d\n", rn);
    printf("rd = %d\n", rd);
    printf("rm = %d\n", rm);

    /* next instruction */

    pc = pc + 1;
    printf("pc = %X\n", (unsigned) pc);

    /* Get the 32 bit instruction work at the pc address. */
    iw = *pc;

    /* Show hex representation of iw. */
    printf("iw = %X\n", iw);

    /* Extract the opcode. */
    opcode = (iw >> 21) & 0b1111;

    printf("opcode = %d\n", opcode);

    /* Extract the register operand numbers. */
    rn = (iw >> 16) & 0b1111;
    rd = (iw >> 12) & 0b1111;
    rm = iw & 0b1111;

    printf("rn = %d\n", rn);
    printf("rd = %d\n", rd);
    printf("rm = %d\n", rm);

    return 0;
}
