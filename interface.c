/* Leitura dos argumentos e apresentacao dos metadados.
 * Alunos: Thiago Wyse e Matheus Souza.
 */
#include "sistema_arquivos.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

static void format_permissions(unsigned mode, char out[10])
{
    int i;
    for (i = 0; i < 9; ++i) {
        out[i] = mode & (1u << (8 - i)) ? "rwx"[i % 3] : '-';
    }
    out[9] = 0;
}

void print_node(const FCB *node)
{
    char permission_text[10];
    format_permissions(node->mode, permission_text);
    printf("%c%s %03o %u:%u %5lu inode=%lu %s\n", node->type == DIRECTORY ? 'd' : '-',
           permission_text, node->mode, node->uid, node->gid, (unsigned long)node->size,
           node->inode, node->name);
}

void print_path(FCB *node)
{
    if (!parent_node(node)) {
        return;
    }
    print_path(parent_node(node));
    printf("/%s", node->name);
}

int parse_number(const char *s, unsigned *value, int base, unsigned maximum)
{
    char *end;
    unsigned long result;
    if (!*s || *s == '-' || *s == '+') {
        return show_error("numero invalido");
    }
    errno = 0;
    result = strtoul(s, &end, base);
    if (errno || *end || result > maximum) {
        return show_error("numero invalido");
    }
    *value = (unsigned)result;
    return 1;
}

int parse_command_line(char *line, char **args)
{
    char *source = line, *destination = line;
    int count = 0;
    while (*source) {
        char quote = 0;
        while (isspace((unsigned char)*source)) {
            ++source;
        }
        if (!*source) {
            break;
        }
        if (count == MAX_ARGS) {
            show_error("argumentos demais");
            return -1;
        }
        args[count++] = destination;
        while (*source && (quote || !isspace((unsigned char)*source))) {
            char c = *source++;
            if (c == '\\') {
                if (!*source) {
                    show_error("escape incompleto");
                    return -1;
                }
                *destination++ = *source++;
            } else if (c == '\'' || c == '"') {
                if (!quote) {
                    quote = c;
                } else if (quote == c) {
                    quote = 0;
                } else {
                    *destination++ = c;
                }
            } else {
                *destination++ = c;
            }
        }
        if (quote) {
            show_error("aspas nao fechadas");
            return -1;
        }
        while (isspace((unsigned char)*source)) {
            ++source;
        }
        *destination++ = 0;
    }
    return count;
}

void print_help(void)
{
    puts("mkdir CAMINHO | cd CAMINHO | pwd | ls [CAMINHO]");
    puts("touch CAMINHO [caractere|numerico|binario|programa]");
    puts("write ARQUIVO TEXTO | echo TEXTO > ARQUIVO | cat ARQUIVO");
    puts("cp ORIGEM DESTINO | mv ORIGEM DESTINO | rm ARQUIVO | rmdir DIRETORIO");
    puts("chmod MODO_OCTAL CAMINHO | stat CAMINHO | blocks ARQUIVO | df");
    puts("su UID GID | whoami | exec ARQUIVO | help | exit");
}

