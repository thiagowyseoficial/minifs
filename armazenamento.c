/* Disco simulado, mapa de ocupacao e alocacao indexada.
 * Alunos: Thiago Wyse e Matheus Souza.
 */
#include "minifs.h"
#include <string.h>

/* Apenas este modulo acessa diretamente os blocos fisicos. */
static unsigned char disk[BLOCK_COUNT][BLOCK_SIZE];
static unsigned char block_used[BLOCK_COUNT];

void release_blocks(FCB *node)
{
    size_t i;
    for (i = 0; i < node->block_count; ++i) {
        block_used[node->blocks[i]] = 0;
        memset(disk[node->blocks[i]], 0, BLOCK_SIZE);
    }
    node->block_count = node->size = 0;
}

/* Confere capacidade antes de substituir dados para preservar o arquivo em falhas. */
int write_data(FCB *node, const unsigned char *data, size_t size)
{
    size_t needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE, available = node->block_count, i;
    for (i = 0; i < BLOCK_COUNT; ++i) {
        if (!block_used[i]) {
            ++available;
        }
    }
    if (needed > MAX_BLOCKS) {
        return show_error("limite de 4096 bytes por arquivo excedido");
    }
    if (needed > available) {
        return show_error("disco simulado cheio");
    }
    release_blocks(node);
    for (i = 0; i < BLOCK_COUNT && node->block_count < needed; ++i) {
        if (!block_used[i]) {
            block_used[i] = 1;
            node->blocks[node->block_count++] = (int)i;
        }
    }
    for (i = 0; i < size; ++i) {
        disk[node->blocks[i / BLOCK_SIZE]][i % BLOCK_SIZE] = data[i];
    }
    node->size = size;
    node->modified = time(NULL);
    return 1;
}

void read_data(const FCB *node, unsigned char *data)
{
    size_t i;
    for (i = 0; i < node->size; ++i) {
        data[i] = disk[node->blocks[i / BLOCK_SIZE]][i % BLOCK_SIZE];
    }
}

/* Quantidade de blocos ocupados, independentemente da arvore de nomes. */
void print_disk_usage(void)
{
    size_t used_count = 0;
    for (size_t i = 0; i < BLOCK_COUNT; ++i) {
        if (block_used[i]) {
            ++used_count;
        }
    }
    printf("blocos: %lu/%d usados; livres=%lu; bloco=%d bytes\n",
           (unsigned long)used_count, BLOCK_COUNT,
           (unsigned long)(BLOCK_COUNT - used_count), BLOCK_SIZE);
}
