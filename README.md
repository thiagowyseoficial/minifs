# MiniFS — sistema de arquivos em memória em C

**Alunos:** Thiago Wyse e Matheus Souza  
**Disciplina:** Sistemas Operacionais — UNIVALI  
**Professor:** Michael D C Alves

Implementação do trabalho de Sistemas Operacionais da UNIVALI. Todas as operações sobre arquivos e
diretórios são realizadas em estruturas na memória do processo. Ao sair, os dados
são descartados. O programa não executa comandos do sistema hospedeiro.

## Enunciado

[Enunciado do trabalho — 2026/2](T3_SistemasOperacionais_2026_2.pdf)

## Compilar e executar

Requisitos: GCC com suporte a C11. No Linux, WSL ou terminal com GCC:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 principal.c sistema_arquivos.c permissoes.c caminhos.c armazenamento.c interface.c comandos.c -o minifs
./minifs
```

No Windows/PowerShell com MinGW:

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 principal.c sistema_arquivos.c permissoes.c caminhos.c armazenamento.c interface.c comandos.c -o minifs.exe
.\minifs.exe
```

Se houver Make instalado, `make` compila e `make clean` remove o executável.
Use `help` dentro do simulador para listar os comandos disponíveis.

O modo `--batch` remove a saudação e os prompts. Para processar comandos de um
arquivo de texto criado pelo usuário, no Linux:

```sh
./minifs --batch < comandos.txt
```

O arquivo `comandos.txt` deve conter um comando por linha. No modo batch, o processo
retorna 1 se algum comando falhar e 0 se todos forem bem-sucedidos, continuando a
leitura após erros. Esse arquivo de entrada não é necessário para o modo interativo.

## Exemplo de uso

```text
mkdir documentos
cd documentos
touch notas caractere
write notas "Conteudo do trabalho"
cat notas
cp notas copia
mv copia revisao
ls
stat notas
blocks notas
chmod 640 notas
su 2000 1000
cat notas
write notas "negado: grupo nao tem escrita"
su 2000 2000
cat notas
su 1000 1000
rm revisao
rm notas
cd /
rmdir documentos
df
exit
```

O grupo pode ler `notas`, mas não escrever. Outros não podem ler. O dono recupera
seu acesso ao usar `su 1000 1000`. Não há necessidade de autenticação, pois `su`
é uma ferramenta didática para trocar as variáveis de identidade.

## Comandos e relação com Linux

| Comando | Operação e comparação |
|---|---|
| `mkdir CAMINHO` | Cria diretório, como `mkdir` sem `-p`. |
| `cd CAMINHO`, `pwd` | Navega na árvore e exibe o caminho absoluto. |
| `ls [CAMINHO]` | Exibe permissões, modo octal, UID:GID, tamanho, inode e nome; semelhante a `ls -li`. |
| `touch CAMINHO [TIPO]` | Cria arquivo vazio; acusa duplicidade em vez de atualizar datas como o `touch` Linux. |
| `write ARQUIVO "TEXTO"` | Substitui o conteúdo completo; o arquivo deve existir. |
| `echo "TEXTO" > ARQUIVO` | Mesma operação de `write`; exige arquivo existente e não acrescenta quebra de linha. |
| `cat ARQUIVO` | Lê os bytes e acrescenta uma quebra de linha para apresentação. |
| `cp ORIGEM DESTINO` | Copia arquivo e conteúdo em blocos independentes; cria outro inode. |
| `mv ORIGEM DESTINO` | Move/renomeia arquivo ou diretório, preservando inode e conteúdo. |
| `rm ARQUIVO` | Remove arquivo e devolve seus blocos ao disco simulado. |
| `rmdir DIRETORIO` | Remove somente diretório vazio e fora do caminho atual. |
| `chmod OCTAL CAMINHO` | Define os nove bits RWX; somente o proprietário pode alterar. |
| `stat CAMINHO` | Exibe o FCB e os três timestamps em segundos desde a época Unix. |
| `blocks ARQUIVO` | Mostra os índices físicos dos blocos do arquivo. |
| `df` | Mostra capacidade, ocupação e tamanho de bloco. |
| `su UID GID`, `whoami` | Troca/exibe identidade numérica da simulação (0–65535). |
| `exec ARQUIVO` | Verifica tipo programa e bit X; informa autorização, sem interpretar conteúdo. |
| `help`, `exit` | Ajuda e encerramento. EOF também encerra. |

