/* Inicializacao, leitura de linhas e encerramento.
 * Alunos: Thiago Wyse e Matheus Souza.
 */
#include "minifs.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    char line[LINE_SIZE], *args[MAX_ARGS];
    int count, batch = 0;
    if (argc == 2 && !strcmp(argv[1], "--batch")) {
        batch = 1;
    } else if (argc != 1) {
        fputs("Uso: minifs [--batch]\n", stderr);
        return EXIT_FAILURE;
    }
    root = create_node(NULL, "", DIRECTORY, 0777);
    if (!root) {
        return EXIT_FAILURE;
    }
    current_directory = root;
    if (!batch) {
        puts("MiniFS em memoria. Digite help para ajuda; exit para sair.");
    }
    for (;;) {
        if (!batch) {
            fputs("minifs> ", stdout);
            fflush(stdout);
        }
        if (!fgets(line, sizeof line, stdin)) {
            break;
        }
        if (!strchr(line, '\n') && !feof(stdin)) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {
            }
            show_error("linha muito longa");
            continue;
        }
        count = parse_command_line(line, args);
        if (count <= 0) {
            continue;
        }
        if (!strcmp(args[0], "exit") && count == 1) {
            break;
        }
        execute_command(count, args);
    }
    destroy_tree(root);
    return batch && command_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

