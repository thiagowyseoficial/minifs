/* Operacoes do usuario e selecao dos comandos.
 * Alunos: Thiago Wyse e Matheus Souza.
 */
#include "minifs.h"
#include <string.h>

/* Pontos finais identificam navegacao, nao uma entrada que possa ser removida/movida. */
static int has_special_final_component(const char *path)
{
    size_t end = strlen(path);
    size_t start;

    while (end > 0 && path[end - 1] == '/') {
        --end;
    }
    start = end;
    while (start > 0 && path[start - 1] != '/') {
        --start;
    }
    return (end - start == 1 && path[start] == '.') ||
           (end - start == 2 && path[start] == '.' && path[start + 1] == '.');
}

static void command_list(int argc, char **arguments)
{
    FCB *node, *cursor;

    node = argc == 2 ? resolve_path(arguments[1]) : current_directory;
    if (!node || !check_permission(node, R | (node->type == DIRECTORY ? X : 0))) {
        return;
    }
    node->accessed = time(NULL);
    if (node->type != DIRECTORY) {
        print_node(node);
    } else {
        for (size_t i = 0; i < MAX_NODES; ++i) {
            cursor = child_at(node, i);
            if (cursor) {
                print_node(cursor);
            }
        }
    }
    return;
}

static void command_create(int argc, char **arguments)
{
    FCB *cursor;
    char name[NAME_SIZE];
    const char *command_name = arguments[0];

    FileType type = !strcmp(command_name, "mkdir") ? DIRECTORY : CHARACTER;
    int i;
    if (argc == 3) {
        for (i = CHARACTER; i <= PROGRAM; ++i) {
            if (!strcmp(arguments[2], type_names[i])) {
                break;
            }
        }
        if (i > PROGRAM) {
            show_error("tipo invalido");
            return;
        }
        type = (FileType)i;
    }
    cursor = resolve_parent(arguments[1], name, W | X);
    if (!cursor) {
        return;
    }
    if (find_child(cursor, name)) {
        show_error("nome ja existe");
        return;
    }
    create_node(cursor, name, type, type == DIRECTORY ? 0755 : 0644);
    return;
}

static void command_copy_or_move(char **arguments)
{
    FCB *node, *cursor;
    char name[NAME_SIZE];
    const char *command_name = arguments[0];

    FCB *destination_node, *ancestor;
    if (!strcmp(command_name, "mv") && has_special_final_component(arguments[1])) {
        show_error("nao e permitido mover . ou ..");
        return;
    }
    node = resolve_path(arguments[1]);
    if (!node) {
        return;
    }
    if (node == root) {
        show_error("operacao sobre a raiz proibida");
        return;
    }
    if (!strcmp(command_name, "cp") && (node->type == DIRECTORY || !check_permission(node, R))) {
        if (node->type == DIRECTORY) {
            show_error("cp aceita somente arquivos");
        }
        return;
    }
    cursor = resolve_destination(arguments[2], node->name, name);
    if (!cursor) {
        return;
    }
    if (!strcmp(command_name, "mv")) {
        if (!check_permission(parent_node(node), W | X)) {
            return;
        }
        for (ancestor = cursor; ancestor; ancestor = parent_node(ancestor)) {
            if (ancestor == node) {
                show_error("movimento criaria ciclo");
                return;
            }
        }
        detach_node(node);
        strcpy(node->name, name);
        attach_node(node, cursor);
    } else {
        unsigned char data[MAX_BLOCKS * BLOCK_SIZE];
        destination_node = create_node(cursor, name, node->type, node->mode);
        if (!destination_node) {
            return;
        }
        read_data(node, data);
        if (!write_data(destination_node, data, node->size)) {
            detach_node(destination_node);
            destroy_tree(destination_node);
            return;
        }
        node->accessed = time(NULL);
    }
    return;
}

static void command_write(char **arguments)
{
    FCB *node;

    node = resolve_path(arguments[1]);
    if (!node || node->type == DIRECTORY) {
        if (node) {
            show_error("nao e arquivo");
        }
        return;
    }
    if (check_permission(node, W)) {
        write_data(node, (unsigned char *)arguments[2], strlen(arguments[2]));
    }
    return;
}

static void command_chmod(char **arguments)
{
    FCB *node;

    unsigned mode;
    if (!parse_number(arguments[1], &mode, 8, 0777)) {
        return;
    }
    node = resolve_path(arguments[2]);
    if (!node) {
        return;
    }
    if (uid != node->uid) {
        show_error("somente o proprietario pode alterar permissoes");
        return;
    }
    node->mode = mode;
    return;
}

static void command_change_directory(FCB *node)
{
    if (node->type != DIRECTORY) {
        show_error("nao e diretorio");
        return;
    }
    if (check_permission(node, X)) {
        current_directory = node;
        node->accessed = time(NULL);
    }
}

static void command_stat(const FCB *node)
{
    print_node(node);
    printf("tipo=%s tamanho=%lu blocos=%lu\n", type_names[node->type],
           (unsigned long)node->size, (unsigned long)node->block_count);
    printf("criacao=%lld modificacao=%lld acesso=%lld (Unix time)\n", (long long)node->created,
           (long long)node->modified, (long long)node->accessed);
}

