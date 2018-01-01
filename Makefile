PROGS = armemu
OBJS =

CFLAGS = -g

all : ${PROGS}

armemu : armemu.c sum_array_a.s find_max_a.s fib_iter_a.s fib_rec_a.s find_str_a.s
        gcc ${CFLAGS} -o armemu armemu.c sum_array_a.s find_max_a.s fib_iter_a.s fib_rec_a.s find_str_a.s

clean:
        rm -rf ${PROGS} ${OBJS}