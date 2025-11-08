
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define ARRAY_SIZE 10000      // use 1000000 no teste final
#define LIMIT 10           // limite para conquista (ordenação local)

// ===================== Bubble Sort =====================
void bubble_sort(int n, int *vetor)
{
    int c=0, d, troca, trocou=1;
    while (c < (n-1) && trocou) {
        trocou = 0;
        for (d=0; d < n-c-1; d++) {
            if (vetor[d] > vetor[d+1]) {
                troca = vetor[d];
                vetor[d] = vetor[d+1];
                vetor[d+1] = troca;
                trocou = 1;
            }
        }
        c++;
    }
}

// ===================== Interleaving =====================
int *interleaving(int vetor[], int tam)
{
    int *vetor_aux = malloc(sizeof(int) * tam);
    int i1 = 0, i2 = tam / 2, i_aux;

    for (i_aux = 0; i_aux < tam; i_aux++) {
        if (((i1 < tam/2) && (vetor[i1] <= vetor[i2])) || (i2 >= tam))
            vetor_aux[i_aux] = vetor[i1++];
        else
            vetor_aux[i_aux] = vetor[i2++];
    }
    return vetor_aux;
}

// ===================== Divisão e Conquista =====================
void divide_and_conquer(int *vetor, int tam, int my_rank, int num_procs)
{
    int left = 2 * my_rank + 1;
    int right = 2 * my_rank + 2;
    MPI_Status status;

    // Condição de conquista
    if (tam <= LIMIT || left >= num_procs) {
        bubble_sort(tam, vetor);
        return;
    }

    int metade = tam / 2;

    // Envia as metades para os filhos, se existirem
    if (left < num_procs)
        MPI_Send(&vetor[0], metade, MPI_INT, left, 0, MPI_COMM_WORLD);
    if (right < num_procs)
        MPI_Send(&vetor[metade], tam - metade, MPI_INT, right, 0, MPI_COMM_WORLD);

    // Recebe as partes ordenadas dos filhos
    if (left < num_procs)
        MPI_Recv(&vetor[0], metade, MPI_INT, left, 0, MPI_COMM_WORLD, &status);
    if (right < num_procs)
        MPI_Recv(&vetor[metade], tam - metade, MPI_INT, right, 0, MPI_COMM_WORLD, &status);

    // Intercala
    int *ordenado = interleaving(vetor, tam);
    for (int i=0; i<tam; i++) vetor[i] = ordenado[i];
    free(ordenado);
}

// ===================== Main =====================
int main(int argc, char *argv[])
{
    int my_rank, num_procs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Status status;

    int *vetor = NULL;
    int tam;

    double start = MPI_Wtime();

    // Processo raiz inicializa o vetor
    if (my_rank == 0) {
        tam = ARRAY_SIZE;
        vetor = malloc(sizeof(int) * tam);
        for (int i=0; i<tam; i++)
            vetor[i] = ARRAY_SIZE - i;

        // Envia as partes iniciais (recursão começa no rank 0)
        divide_and_conquer(vetor, tam, my_rank, num_procs);

        double end = MPI_Wtime();
        printf("\n[MASTER] Tempo total: %.4fs\n", end - start);

    } else {
        // Recebe vetor e tamanho do pai
        MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_INT, &tam);
        vetor = malloc(sizeof(int) * tam);
        MPI_Recv(vetor, tam, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        divide_and_conquer(vetor, tam, my_rank, num_procs);

        // Envia de volta para o pai
        MPI_Send(vetor, tam, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
    }

    free(vetor);
    MPI_Finalize();
    return 0;
}

