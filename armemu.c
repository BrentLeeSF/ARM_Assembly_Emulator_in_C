#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* This program emulates the register state of the CPU.
It calls each function and creates the necessary registers, flags, 
and size of stack for each function. Then gets each 32 bit instruction word (line of code), 
determines it's functionality, and does the functionality. 


THERE ARE THREE MAIN INSTRUCTIONS

1. Data Processing (add, sub, mov, mvn, cmp, etc). 
These instructions have the following: 
Cond (31:28), Op (27:26)=00, Funct (25:20), Rn (19:16), Rd (15:12), Src2 (11:0)

There are 3 options for Src2. If 25=I=1, then src2 is an immediate value. imm (11:8 = shift, 7:0 = imm8). 
If 25=I=0 then the src is a register. There are 2 register options.
Src2 = (shamt5 = shift by constant (11:7), sh=type of shift (6:5) = 00=LSL, 01=LSR, 10=ASR, 11=ROR, 0 (4), reg Rm (3:0)).
Src2 = (Rs another reg (11:8), 0 (7), sh (type of shift 6:5), 1 (4), Rm = reg (3:0)). Ex. Add R5, R6, R7 -> Rn = 6, Rd = 5, Rm = 7.

2. Memory Instructions (STR (store into memory), LDR (load from memory into register), STRB (store byte into memory), 
LDRB (load byte from memory into register)). 
Memory instruction has: cond, op=01, funct (25:20 - I=25 (25=1 for immediate or 25=0 for reg).
24=P (P=pre-index). 23=U. 22=B. 21=W (write back). 20=L (load and byte). See examples below)
I = imm (0=imm, U=subtr from base), (1=reg , U = add to base), P (pre-index) and W (write-back) specify index mode for mem instr. 
P=0,W=0 (post-ind). P=0, W = 1 (ind not support), P = 1, Off = 0 (Offset), P = 1, W = 1 (pre-ind), L=0 & B = 0 =STR, L= 0 & B = 1 =STRB, 
L=1 & B=0 =LDR, L=1 & B=1=LDRB. 
Memory also has: Rn (base reg/1st reg 19:16), Rd (15:12), Src2 (11:0) = 2 opt = imm12, OR, (11:7=shat5, 6:5=sh, 4=1, Rm=3:0). 
Offset = dist from begin to a point.

3. Branch Instuctions: cond (31:28), op (27:26 = 10), funct (25:24. 25=1, 24=0 if Branch, 24=1 if Branch and link), and 
imm24 (23:0 - immediate value used for instruction address of where to branch to).



cond (or condition - bits 31-28) determine the condition of the program (or if it should run).
The bits are as follows: NZCV (N=31, Z=30, C=29, V=28). 
N = negative, Z = zero, C = carry, V = overflow
Examples: cond = 1110 (in hex or 14 in decimal) = always run. cond = 0000 (ex. addeq add if equals).

Op (27:26 - op code or operation). 00=data instruction, 01=memory instruction (str,ldr,ldrb), 10=branch instruction.

funct (25:20 (I=25, cmd=24:21, s=20).) If I=1 then instruction uses immediate value (0=reigster). 
If S=1, then the cond sets a condition flag (see cond).

cmd (24:21) indicates the data processing instruction (ex. 0110=add, 0110=subtract, 1101=mov)

Rn (19:16. Is the 1st regester (base reg)).

Rd (15:12. Is the destination register. Where the value will be put into).

Src2 (11:0 - see Data Instructions for 3 options of Src2).



to run this program you must have a Raspberry Pi. In terminal call: 
1. make
2. ./armemu */

int sum_array_a(int *x, int y);
int find_max_a(int *x, int y);
int fib_iter_a(int n);
int fib_rec_a(int n);

#define NREGS 16
#define SP 13
#define LR 14
#define PC 15

struct arm_state {

    unsigned int regs[NREGS];
    unsigned int cpsr;
    unsigned int stack_size;
    unsigned char *stack;

    unsigned int eq;
    unsigned int ne;
    unsigned int gt;
    unsigned int lt;
    unsigned int z;
    unsigned int n;
    unsigned int v;

    int num_instr;
    int data_instr;
    int b_instr;
    int mem_instr;

};


