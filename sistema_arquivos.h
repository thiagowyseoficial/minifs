#ifndef SISTEMA_ARQUIVOS_H
#define SISTEMA_ARQUIVOS_H

#include <stddef.h>
#include <stdio.h>
#include <time.h>

#define BLOCK_SIZE 64
#define BLOCK_COUNT 256
#define MAX_BLOCKS 64
#define MAX_NODES 256
#define NAME_SIZE 64
#define LINE_SIZE 8192
#define MAX_ARGS 32
#define R 4
#define W 2
#define X 1

/* Estruturas compartilhadas pelos modulos. */

typedef enum
{
    DIRECTORY,
    CHARACTER,
    NUMERIC,
    BINARY,
    PROGRAM
} FileType;
typedef struct FCB
{
    char name[NAME_SIZE];
    unsigned long inode;
    FileType type;
    unsigned mode; /* Nove bits RWX: proprietario, grupo e outros. */
    unsigned uid;
    unsigned gid;
    size_t size; /* Tamanho real do conteudo, em bytes. */
    size_t block_count;
    int blocks[MAX_BLOCKS];
    time_t created;
    time_t modified;
    time_t accessed;
    int used;         /* Posicao ocupada na tabela. */
    int parent_index; /* Indice do pai; -1 para raiz ou entrada desligada. */
} FCB;

/* Estado unico da simulacao, definido em sistema_arquivos.c. */
extern FCB *root;
extern FCB *current_directory;
extern unsigned uid;
extern unsigned gid;
extern int command_failed;
extern const char *type_names[];

/* sistema_arquivos.c: Estado do sistema e gerenciamento da tabela de FCBs. */
int show_error(const char *message);
FCB *find_child(FCB *parent, const char *name);
FCB *create_node(FCB *parent, const char *name, FileType type, unsigned mode);
void detach_node(FCB *node);
FCB *parent_node(const FCB *node);
FCB *child_at(const FCB *parent, size_t index);
void attach_node(FCB *node, FCB *parent);
int has_children(const FCB *node);
void destroy_tree(FCB *node);

/* permissoes.c: Selecao da classe de usuario e verificacao dos bits RWX. */
int check_permission(const FCB *node, unsigned bits);

/* caminhos.c: Validacao de nomes e resolucao de caminhos na arvore. */
FCB *resolve_path(const char *path);
FCB *resolve_parent(const char *path, char name[NAME_SIZE], unsigned required_permissions);
FCB *resolve_destination(const char *path, const char *source_name, char name[NAME_SIZE]);

/* armazenamento.c: Disco simulado, mapa de ocupacao e alocacao indexada. */
void release_blocks(FCB *node);
int write_data(FCB *node, const unsigned char *data, size_t size);
void read_data(const FCB *node, unsigned char *data);
void print_disk_usage(void);

/* interface.c: Leitura dos argumentos e apresentacao dos metadados. */
void print_node(const FCB *node);
void print_path(FCB *node);
int parse_number(const char *s, unsigned *value, int base, unsigned maximum);
int parse_command_line(char *line, char **args);
void print_help(void);

/* comandos.c: Operacoes do usuario e selecao dos comandos. */
void execute_command(int argc, char **arguments);

#endif
