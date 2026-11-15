CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
SOURCES = principal.c sistema_arquivos.c permissoes.c caminhos.c armazenamento.c interface.c comandos.c

all: minifs

minifs: $(SOURCES) sistema_arquivos.h
	$(CC) $(CFLAGS) $(SOURCES) -o minifs

clean:
	rm -f minifs minifs.exe

.PHONY: all clean
