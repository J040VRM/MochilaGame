#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_ITENS 100
#define MAX_FASES 10

typedef struct {
    char nome[100];
    float peso;
    float valor;
    char tipo[20];
} Item;

typedef struct {
    char nome[100];
    float capacidade;
    char regra[50];
    Item itens[MAX_ITENS];
    int total_itens;
} Fase;

typedef struct {
    int inteiro;         // 1 = inteiro, 0 = fracionado
    float peso_pego;
    float valor_pego;
    Item item;
} Escolha;

void le_entrada(const char *nome_arquivo, Fase fases[], int *num_fases) {
    FILE *arq = fopen(nome_arquivo, "r");
    if (!arq) {
        printf("Erro ao abrir o arquivo de entrada.\n");
        exit(1);
    }

    char linha[256];
    Fase fase_atual;
    int lendo_primeira = 1;
    *num_fases = 0;

    while (fgets(linha, sizeof(linha), arq)) {
        if (strncmp(linha, "FASE:", 5) == 0) {
            if (!lendo_primeira) {
                fases[*num_fases] = fase_atual;
                (*num_fases)++;
            }
            lendo_primeira = 0;
            fase_atual.total_itens = 0;
            sscanf(linha, "FASE: %[^\n]", fase_atual.nome);
        }
        else if (strncmp(linha, "CAPACIDADE:", 11) == 0) {
            sscanf(linha, "CAPACIDADE: %f", &fase_atual.capacidade);
        }
        else if (strncmp(linha, "REGRA:", 6) == 0) {
            sscanf(linha, "REGRA: %[^\n]", fase_atual.regra);
        }
        else if (strncmp(linha, "ITEM:", 5) == 0) {
            Item item;
            sscanf(linha, "ITEM: %[^,], %f, %f, %s",
                   item.nome, &item.peso, &item.valor, item.tipo);
            fase_atual.itens[fase_atual.total_itens++] = item;
        }
    }

    // salvar a última fase lida
    fases[*num_fases] = fase_atual;
    (*num_fases)++;

    fclose(arq);
}


void aplicar_regra(Fase *fase) {
    for (int i = 0; i < fase->total_itens; i++) {
        Item *it = &fase->itens[i];

        if (strcmp(fase->regra, "MAGICOS_VALOR_DOBRADO") == 0 && strcmp(it->tipo, "magico") == 0)
            it->valor *= 2;
        else if (strcmp(fase->regra, "SOBREVIVENCIA_DESVALORIZADA") == 0 && strcmp(it->tipo, "sobrevivencia") == 0)
            it->valor *= 0.8;
    }
}

int comparar(const void *a, const void *b) {
    Item *i1 = (Item *)a;
    Item *i2 = (Item *)b;
    float vp1 = i1->valor / i1->peso;
    float vp2 = i2->valor / i2->peso;
    return (vp2 > vp1) - (vp2 < vp1);
}

void resolve_fase(Fase fase, Escolha escolhidos[], int *num_escolhidos, float *lucro) {
    aplicar_regra(&fase);

    *num_escolhidos = 0;
    *lucro = 0.0;
    float capacidade_restante = fase.capacidade;

    if (strcmp(fase.regra, "TRES_MELHORES_VALOR_PESO") == 0) {
        qsort(fase.itens, fase.total_itens, sizeof(Item), comparar);
        int top = fase.total_itens < 3 ? fase.total_itens : 3;

        for (int i = 0; i < top && capacidade_restante > 0; i++) {
            Item it = fase.itens[i];
            float peso_pegavel = fmin(it.peso, capacidade_restante);

            Escolha e;
            e.item = it;
            e.peso_pego = peso_pegavel;
            e.valor_pego = (peso_pegavel / it.peso) * it.valor;
            e.inteiro = (peso_pegavel == it.peso);

            escolhidos[(*num_escolhidos)++] = e;
            *lucro += e.valor_pego;
            capacidade_restante -= peso_pegavel;
        }
        return;
    }

    qsort(fase.itens, fase.total_itens, sizeof(Item), comparar);

    for (int i = 0; i < fase.total_itens && capacidade_restante > 0; i++) {
        Item it = fase.itens[i];
        float peso_pegavel = fmin(it.peso, capacidade_restante);

        int fracionar = 1;

        if (strcmp(fase.regra, "TECNOLOGICOS_INTEIROS") == 0 && strcmp(it.tipo, "tecnologico") == 0) {
            if (it.peso <= capacidade_restante) {
                fracionar = 0;
                peso_pegavel = it.peso;
            } else {
                continue;
            }
        }

        Escolha e;
        e.item = it;
        e.peso_pego = peso_pegavel;
        e.valor_pego = (peso_pegavel / it.peso) * it.valor;
        e.inteiro = (peso_pegavel == it.peso);

        escolhidos[(*num_escolhidos)++] = e;
        *lucro += e.valor_pego;
        capacidade_restante -= peso_pegavel;
    }
}

void imprimir_saida(Fase fases[], Escolha todas_escolhas[][MAX_ITENS], int qtd_escolhas[], float lucros[], int num_fases) {
    float lucro_total = 0;

    for (int i = 0; i < num_fases; i++) {
        printf("--- FASE: %s ---\n", fases[i].nome);
        printf("Capacidade da mochila: %.2f kg\n", fases[i].capacidade);

        if (strcmp(fases[i].regra, "MAGICOS_VALOR_DOBRADO") == 0)
            printf("Regra aplicada: Itens mágicos com valor dobrado\n");
        else if (strcmp(fases[i].regra, "TECNOLOGICOS_INTEIROS") == 0)
            printf("Regra aplicada: Itens tecnológicos não podem ser fracionados\n");
        else if (strcmp(fases[i].regra, "SOBREVIVENCIA_DESVALORIZADA") == 0)
            printf("Regra aplicada: Itens de sobrevivência perdem 20%% do valor\n");
        else if (strcmp(fases[i].regra, "TRES_MELHORES_VALOR_PESO") == 0)
            printf("Regra aplicada: Apenas os três itens com maior valor/peso podem ser escolhidos\n");
        else
            printf("Regra aplicada: %s\n", fases[i].regra);

        for (int j = 0; j < qtd_escolhas[i]; j++) {
            Escolha e = todas_escolhas[i][j];
            printf("Pegou (%s) %s (%.2fkg, R$ %.2f)\n",
                e.inteiro ? "inteiro" : "fracionado",
                e.item.nome,
                e.peso_pego,
                e.valor_pego);
        }

        printf("Lucro da fase: R$ %.2f\n\n", lucros[i]);
        lucro_total += lucros[i];
    }

    printf("Lucro total acumulado: R$ %.2f\n", lucro_total);
}

int main() {
    Fase fases[MAX_FASES];
    int num_fases = 0;

    le_entrada("entrada_jogo.txt", fases, &num_fases);

    Escolha todas_escolhas[MAX_FASES][MAX_ITENS];
    int qtd_escolhas[MAX_FASES];
    float lucros[MAX_FASES];

    for (int i = 0; i < num_fases; i++) {
        resolve_fase(fases[i], todas_escolhas[i], &qtd_escolhas[i], &lucros[i]);
    }

    imprimir_saida(fases, todas_escolhas, qtd_escolhas, lucros, num_fases);

    return 0;
}
