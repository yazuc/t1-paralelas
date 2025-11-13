#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mpi.h"

float lerp(float from, float to, float t);
void writeToPPM(float* mesh, int iterations, const int MESHSIZE);
int getChunkRows(int rank, int commSize, const int MESHSIZE);
int getChunkSize(int rank, int commSize, const int MESHSIZE);

int main(int argc, char **argv)
{
    const int MESHSIZE = 1000;
    const float NORTH_BOUND = 100.0;
    const float SOUTH_BOUND = 100.0;
    const float EAST_BOUND  = 0.0;
    const float WEST_BOUND  = 0.0;
    const float INTERIOR_AVG =
        (NORTH_BOUND + SOUTH_BOUND + EAST_BOUND + WEST_BOUND) / 4.0;

    int rank, commSize, r, c, itrCount;
    int rFirst, rLast;
    MPI_Status status;
    float diffNorm, gDiffNorm;
    float *xLocal, *xNew, *xFull;

    float epsilon;
    int maxIterations;
    double start, stop;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &commSize);

    const int CHUNKROWS = getChunkRows(rank, commSize, MESHSIZE);
    const int CHUNKSIZE = getChunkSize(rank, commSize, MESHSIZE);

    if (argc < 3) {
        if (rank == 0) {
            printf("Uso: mpirun -np <N> ./jacobi [epsilon] [max_iterations]\n");
        }
        MPI_Finalize();
        return 0;
    }

    xLocal = (float*)malloc((CHUNKROWS + 2) * MESHSIZE * sizeof(float));
    xNew   = (float*)malloc((CHUNKROWS + 2) * MESHSIZE * sizeof(float));
    if (rank == 0) xFull = (float*)malloc(MESHSIZE * MESHSIZE * sizeof(float));

    epsilon = strtod(argv[1], NULL);
    maxIterations = strtol(argv[2], NULL, 10);

    if (rank == 0)
        printf("Jacobi (MPI) - Modelo de Fases Paralelas com Alltoall\n");

    // === GERAÇÃO LOCAL (cada processo cria sua parte) ===
    rFirst = 1;
    rLast = CHUNKROWS;
    if (rank == 0) rFirst++;
    if (rank == commSize - 1) rLast--;

    for (r = 1; r <= CHUNKROWS; r++) {
        for (c = 0; c < MESHSIZE; c++) {
            xLocal[r * MESHSIZE + c] = INTERIOR_AVG;
        }
        xLocal[r * MESHSIZE + MESHSIZE - 1] = EAST_BOUND;
        xLocal[r * MESHSIZE + 0] = WEST_BOUND;
    }

    for (c = 0; c < MESHSIZE; c++) {
        xLocal[(rFirst - 1) * MESHSIZE + c] = NORTH_BOUND;
        xLocal[(rLast + 1) * MESHSIZE + c]  = SOUTH_BOUND;
    }

    memcpy(xNew, xLocal, (CHUNKROWS + 2) * MESHSIZE * sizeof(float));

    if (rank == 0) start = MPI_Wtime();

    itrCount = 0;
    int pronto = 0;

    // === LOOP PRINCIPAL EM FASES ===
    while (!pronto && itrCount < maxIterations) {
        itrCount++;

        // ---- FASE 1: PROCESSAMENTO LOCAL ----
        diffNorm = 0.0;
        for (r = rFirst; r <= rLast; r++) {
            for (c = 1; c < MESHSIZE - 1; c++) {
                xNew[r * MESHSIZE + c] = (
                    xLocal[r * MESHSIZE + c + 1] +
                    xLocal[r * MESHSIZE + c - 1] +
                    xLocal[(r + 1) * MESHSIZE + c] +
                    xLocal[(r - 1) * MESHSIZE + c]) / 4.0;
                diffNorm += (xNew[r * MESHSIZE + c] - xLocal[r * MESHSIZE + c]) *
                            (xNew[r * MESHSIZE + c] - xLocal[r * MESHSIZE + c]);
            }
        }

        // ---- FASE 2: VERIFICAÇÃO GLOBAL DE CONVERGÊNCIA ----
        // Cada processo calcula seu próprio diffNorm
        gDiffNorm = sqrt(diffNorm);
        int local_ok = (gDiffNorm < epsilon) ? 1 : 0;

        // Cada processo compartilha seu estado com todos (Alltoall)
        int *estados_env = (int*)malloc(commSize * sizeof(int));
        int *estados_rec = (int*)malloc(commSize * sizeof(int));
        for (int i = 0; i < commSize; i++) estados_env[i] = local_ok;
        MPI_Alltoall(estados_env, 1, MPI_INT, estados_rec, 1, MPI_INT, MPI_COMM_WORLD);

        // Cada processo avalia se todos estão prontos
        pronto = 1;
        for (int i = 0; i < commSize; i++) {
            if (estados_rec[i] == 0) pronto = 0;
        }
        free(estados_env);
        free(estados_rec);

        if (rank == 0 && itrCount % 500 == 0)
            printf("[Iter %d] Local diff = %e | Pronto = %d\n", itrCount, gDiffNorm, pronto);

        if (pronto) break;

        // ---- FASE 3: TROCA DE VALORES COM VIZINHOS ----
        if (rank < commSize - 1)
            MPI_Send(xNew + (CHUNKROWS * MESHSIZE), MESHSIZE, MPI_FLOAT, rank + 1, 0, MPI_COMM_WORLD);
        if (rank > 0)
            MPI_Recv(xNew, MESHSIZE, MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD, &status);

        if (rank > 0)
            MPI_Send(xNew + (1 * MESHSIZE), MESHSIZE, MPI_FLOAT, rank - 1, 1, MPI_COMM_WORLD);
        if (rank < commSize - 1)
            MPI_Recv(xNew + ((CHUNKROWS + 1) * MESHSIZE), MESHSIZE, MPI_FLOAT, rank + 1, 1, MPI_COMM_WORLD, &status);

        // Troca de ponteiros
        float* tmp = xLocal;
        xLocal = xNew;
        xNew = tmp;
    }

    // === FASE FINAL: COLETA E SAÍDA ===
    if (rank == 0) {
        memcpy(xFull, xLocal + (1 * MESHSIZE), CHUNKSIZE * sizeof(float));

        int pointerOffset = 0;
        for (int proc = 1; proc < commSize; proc++) {
            pointerOffset += getChunkSize(proc - 1, commSize, MESHSIZE);
            MPI_Recv(xFull + pointerOffset, getChunkSize(proc, commSize, MESHSIZE),
                     MPI_FLOAT, proc, 0, MPI_COMM_WORLD, &status);
        }

        stop = MPI_Wtime();
        printf("\nConvergência atingida em %d iterações (%f s)\n", itrCount, stop - start);
        writeToPPM(xFull, itrCount, MESHSIZE);
    } else {
        MPI_Send(xLocal + (1 * MESHSIZE), CHUNKSIZE, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
    }

    free(xLocal);
    free(xNew);
    if (rank == 0) free(xFull);

    if (rank == 0) printf("<normal termination>\n");
    MPI_Finalize();
    return 0;
}