struct arm_state *arm_state_new(unsigned int stack_size, unsigned int *func,
                                unsigned int arg0, unsigned int arg1,
                                unsigned int arg2, unsigned int arg3) {

    struct arm_state *as;
    int i;

    as = (struct arm_state *) malloc(sizeof(struct arm_state));
    if (as == NULL) {
        printf("malloc() failed, exiting.\n");
        exit(-1);
    }

    as->stack = (unsigned char *) malloc(stack_size);
    if (as->stack == NULL) {
        printf("malloc() failed, exiting.\n");
        exit(-1);
    }

    as->stack_size = stack_size;

    /* Initialize all registers to zero. */
    for (i = 0; i < NREGS; i++) {
        as->regs[i] = 0;
    }

    as->regs[PC] = (unsigned int) func;
    as->regs[SP] = (unsigned int) as->stack + as->stack_size;

    as->regs[0] = arg0;
    as->regs[1] = arg1;
    as->regs[2] = arg2;
    as->regs[3] = arg3;

    as->eq = 0;
    as->ne = 0;
    as->gt = 0;
    as->lt = 0;

    as->z = 0;
    as->n = 0;
    as->v = 0;

    as->num_instr = 0;
    as->data_instr = 0;
    as->b_instr = 0;
    as->mem_instr = 0;

    return as;
}

void arm_state_free(struct arm_state *as) {

    free(as->stack);
    free(as);

}

void arm_state_print(struct arm_state *as) {

    int i;
    printf("stack size = %d\n", as->stack_size);

    for (i = 0; i < NREGS; i++) {
        printf("regs[%d] = (%X) %d\n", i, as->regs[i], (int) as->regs[i]);
    }

}

bool iw_is_add_instruction(unsigned int iw) {

    unsigned int op, opcode;

    op = (iw >> 26) & 0b11;
    opcode = (iw >> 21) & 0xF;

    return (op == 0) && (opcode == 4);

}

bool iw_is_sub_instruction(unsigned int iw) {

    unsigned int op, opcode;

    op = (iw >>  26) & 0b11;
    opcode = (iw >> 21) & 0xF;

    return (op == 0) && (opcode == 2);

}

bool iw_is_b_instruction(unsigned int iw) {

    unsigned int op, funct;

    op = (iw >> 25) & 0b111;
    funct = (iw >> 24) & 0b1;

    return (op == 5) && (funct == 0);

}

bool iw_is_bl_instruction(unsigned int iw) {

    unsigned int op, funct;

    op = (iw >> 25) & 0b111;
    funct = (iw >> 24) & 0b1;

    return (op == 5) && (funct == 1);

}

bool iw_is_mov_instruction(unsigned int iw) {

    unsigned int op, opcode;

    op = (iw >>  26) & 0b11;
    opcode = (iw >> 21) & 0xF;

    return ((op == 0b00) && (opcode == 0b1101));

}

bool iw_is_mvn_instruction(unsigned int iw) {

    unsigned int op, opcode;

    op = (iw >> 26) & 0b11;
    opcode = (iw >> 21) & 0xF;

    return (op == 0b00) && (opcode == 0xF);

}

bool iw_is_cmp_instruction(unsigned int iw) {

    unsigned int op, opcode;

    op = (iw >> 26) & 0b11;
    opcode = (iw >> 21) & 0xF;

    return (op == 0) && (opcode == 10);

}

bool iw_is_ldr_instruction(unsigned int iw) {

    unsigned int opcode, l_flag, b_flag;

    opcode = (iw >> 26) & 0b11;
    l_flag = (iw >> 20) & 0b1;
    b_flag = (iw >> 22) & 0b1;

    return (opcode == 1) && (l_flag == 1) && (b_flag == 0);

}


bool iw_is_str_instruction(unsigned int iw) {

    unsigned int opcode, l_flag, b_flag;

    opcode = (iw >> 26) & 0b11;
    l_flag = (iw >> 20) & 0b1;
    b_flag = (iw >> 22) & 0b1;
    return (opcode == 1) && (l_flag == 0) && (b_flag == 0);

}

bool is_valid(struct arm_state *as, unsigned int cond) {

    bool cond_valid = false;

    if(cond == 0b0000) {
        if(as->z == 1 && as->eq == 1) {
            cond_valid = true;
        }

    } else if(cond == 0b0001) {
        if(as->ne == 1) {
            cond_valid = true;
        }

    } else if(cond == 0b1011) {
        if(as->lt == 1) {
            cond_valid = true;
        }

    } else if(cond == 0b1100) {
        if(as->z !=1 && as->n == as->v && as->gt == 1) {
            cond_valid = true;
        }

    } else if(cond == 0b1110) {
        cond_valid = true;
    }

    return cond_valid;

}