Remoção e movimentação rejeitam caminhos cujo componente final seja `.` ou `..`,
mesmo com barras finais, para evitar alterar o diretório indicado por uma expressão
de navegação. Esses componentes continuam disponíveis em `cd`, na resolução de
caminhos e como diretórios de destino de cópia/movimentação.

`cp` e `mv` aceitam um diretório existente como destino. Colisões de nome são
rejeitadas; não há sobrescrita implícita. Caminhos absolutos e relativos aceitam
`.` e `..`, inclusive como destinos, assim como a raiz `/` e diretórios terminados
em `/`. A permissão de escrita é exigida no diretório que receberá a entrada;
nos ancestrais, basta a permissão de travessia. Aspas simples/duplas permitem
espaços nos nomes e conteúdo; barra
invertida escapa o próximo caractere. Não há expansão de variáveis, curingas,
pipelines ou comandos do shell. Para nomes com barra invertida, duplique-a.

## Estruturas e conceitos teóricos

### Organização dos arquivos

| Arquivo | Responsabilidade |
|---|---|
| `sistema_arquivos.h` | Constantes, tipos, FCB e declarações das funções compartilhadas. |
| `principal.c` | Inicializa a raiz, lê comandos e libera a memória ao encerrar. |
| `sistema_arquivos.c` | Mantém o estado da simulação e cria, busca, desliga e libera entradas do array. |
| `permissoes.c` | Seleciona proprietário/grupo/outros e verifica os bits RWX. |
| `caminhos.c` | Valida nomes e resolve caminhos e destinos de cópia/movimentação. |
| `armazenamento.c` | Mantém o disco e o mapa de ocupação privados e lê/escreve os blocos. |
| `interface.c` | Separa argumentos, interpreta números, exibe metadados e ajuda. |
| `comandos.c` | Valida argumentos e encaminha cada operação para uma função própria, como `command_cat`, `command_remove` e `command_change_directory`. |
| `Makefile` | Compila todos os módulos e oferece o alvo `clean`. |

O cabeçalho usa uma guarda de inclusão para evitar declarações duplicadas.
As variáveis compartilhadas são declaradas com `extern` em `sistema_arquivos.h` e definidas
uma única vez em `sistema_arquivos.c`. Funções auxiliares usadas somente dentro de um módulo
continuam `static`. O disco e seu mapa de ocupação ficam privados em
`armazenamento.c`; os outros módulos acessam o conteúdo pelas funções de leitura
e escrita. Todos os arquivos C devem ser compilados juntos.

### Arquivo, FCB e inode

Cada `FCB` armazena nome, inode único e crescente, tipo, tamanho em bytes,
UID/GID, permissões, datas de criação, modificação e acesso, índices dos blocos,
indicador de ocupação e índice do pai. Os tipos são `caractere` (padrão),
`numerico`, `binario` e `programa`. São classificações de metadados: a CLI insere
texto, sem validar números ou receber bytes NUL. `exec` simula a verificação de
execução, sem interpretar conteúdo ou iniciar programas externos.

No Unix, o nome fica na entrada de diretório e o inode é separado; aqui ficam
reunidos para simplificar. Copiar cria novo inode, proprietário atual, mesmas
permissões e novas datas. Mover preserva o inode. IDs removidos não são reutilizados.

### Diretórios em array

Os arquivos e diretórios ficam em `static FCB nodes[MAX_NODES]`, com 256 posições,
incluindo a raiz. Cada entrada tem `used` (ocupação) e `parent_index` (índice do
pai). A raiz usa -1. A hierarquia continua sendo uma árvore lógica, representada
por índices no array, sem listas encadeadas ou alocação dinâmica de FCBs.

Buscar um filho ou listar um diretório percorre a tabela, selecionando entradas
ocupadas pelo índice do pai. A busca custa O(N), sendo N a capacidade da tabela.
Criar usa a primeira posição livre; remover limpa a entrada para reutilização,
sem deslocar as demais. Inodes continuam crescentes e não são reutilizados.
O limite de 256 entradas inclui arquivos e diretórios; criar além dele gera erro.

Mover altera o índice do pai e o nome, preservando inode, conteúdo e descendentes.
A movimentação impede ciclos e a remoção protege o diretório atual e seus pais.
Ponteiros locais apenas acessam posições estáveis do array. `destroy_tree` limpa
as entradas de uma subárvore e libera seus blocos de dados.

### Proteção RWX obrigatória na simulação

