EXE=client server

.PHONY: all
all: $(EXE)

%: %.c
	gcc -Wall -ggdb -o $@ $< -pthread -lm

.PHONY: clean
clean:
	rm -rfv $(EXE)
