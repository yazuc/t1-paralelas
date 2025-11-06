#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define ARRAY_SIZE 40

// ===================== Bubble Sort =====================
void bubble_sort(int n, int * vetor)
{
    int c=0, d, troca, trocou =1;

    while (c < (n-1) & trocou )
        {
        trocou = 0;
        for (d = 0 ; d < n - c - 1; d++)
            if (vetor[d] > vetor[d+1])
                {
                troca      = vetor[d];
                vetor[d]   = vetor[d+1];
                vetor[d+1] = troca;
                trocou = 1;
                }
        c++;
        }
}

// ===================== Interleaving =====================
int *interleaving(int vetor[], int tam)
{
	int *vetor_auxiliar;
	int i1, i2, i_aux;

	vetor_auxiliar = (int *)malloc(sizeof(int) * tam);

	i1 = 0;
	i2 = tam / 2;

	for (i_aux = 0; i_aux < tam; i_aux++) {
		if (((vetor[i1] <= vetor[i2]) && (i1 < (tam / 2)))
		    || (i2 == tam))
			vetor_auxiliar[i_aux] = vetor[i1++];
		else
			vetor_auxiliar[i_aux] = vetor[i2++];
	}

	return vetor_auxiliar;
}

// ===================== Main =====================
int main(int argc, char *argv[]) {
    int my_rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Status Status;

    double start = MPI_Wtime();

    int vetor[ARRAY_SIZE];

    for (int i=0 ; i<ARRAY_SIZE; i++)              /* init array with worst case for sorting */
        vetor[i] = ARRAY_SIZE-i;

    // Define o tamanho da mensagem de acordo com o processo raiz
    if (my_rank != 0) {
        MPI_Recv(vetor, ARRAY_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &Status);
        MPI_Get_count(&Status, MPI_INT, &size); // Descobrir tamanho da mensagem
    } else {
        size = ARRAY_SIZE;
    }

    // Se o vetor for pequeno o suficiente, ordena com bubble sort
    if (size <= ARRAY_SIZE) {
        bubble_sort(ARRAY_SIZE, vetor);
    } else {
        // Se o vetor for grande, divide o vetor e envia para os filhos
        int metade = size / 2;
        MPI_Send(&vetor[0], metade, MPI_INT, 1, 0, MPI_COMM_WORLD);  // Envia metade para o filho 1
        MPI_Send(&vetor[metade], metade, MPI_INT, 2, 0, MPI_COMM_WORLD);  // Envia metade para o filho 2

        // Recebe as metades ordenadas dos filhos
        MPI_Recv(vetor, metade, MPI_INT, 1, 0, MPI_COMM_WORLD, &Status);
        MPI_Recv(vetor + metade, metade, MPI_INT, 2, 0, MPI_COMM_WORLD, &Status);

        // Intercala os vetores recebidos
        interleaving(vetor, size);
    }

    // Se o processo não for o raiz, envia o vetor ordenado de volta para o raiz
    if (my_rank != 0) {
        MPI_Send(vetor, size, MPI_INT, 0, 0, MPI_COMM_WORLD);
    } else {
        double end = MPI_Wtime();
        printf("\n[MASTER] Tempo total: %.4fs\n", end - start);
    }

    MPI_Finalize();
    return 0;
}
