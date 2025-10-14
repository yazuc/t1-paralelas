// mpirun -np 4 ./mpiMCpi

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

#define SEED 314159
#define TASK_TAG 2
#define RESULT_TAG 3
#define TERMINATE_TAG 4
#define REQUEST_TAG 1

int main(int argc, char* argv[]) {
    int myid, numnodes;
    long total_tasks = 10000;        // número de blocos de trabalho
    long points_per_task = 1000000;  // número de pontos por bloco
    long next_task = 0;              // índice do próximo bloco a distribuir
    double pi = 0.0;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &numnodes);

    srand(SEED + myid);

    if (myid == 0) {
        // ========== MESTRE ==========
        int active_workers = numnodes - 1;
        long total_in_circle = 0;

        while (active_workers > 0) {
            int worker_id;
            MPI_Recv(&worker_id, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &status);

            if (next_task < total_tasks) {
                // Envia uma nova tarefa (um número indicando o bloco)
                MPI_Send(&next_task, 1, MPI_LONG, status.MPI_SOURCE, TASK_TAG, MPI_COMM_WORLD);
                next_task++;
            } else {
                // Saco vazio — envia mensagem de término
                MPI_Send(&next_task, 1, MPI_LONG, status.MPI_SOURCE, TERMINATE_TAG, MPI_COMM_WORLD);
                active_workers--;
            }

            // Recebe resultados prontos, se houver
            int flag;
            MPI_Iprobe(MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, &flag, &status);
            while (flag) {
                long result;
                MPI_Recv(&result, 1, MPI_LONG, status.MPI_SOURCE, RESULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                total_in_circle += result;
                MPI_Iprobe(MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, &flag, &status);
            }
        }

        long total_points = total_tasks * points_per_task;
        pi = 4.0 * ((double)total_in_circle / (double)total_points);
        printf("\n[MASTER] PI ≈ %.6f com %ld pontos (%ld tarefas)\n", pi, total_points, total_tasks);
    }

    else {
        // ========== TRABALHADOR ==========
        while (1) {
            // Envia pedido de trabalho
            MPI_Send(&myid, 1, MPI_INT, 0, REQUEST_TAG, MPI_COMM_WORLD);

            long task_id;
            MPI_Recv(&task_id, 1, MPI_LONG, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == TERMINATE_TAG)
                break; // encerra o trabalhador

            // Processa a tarefa recebida
            long count = 0;
            for (long i = 0; i < points_per_task; i++) {
                double x = (double)rand() / RAND_MAX;
                double y = (double)rand() / RAND_MAX;
                if (x*x + y*y <= 1.0)
                    count++;
            }

            // Envia resultado de volta ao mestre
            MPI_Send(&count, 1, MPI_LONG, 0, RESULT_TAG, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