void execute_add_instruction(struct arm_state *as, unsigned int iw) {

    unsigned int rd, rn, value, immediate, cond;

    as->num_instr++;
    as->data_instr++;

    rd = (iw >> 12) & 0xF;
    rn = (iw >> 16) & 0xFF;
    immediate = (iw >> 25) &0b1;
    cond = (iw >> 28) & 0xF;

    if(is_valid(as, cond)) {

    	if(immediate > 0) {

	    value = iw & 0xFF;
	    as->regs[rd] = as->regs[rn] + value;

    	} else {
	    value = iw & 0xF;
    	    as->regs[rd] = as->regs[rn] + as->regs[value];
    	}

	as->regs[PC] += 4;

    } else {
    	as->regs[PC] += 4;
    }

}

void execute_sub_instruction(struct arm_state *as, unsigned int iw) {

    unsigned int rd, rn, rm, value, immediate, cond;

    as->num_instr++;
    as->data_instr++;

    rd = (iw >> 12) & 0xF;
    rn = (iw >> 16) & 0xF;
    immediate = (iw >> 25) & 0b1;
    cond = (iw >> 28) & 0xF;

    if(is_valid(as, cond)) {

        if(immediate > 0) {
    	    value = iw & 0xFF;
	    as->regs[rd] = as->regs[rn] - value;

        } else {
	    value = iw & 0xF;
	    as->regs[rd] = as->regs[rn] - as->regs[value];
    	}

 	as->regs[PC] += 4;

    } else {
    	as->regs[PC] += 4;
    }

}

void execute_mvn_instruction(struct arm_state *as, unsigned int iw) {

    unsigned int rd, rn, value, immediate, cond, opcode, thirty_two;

    rd = (iw >> 12) & 0xF;
    rn = (iw >> 16) & 0xF;
    immediate = (iw >> 25) & 0b1;
    cond = (iw >> 28) & 0xF;
    opcode = (iw >> 21) & 0xF;

    if(is_valid(as, cond)) {

	as->num_instr++;
	as->data_instr++;

        if(immediate > 0) {

            value = iw & 0xFF;
	    value = ~value;
            as->regs[rd] = value;

        } else {

            value = iw & 0b1111;
            as->regs[rd] = as->regs[value];

        }

        if(rd != PC) {
            as->regs[PC] += 4;
        }

    } else if(rd != PC) {
        as->regs[PC] += 4;
    }

}

void execute_mov_instruction(struct arm_state *as, unsigned int iw) {

    unsigned int rd, rn, value, immediate, cond, opcode, thirty_two;

    rd = (iw >> 12) & 0xF;
    rn = (iw >> 16) & 0xF;

    immediate = (iw >> 25) & 0b1;
    cond = (iw >> 28) & 0xF;
    opcode = (iw >> 21) & 0xF;

    if(is_valid(as, cond)) {

	as->num_instr++;
	as->data_instr++;

    	if(immediate > 0) {

            value = iw & 0xFF;
            as->regs[rd] = value;

    	} else {

	    value = iw & 0xF;
	    as->regs[rd] = as->regs[value];

    	}

	if(rd != PC) {
		as->regs[PC] += 4;
	}

    } else if(rd != PC) {
    	as->regs[PC] += 4;
    }

}

void execute_cmp_instruction(struct arm_state *as, unsigned int iw) {

    unsigned int rd, rn, op2, immediate, cmp_result, reg_val1, reg_val2;

    as->num_instr++;
    as->data_instr++;

    rd = (iw >> 12) & 0xF;
    rn = (iw >> 16) & 0xF;
    immediate = (iw >> 25) & 0b1;

    as->eq = 0;
    as->ne = 0;
    as->lt = 0;
    as->gt = 0;
    as->z = 0;
    as->n = 0;
    as->v = 0;

    if(immediate > 0) {

        op2 = iw & 0b11111111;

	cmp_result = as->regs[rn] - op2;
	reg_val1 = as->regs[rn];
	reg_val2 = op2;

    } else {

        op2 = iw & 0b1111;

	cmp_result = as->regs[rn] - as->regs[op2];
	reg_val1 = as->regs[rn];
	reg_val2 = as->regs[op2];

    }

    if(cmp_result == 0) {

	as->eq = 1;
	as->z = 1;

    } else if(reg_val1 < reg_val2) {

	as->lt = 1;
	as->ne = 1;

    } else if(reg_val1 > reg_val2) {

	as->gt = 1;
	as->ne = 1;

    }

    if(cmp_result > 10000) {

	as->v = 1;
 	as->n = 1;

    }

    as->regs[PC] += 4;

}

