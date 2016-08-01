CFLAGS := -Wall -Wextra -O2 -g -pipe -I/home/pi/.local/local/include
LDLIBS := -lvc4vec

M4 := m4
QASM2 := qasm2
QBIN2HEX := qbin2hex
RM := rm -f

all: main

main.o: transpose.qhex

main: main.o

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
	$(RM) main main.o transpose.qhex transpose.qbin transpose.qasm2
