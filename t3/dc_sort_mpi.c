#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define ARRAY_SIZE 40

// ===================== Bubble Sort =====================
void bubble_sort(int n, int *vetor) {
    int c = 0, d, troca, trocou = 1;
    while (c < (n - 1) && trocou) {
        trocou = 0;
        for (d = 0; d < n - c - 1; d++) {
            if (vetor[d] > vetor[d + 1]) {
                troca = vetor[d];
                vetor[d] = vetor[d + 1];
                vetor[d + 1] = troca;
                trocou = 1;
            }
        }
        c++;
    }
}

// ===================== Interleaving =====================
int *merge(int *a, int sizeA, int *b, int sizeB) {
    int *res = malloc((sizeA + sizeB) * sizeof(int));
    int i = 0, j = 0, k = 0;
    while (i < sizeA && j < sizeB)
        res[k++] = (a[i] < b[j]) ? a[i++] : b[j++];
    while (i < sizeA) res[k++] = a[i++];
    while (j < sizeB) res[k++] = b[j++];
    return res;
}

// ===================== Main =====================
int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 4) {
        if (rank == 0)
            printf("Este programa deve ser executado com 4 processos.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int n = ARRAY_SIZE;
    int part = n / size;
    MPI_Status status;

    if (rank == 0) {
        int *vetor = malloc(n * sizeof(int));
        for (int i = 0; i < n; i++) vetor[i] = n - i;

        double start = MPI_Wtime();

        // Envia 3 partes
        for (int p = 1; p < size; p++)
            MPI_Send(vetor + p * part, part, MPI_INT, p, 0, MPI_COMM_WORLD);

        // Ordena a própria parte
        bubble_sort(part, vetor);

        // Recebe partes ordenadas
        int *sorted_all = malloc(n * sizeof(int));
        for (int i = 0; i < part; i++)
            sorted_all[i] = vetor[i];

        for (int p = 1; p < size; p++) {
            MPI_Recv(sorted_all + p * part, part, MPI_INT, p, 1, MPI_COMM_WORLD, &status);
        }

        // Junta tudo
        int *temp1 = merge(sorted_all, part * 2, sorted_all + part * 2, part * 2);
        int *final = merge(temp1, n / 2, sorted_all + n / 2, n / 2);

        double end = MPI_Wtime();
        printf("\n[MASTER] Tempo total: %.4fs\n", end - start);

        printf("[MASTER] Vetor final ordenado:\n");
        for (int i = 0; i < n; i++) printf("%d ", final[i]);
        printf("\n");

        free(temp1);
        free(final);
        free(vetor);
        free(sorted_all);
    } 
    else {
        int *subvetor = malloc(part * sizeof(int));
        MPI_Recv(subvetor, part, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        bubble_sort(part, subvetor);
        MPI_Send(subvetor, part, MPI_INT, 0, 1, MPI_COMM_WORLD);
        free(subvetor);
    }

    MPI_Finalize();
    return 0;
}