void execute_b_instruction(struct arm_state *as, unsigned int iw) {

    unsigned int thirty_two, offset, cond;

    offset = (iw & 0xFFFFFF);
    cond = (iw >> 28) & 0xF;

    if(is_valid(as, cond)) {

	as->num_instr++;
	as->b_instr++;

	if(offset & 0x800000) {

	    thirty_two = 0xFF000000 + offset;

	} else {

	    thirty_two = offset;

	}

	thirty_two = thirty_two << 2;
	as->regs[PC] += 8;
	as->regs[PC] += thirty_two;

    } else {
	as->regs[PC] += 4;
    }

}

void execute_bl_instruction(struct arm_state *as, unsigned int iw) {

    unsigned int thirty_two, offset, cond;

    offset = (iw & 0xFFFFFF);
    cond = (iw >> 28) & 0xF;

    if(is_valid(as, cond)) {

	as->num_instr++;
	as->b_instr++;

        if(offset & 0x800000) {

            thirty_two = 0xFF000000 + offset;

        } else {

            thirty_two = offset;

        }

        thirty_two = thirty_two << 2;
	    as->regs[PC] += 8;
        as->regs[LR] = as->regs[PC] - 4;
        as->regs[PC] += thirty_two;

    } else {
        as->regs[PC] += 4;
    }

}

void execute_ldr_instruction(struct arm_state *as, unsigned int iw) {

    unsigned int rm, rd, rn;

    as->num_instr++;
    as->mem_instr++;

    rd = (iw >> 12) & 0xF;
    rn = (iw >> 16) & 0xF;
    rm = iw & 0xF;

    unsigned int *num = (unsigned int *)as->regs[rn];
    as->regs[rd] = *num;

    if(rd != PC) {
    	as->regs[PC] += 4;
    }

}

void execute_str_instruction(struct arm_state *as, unsigned int iw) {

    unsigned int src2, rd, rn;

    as->num_instr++;
    as->mem_instr++;

    rn = (iw >> 16) & 0b1111;
    rd = (iw >> 12) & 0b1111;

    unsigned int *num = (unsigned int *)as->regs[rn];
    *num = as->regs[rd];

    if(rd != PC) {
	as->regs[PC] += 4;
    }

}

bool iw_is_bx_instruction(unsigned int iw) {

    return ((iw >> 4) & 0xFFFFFF) == 0b000100101111111111110001;

}

void execute_bx_instruction(struct arm_state *as, unsigned int iw) {

    unsigned int rn;

    as->num_instr++;
    as->b_instr++;

    rn = iw & 0b1111;
    as->regs[PC] = as->regs[rn];

}

void arm_state_execute_one(struct arm_state *as) {

    unsigned int iw;
    unsigned int *pc;

    pc = (unsigned int *) as->regs[PC];
    iw = *pc;


    if(iw_is_bx_instruction(iw)) {
        execute_bx_instruction(as, iw);

    } else if(iw_is_add_instruction(iw)) {
	   execute_add_instruction(as, iw);

    } else if(iw_is_sub_instruction(iw)) {
	   execute_sub_instruction(as, iw);

    } else if(iw_is_mov_instruction(iw)) {
	   execute_mov_instruction(as, iw);

    } else if(iw_is_mvn_instruction(iw)) {
	   execute_mvn_instruction(as, iw);

    } else if(iw_is_cmp_instruction(iw)) {
	   execute_cmp_instruction(as, iw);

    } else if(iw_is_ldr_instruction(iw)) {
	   execute_ldr_instruction(as, iw);

    } else if(iw_is_str_instruction(iw)) {
	   execute_str_instruction(as, iw);

    } else if(iw_is_b_instruction(iw)) {
	   execute_b_instruction(as, iw);

    } else if(iw_is_bl_instruction(iw)) {
	   execute_bl_instruction(as, iw);

    }

}

unsigned int arm_state_execute(struct arm_state *as) {

    while (as->regs[PC] != 0) {
        arm_state_execute_one(as);
    }

    return as->regs[0];
}

