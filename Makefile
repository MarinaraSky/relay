CFLAGS += -Wall -Wextra -Wpedantic -Waggregate-return -Wwrite-strings -Wvla -Wfloat-equal

CFLAGS += -std=c11

OUTPUT = -o bin/relay

relay: src/dispatcher.o src/listener.o
	$(CC) $(CFLAGS) $^ $(OUTPUT)

debug: CFLAGS += -g
debug: src/dispatcher.o src/listener.o
	$(CC) $(CFLAGS) $^ $(OUTPUT)

profile: CFLAGS += -pg
profile: src/dispatcher.o src/listener.o
	$(CC) $(CFLAGS) $^ $(OUTPUT)

.PHONY: clean

clean:
	rm -f *.o *.out ./src/*.o 

