/* Selecao da classe de usuario e verificacao dos bits RWX.
 * Alunos: Thiago Wyse e Matheus Souza.
 */
#include "sistema_arquivos.h"

/* Seleciona somente uma classe: proprietario, grupo ou outros. */
static int has_permission(const FCB *node, unsigned bits)
{
    unsigned shift;
    unsigned class_permissions;

    if (uid == node->uid) {
        shift = 6; /* Bits do proprietario. */
    } else if (gid == node->gid) {
        shift = 3; /* Bits do grupo. */
    } else {
        shift = 0; /* Bits dos outros usuarios. */
    }

    class_permissions = (node->mode >> shift) & 7;
    return (class_permissions & bits) == bits;
}

int check_permission(const FCB *node, unsigned bits)
{
    if (!has_permission(node, bits)) {
        return show_error("permissao negada");
    }
    return 1;
}