Os nove bits de `mode` são separados em proprietário, grupo e outros. A seleção
é exclusiva: se o UID corresponde ao proprietário, usam-se apenas os bits do dono;
caso contrário, verifica-se GID; por fim, outros. A operação só ocorre quando
`(permissoes_da_classe & bits_exigidos) == bits_exigidos`.

`chmod 640` corresponde a `rw-r-----`. A conversão usa base 8 e aceita somente
valores entre 000 e 777. Não existe exceção para UID 0, ACL, grupos suplementares,
sticky bit ou setuid. A raiz nasce com 777 para facilitar experiências com usuários
diferentes; diretórios novos usam 755 e arquivos 644. O dono inicial é 1000:1000.

Leitura requer R, escrita requer W e execução simulada requer X. Em diretórios,
X permite atravessar/buscar nomes, R permite listar (a listagem detalhada também
exige X) e W+X permite criar/remover/renomear entradas. Apagar depende do diretório
pai, assim como no Unix: um arquivo sem W ainda pode ser removido por quem tem
W+X no pai. `stat` consulta metadados após a verificação de travessia do caminho,
sem exigir leitura do conteúdo. `chmod` exige propriedade e travessia do caminho.

### Alocação indexada e disco simulado

`disk[256][64]` representa 16 KiB de armazenamento de dados. `block_used[256]` é o mapa
de ocupação, e `FCB.blocks[64]` é o índice de blocos físicos de cada arquivo.
O limite por arquivo é 4096 bytes; diretórios e metadados ficam fora dessa cota.
Um arquivo de 65 bytes ocupa dois blocos: o byte de posição 64 está em
`disk[blocks[1]][0]`. O último bloco pode ter fragmentação interna: em um arquivo
de 65 bytes, 63 bytes do segundo bloco ficam sem uso. Os índices permitem usar
blocos fisicamente separados, evitando a exigência de espaço livre contíguo.

O alocador escolhe os primeiros blocos livres, que podem ser não contíguos.
Ao contrário da alocação contínua, não precisa de uma região livre consecutiva;
ao contrário da encadeada, não precisa percorrer os blocos anteriores para acessar
um byte. O índice tem tamanho fixo e não há blocos indiretos. `blocks` demonstra o
mapeamento e `df` demonstra consumo/liberação. A escrita verifica capacidade antes
de liberar os dados antigos, preservando o arquivo se faltar espaço. Cópia que
falha por falta de espaço remove o nó provisório.

Datas de acesso mudam em leitura, listagem, navegação e execução autorizada;
datas de modificação mudam em escrita e alteração de entradas do diretório.
A resolução temporal é de um segundo. Não há persistência, concorrência, links,
journaling ou garantia de reproduzir todos os detalhes de POSIX.

## Validação realizada

A versão com array foi compilada no Windows com `zig cc`, usando C11 e
`-Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes`,
sem avisos.

Passaram sete cenários funcionais: criação/leitura/escrita/cópia/movimentação e
remoção; permissões; proteção contra ciclos e remoções inválidas; limite de 256
entradas e reutilização de posições; nomes iguais em diretórios distintos;
hierarquia com 255 níveis; e recuperação após falha de cópia por disco cheio.
Os testes auxiliares e o compilador temporário não integram a entrega.

## Relação com os requisitos do trabalho

| Requisito | Implementação | Como demonstrar |
|---|---|---|
| Diretórios em árvore | Array `nodes`, `parent_index` e `resolve_path` | `mkdir`, `cd`, `pwd`, `ls` |
| Arquivos e metadados | `FCB`, `FileType` e contador `next_inode` | `touch`, `stat` |
| Operações sobre arquivos | Criação, escrita, leitura, cópia, movimentação e remoção | `write`, `cat`, `cp`, `mv`, `rm` |
| Controle de acesso | `check_permission` e `command_chmod` | `chmod 640`, troca de UID/GID e tentativas de leitura/escrita |
| Execução protegida | Verificação de tipo programa e bit X | `touch app programa`, `chmod 700 app`, `exec app` |
| Alocação de blocos | Disco em RAM, mapa de ocupação e índice no FCB | `blocks` e `df` |
| Tratamento de erros | Validações de caminho, tipo, nome, capacidade e permissões | Tentativas com nome inexistente, nome inválido e permissão insuficiente |
| Gestão de memória | Array fixo, reutilizacao de entradas e `destroy_tree` | Leitura de `destroy_tree` e conferência de `df` após remoções |
