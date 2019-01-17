CFLAGS += -Wall -Wextra -Wpedantic -Waggregate-return -Wwrite-strings -Wvla -Wfloat-equal

CFLAGS += -std=c11

CFLAGS +=-D_XOPEN_SOURCE=700


OUTPUT = -o bin/relay

relay: dispatcher listener

dispatcher: src/dispatcher.o
	$(CC) $(CFLAGS) $^ -o ./bin/dispatcher

listener: src/listener.o
	$(CC) $(CFLAGS) $^ -o ./bin/listener

debug: CFLAGS += -g
debug: dispatcher listener

profile: CFLAGS += -pg
profile: dispatcher listener

.PHONY: clean

clean:
	rm -f *.o *.out ./src/*.o ./bin/dispatcher ./bin/listener

