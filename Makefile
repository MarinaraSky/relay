CFLAGS += -Wall -Wextra -Wpedantic -Waggregate-return -Wwrite-strings -Wvla -Wfloat-equal

CFLAGS += -std=c11

CFLAGS +=-D_XOPEN_SOURCE=700 -pthread

comp = $(CC) $(CFLAGS) $^ -o ./bin/$@

relay: dispatcher listener

dispatcher: src/dispatcher.o
	$(comp)

listener: src/listener.o
	$(comp)

debug: CFLAGS += -g
debug: dispatcher listener

profile: CFLAGS += -pg
profile: dispatcher listener

.PHONY: clean

clean:
	rm -f *.o *.out ./src/*.o ./bin/dispatcher ./bin/listener

