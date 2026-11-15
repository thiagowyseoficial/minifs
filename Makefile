CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
SOURCES = main.c minifs.c permissoes.c caminhos.c armazenamento.c interface.c comandos.c

all: minifs

minifs: $(SOURCES) minifs.h
	$(CC) $(CFLAGS) $(SOURCES) -o minifs

clean:
	rm -f minifs minifs.exe

.PHONY: all clean
