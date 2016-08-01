CFLAGS := -Wall -Wextra -O2 -g -pipe

M4 := m4
QASM2 := qasm2
QBIN2HEX := qbin2hex
RM := rm -f

all: main cpu_only

main.o: transpose.qhex

main: CFLAGS += -I/home/pi/.local/local/include
main: LDLIBS += -lvc4vec
main: main.o

cpu_only: cpu_only.o

.PRECIOUS: %.qasm2
%.qasm2: %.qasm2m4
	$(M4) <"$<" >"$@"

.PRECIOUS: %.qbin
%.qbin: %.qasm2
	$(QASM2) <"$<" >"$@"

.PRECIOUS: %.qhex
%.qhex: %.qbin
	$(QBIN2HEX) <"$<" >"$@"

.PHONY: clean
clean:
	$(RM) main main.o transpose.qhex transpose.qbin transpose.qasm2 cpu_only cpu_only.o