void test_sum() {

    struct arm_state *as;
    unsigned int rv;

    int arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int arr2[] = {-1,-2,-3,-4,-5,-6,-7,-8,-9, -10};
    int arr_zero[] = {0, 1, 0, 3, 0, 5, 0, 7, 0, 9};

    int arr_thousand[1000];
    int i = 0, start = 1000;

    for(i = 0; i < 1000; i++) {
        if(i%3 == 0) {
            arr_thousand[i] = 0;
        } else {
            arr_thousand[i] = start;
            start += 2;
        }
    }

    as = arm_state_new(1024, (unsigned int *)sum_array_a, (unsigned int)arr, 10, 0, 0);
    rv = arm_state_execute(as);
    printf("\n\nSUM from 1 to 10 = %d\n\n", rv);

    as = arm_state_new(1024, (unsigned int *)sum_array_a, (unsigned int)arr2, 10, 0, 0);
    rv = arm_state_execute(as);
    printf("SUM from -1 to -10 = %d\n\n", rv);

    as = arm_state_new(1024, (unsigned int *)sum_array_a, (unsigned int)arr_zero, 10, 0, 0);
    rv = arm_state_execute(as);
    printf("SUM of numbers. Positive numbers are zeros = %d\n\n", rv);

    as = arm_state_new(1024, (unsigned int *)sum_array_a, (unsigned int)arr_thousand, 1000, 0, 0);
    rv = arm_state_execute(as);
    printf("SUM of 1000. If i%3 == 0, make a 0, else += 2 = %d\n\n", rv);

    printf("Sum Number of instructions %d\n",as->num_instr);
    printf("Sum Data Instructions %d\n",as->data_instr);
    printf("Sum Memory Instructions %d\n",as->mem_instr);
    printf("Sum Branch Instructions %d\n",as->b_instr);
}

void test_max() {

    struct arm_state *as;
    unsigned int rv;

    int arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int arr2[] = {-1,-2,-3,-4,-5,-6,-7,-8,-9, -10};
    int arr_zero[] = {0, 1, 0, 3, 0, 5, 0, 7, 0, 9};

    int arr_thousand[1000];
    int i = 0, start = 1000;

    for(i = 0; i < 1000; i++) {
    	if(i%3 == 0) {
            arr_thousand[i] = 0;
        } else {
            arr_thousand[i] = start;
            start += 2;
        }
    }

    as = arm_state_new(1024, (unsigned int *)find_max_a, (unsigned int)arr, 10, 0, 0);
    rv = arm_state_execute(as);
    printf("\n\nMAX from 1 to 10 = %d\n\n", rv);

    as = arm_state_new(1024, (unsigned int *)find_max_a, (unsigned int)arr2, 10, 0, 0);
    rv = arm_state_execute(as);
    printf("MAX from -1 to -10 = %d\n\n", rv);

    as = arm_state_new(1024, (unsigned int *)find_max_a, (unsigned int)arr_zero, 10, 0, 0);
    rv = arm_state_execute(as);
    printf("MAX from 0 through 9 = %d\n\n", rv);

    as = arm_state_new(1024, (unsigned int *)find_max_a, (unsigned int)arr_thousand, 1000, 0, 0);
    rv = arm_state_execute(as);
    printf("MAX from 1000. If i%3 == 0, make a 0, else += 2 = %d\n\n", rv);

    printf("Max Number of instructions %d\n",as->num_instr);
    printf("Max Data Instructions %d\n",as->data_instr);
    printf("Max Memory Instructions %d\n",as->mem_instr);
    printf("Max Branch Instructions %d\n",as->b_instr);

}

void test_fib_iter() {

    int j = 0;
    struct arm_state *as;
    unsigned int rv;

    printf("\n\nFib iter\n\n");

    for(j = 0; j < 20; j++) {

        as = arm_state_new(1024, (unsigned int *)fib_iter_a, (unsigned int)j, 0, 0, 0);
        rv = arm_state_execute(as);
        printf("%d, ", rv);

    }

    printf("\n\n");

    printf("Fib Iteration Number of instructions %d\n",as->num_instr);
    printf("Fib Iteration Data Instructions %d\n",as->data_instr);
    printf("Fib Iteration Memory Instructions %d\n",as->mem_instr);
    printf("Fib Iteration Branch Instructions %d\n",as->b_instr);

}

void test_fib_rec() {

    int j = 0;
    struct arm_state *as;
    unsigned int rv;

    printf("\n\nFib Rec\n\n", rv);

    for(j = 0; j < 20; j++) {

        as = arm_state_new(1024, (unsigned int *)fib_rec_a, (unsigned int)j, 0, 0, 0);
        rv = arm_state_execute(as);
        printf("%d, ",rv);

    }

    printf("\n\n");

    printf("Fib Recursion Number of instructions %d\n",as->num_instr);
    printf("Fib Recursion Data Instructions %d\n",as->data_instr);
    printf("Fib Recursion Memory Instructions %d\n",as->mem_instr);
    printf("Fib Recursion Branch Instructions %d\n",as->b_instr);

}

int main(int argc, char **argv) {

    test_sum();

    test_max();

    test_fib_iter();

    test_fib_rec();

    return 0;

}