/* Apagar depende de W+X no pai; nao depende de W no arquivo. */
static void command_remove(FCB *node, const char *path, int remove_directory)
{
    FCB *ancestor;

    if (has_special_final_component(path)) {
        show_error("nao e permitido remover . ou ..");
        return;
    }
    if (node == root) {
        show_error("operacao sobre a raiz proibida");
        return;
    }
    if ((node->type == DIRECTORY) != remove_directory) {
        show_error("tipo incorreto para remocao");
        return;
    }
    if (has_children(node)) {
        show_error("diretorio nao vazio");
        return;
    }
    for (ancestor = current_directory; ancestor; ancestor = parent_node(ancestor)) {
        if (ancestor == node) {
            show_error("diretorio em uso");
            return;
        }
    }
    if (!check_permission(parent_node(node), W | X)) {
        return;
    }
    detach_node(node);
    destroy_tree(node);
}

static void command_execute(FCB *node)
{
    if (node->type != PROGRAM) {
        show_error("arquivo nao e programa");
        return;
    }
    if (check_permission(node, X)) {
        puts("Execucao simulada autorizada (conteudo nao interpretado).");
        node->accessed = time(NULL);
    }
}

static void command_cat(FCB *node)
{
    unsigned char content[MAX_BLOCKS * BLOCK_SIZE];

    if (!check_permission(node, R)) {
        return;
    }
    read_data(node, content);
    fwrite(content, 1, node->size, stdout);
    putchar('\n');
    node->accessed = time(NULL);
}

static void command_blocks(const FCB *node)
{
    if (!check_permission(node, R)) {
        return;
    }
    printf("inode=%lu; indices:", node->inode);
    for (size_t i = 0; i < node->block_count; ++i) {
        printf(" %d", node->blocks[i]);
    }
    putchar('\n');
}

/* O despachante valida argumentos; cada operacao tem sua propria funcao. */
void execute_command(int argc, char **arguments)
{
    FCB *node = NULL;
    const char *command_name = arguments[0];
    if (!strcmp(command_name, "help") && argc == 1) {
        print_help();
        return;
    }
    if (!strcmp(command_name, "pwd") && argc == 1) {
        if (current_directory == root) {
            putchar('/');
        } else {
            print_path(current_directory);
        }
        putchar('\n');
        return;
    }
    if (!strcmp(command_name, "whoami") && argc == 1) {
        printf("uid=%u gid=%u\n", uid, gid);
        return;
    }
    if (!strcmp(command_name, "su") && argc == 3) {
        unsigned new_uid, new_gid;
        if (parse_number(arguments[1], &new_uid, 10, 65535) &&
            parse_number(arguments[2], &new_gid, 10, 65535)) {
            uid = new_uid;
            gid = new_gid;
        }
        return;
    }
    if (!strcmp(command_name, "df") && argc == 1) {
        print_disk_usage();
        return;
    }
    if (!strcmp(command_name, "ls") && argc <= 2) {
        command_list(argc, arguments);
        return;
    }
    if ((!strcmp(command_name, "mkdir") && argc == 2) ||
        (!strcmp(command_name, "touch") && (argc == 2 || argc == 3))) {
        command_create(argc, arguments);
        return;
    }
    if ((!strcmp(command_name, "cp") || !strcmp(command_name, "mv")) && argc == 3) {
        command_copy_or_move(arguments);
        return;
    }
    if (!strcmp(command_name, "echo") && argc == 4 && !strcmp(arguments[2], ">")) {
        char *w[] = {"write", arguments[3], arguments[1]};
        execute_command(3, w);
        return;
    }
    if (!strcmp(command_name, "write") && argc == 3) {
        command_write(arguments);
        return;
    }
    if (!strcmp(command_name, "chmod") && argc == 3) {
        command_chmod(arguments);
        return;
    }
    if (argc != 2 || (strcmp(command_name, "cd") && strcmp(command_name, "cat") &&
                      strcmp(command_name, "stat") && strcmp(command_name, "blocks") &&
                      strcmp(command_name, "exec") && strcmp(command_name, "rm") &&
                      strcmp(command_name, "rmdir"))) {
        show_error("comando ou argumentos invalidos; use help");
        return;
    }
    node = resolve_path(arguments[1]);
    if (!node) {
        return;
    }
    if (!strcmp(command_name, "cd")) {
        command_change_directory(node);
    } else if (!strcmp(command_name, "stat")) {
        command_stat(node);
    } else if (!strcmp(command_name, "rm") || !strcmp(command_name, "rmdir")) {
        command_remove(node, arguments[1], !strcmp(command_name, "rmdir"));
    } else {
        if (node->type == DIRECTORY) {
            show_error("nao e arquivo");
            return;
        }
        if (!strcmp(command_name, "exec")) {
            command_execute(node);
        } else if (!strcmp(command_name, "cat")) {
            command_cat(node);
        } else {
            command_blocks(node);
        }
    }
}