// === Funções auxiliares (sem alterações estruturais) ===
void writeToPPM(float* mesh, int iterations, const int MESHSIZE) {
    FILE* fp = fopen("jacobi.ppm", "w");
    fprintf(fp, "P3 %d %d 255\n", MESHSIZE, MESHSIZE);
    fprintf(fp, "# Jacobi MPI (Fases Paralelas com Alltoall)\n");
    fprintf(fp, "# Iterações: %d\n", iterations);

    for (int r = 0; r < MESHSIZE; r++) {
        for (int c = 0; c < MESHSIZE; c++) {
            float temp = mesh[r * MESHSIZE + c];
            int red = lerp(0.0, 255.0, temp);
            int blue = lerp(255.0, 0.0, temp);
            fprintf(fp, "%d 0 %d ", red, blue);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

float lerp(float from, float to, float t) {
    return from + (t / 100.0f) * (to - from);
}

int getChunkRows(int rank, int commSize, const int MESHSIZE) {
    int chunkRows = MESHSIZE / commSize;
    int remainder = MESHSIZE % commSize;
    return (rank < remainder) ? chunkRows + 1 : chunkRows;
}

int getChunkSize(int rank, int commSize, const int MESHSIZE) {
    return getChunkRows(rank, commSize, MESHSIZE) * MESHSIZE;
}
