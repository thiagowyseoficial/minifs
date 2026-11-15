/* Validacao de nomes e resolucao de caminhos na arvore.
 * Alunos: Thiago Wyse e Matheus Souza.
 */
#include "minifs.h"
#include <string.h>
#include <ctype.h>

static int is_valid_name(const char *s)
{
    const unsigned char *cursor = (const unsigned char *)s;
    if (!*s || strlen(s) >= NAME_SIZE || !strcmp(s, ".") || !strcmp(s, "..")) {
        return 0;
    }
    for (; *cursor; ++cursor) {
        if (*cursor == '/' || iscntrl(*cursor)) {
            return 0;
        }
    }
    return 1;
}

FCB *resolve_path(const char *path)
{
    FCB *node = path[0] == '/' ? root : current_directory;
    const char *cursor = path;
    if (!*cursor) {
        show_error("caminho vazio");
        return NULL;
    }
    while (*cursor) {
        char part[NAME_SIZE];
        size_t length = 0;
        while (*cursor == '/') {
            ++cursor;
        }
        if (!*cursor) {
            if (node->type != DIRECTORY) {
                show_error("nao e diretorio");
                return NULL;
            }
            break;
        }
        while (*cursor && *cursor != '/') {
            if (length + 1 >= sizeof part) {
                show_error("nome muito longo");
                return NULL;
            }
            part[length++] = *cursor++;
        }
        part[length] = 0;
        if (node->type != DIRECTORY) {
            show_error("nao e diretorio");
            return NULL;
        }
        if (!check_permission(node, X)) {
            return NULL;
        }
        if (!strcmp(part, ".")) {
            continue;
        }
        if (!strcmp(part, "..")) {
            node = parent_node(node) ? parent_node(node) : node;
        } else {
            node = find_child(node, part);
            if (!node) {
                show_error("arquivo ou diretorio nao encontrado");
                return NULL;
            }
        }
    }
    return node;
}

FCB *resolve_parent(const char *path, char name[NAME_SIZE], unsigned required_permissions)
{
    char buffer[LINE_SIZE], *slash;
    size_t length = strlen(path);
    FCB *parent;
    if (!length || length >= sizeof buffer || path[length - 1] == '/') {
        show_error("caminho de destino invalido");
        return NULL;
    }
    memcpy(buffer, path, length + 1);
    slash = strrchr(buffer, '/');
    if (!is_valid_name(slash ? slash + 1 : buffer)) {
        show_error("nome invalido");
        return NULL;
    }
    strcpy(name, slash ? slash + 1 : buffer);
    if (!slash) {
        parent = current_directory;
    } else if (slash == buffer) {
        parent = root;
    } else {
        *slash = 0;
        parent = resolve_path(buffer);
    }
    if (!parent) {
        return NULL;
    }
    if (parent->type != DIRECTORY) {
        show_error("destino nao e diretorio");
        return NULL;
    }
    if (!check_permission(parent, required_permissions)) {
        return NULL;
    }
    return parent;
}

/* Distingue um nome novo de um diretorio existente: /, . e .. tambem sao aceitos. */
FCB *resolve_destination(const char *path, const char *source_name,
                         char name[NAME_SIZE])
{
    FCB *parent;
    FCB *existing_node;
    const char *last_slash = strrchr(path, '/');
    const char *last_component = last_slash ? last_slash + 1 : path;
    size_t length = strlen(path);

    if (!length) {
        show_error("caminho de destino invalido");
        return NULL;
    }

    if (path[length - 1] == '/' || !strcmp(last_component, ".") ||
        !strcmp(last_component, "..")) {
        parent = resolve_path(path);
        if (!parent) {
            return NULL;
        }
        if (parent->type != DIRECTORY) {
            show_error("destino nao e diretorio");
            return NULL;
        }
        strcpy(name, source_name);
    } else {
        /* Apenas atravessamos o pai; ainda nao sabemos onde sera criada a entrada. */
        parent = resolve_parent(path, name, X);
        if (!parent) {
            return NULL;
        }
        existing_node = find_child(parent, name);
        if (existing_node && existing_node->type == DIRECTORY) {
            parent = existing_node;
            strcpy(name, source_name);
        }
    }

    /* Escrita e exigida somente no diretorio que recebera a nova entrada. */
    if (!check_permission(parent, W | X)) {
        return NULL;
    }
    if (find_child(parent, name)) {
        show_error("destino ja existe");
        return NULL;
    }
    return parent;
}

