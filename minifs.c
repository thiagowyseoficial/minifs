/* Estado do sistema e gerenciamento da tabela de FCBs.
 * Alunos: Thiago Wyse e Matheus Souza.
 */
#include "minifs.h"
#include <string.h>

FCB *root = NULL;
FCB *current_directory = NULL;
unsigned uid = 1000;
unsigned gid = 1000;
int command_failed = 0;
const char *type_names[] = {"diretorio", "caractere", "numerico", "binario", "programa"};

/* Inodes sao unicos durante a execucao; IDs removidos nao sao reutilizados. */
static unsigned long next_inode = 1;

int show_error(const char *message)
{
    command_failed = 1;
    printf("ERRO: %s\n", message);
    return 0;
}

/* Entradas nao sao deslocadas; indices e ponteiros permanecem estaveis. */
static FCB nodes[MAX_NODES];

FCB *parent_node(const FCB *node)
{
    return node->parent_index >= 0 ? &nodes[node->parent_index] : NULL;
}

FCB *child_at(const FCB *parent, size_t index)
{
    if (index >= MAX_NODES || !nodes[index].used ||
        nodes[index].parent_index != (int)(parent - nodes)) {
        return NULL;
    }
    return &nodes[index];
}

int has_children(const FCB *node)
{
    for (size_t i = 0; i < MAX_NODES; ++i) {
        if (child_at(node, i)) {
            return 1;
        }
    }
    return 0;
}

FCB *find_child(FCB *parent, const char *name)
{
    for (size_t i = 0; i < MAX_NODES; ++i) {
        FCB *node = child_at(parent, i);
        if (node && !strcmp(node->name, name)) {
            return node;
        }
    }
    return NULL;
}

void attach_node(FCB *node, FCB *parent)
{
    node->parent_index = parent ? (int)(parent - nodes) : -1;
    if (parent) {
        parent->modified = time(NULL);
    }
}

FCB *create_node(FCB *parent, const char *name, FileType type, unsigned mode)
{
    FCB *node = NULL;
    for (size_t i = 0; i < MAX_NODES; ++i) {
        if (!nodes[i].used) {
            node = &nodes[i];
            break;
        }
    }
    if (!node) {
        show_error("tabela de arquivos cheia");
        return NULL;
    }
    memset(node, 0, sizeof *node);
    node->used = 1;
    strcpy(node->name, name);
    node->inode = next_inode++;
    node->type = type;
    node->mode = mode;
    node->uid = uid;
    node->gid = gid;
    node->created = node->modified = node->accessed = time(NULL);
    attach_node(node, parent);
    return node;
}

void detach_node(FCB *node)
{
    FCB *parent = parent_node(node);
    if (parent) {
        parent->modified = time(NULL);
    }
    node->parent_index = -1;
}

void destroy_tree(FCB *node)
{
    for (size_t i = 0; i < MAX_NODES; ++i) {
        FCB *child = child_at(node, i);
        if (child) {
            destroy_tree(child);
        }
    }
    release_blocks(node);
    memset(node, 0, sizeof *node);
    node->parent_index = -1;
}
