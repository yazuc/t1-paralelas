// mpirun -np 4 ./mpiMCpi_balanceamento weak
// mpirun -np 4 ./mpiMCpi_balanceamento strong

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "mpi.h"

#define SEED 314159
#define TASK_TAG 2
#define RESULT_TAG 3
#define TERMINATE_TAG 4
#define REQUEST_TAG 1
#define BALANCE_TAG 5

int main(int argc, char* argv[]) {
    int myid, numnodes;
    long base_tasks = 10000;
    long points_per_task = 1000000;
    long total_tasks;
    long next_task = 0;
    double pi = 0.0;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &numnodes);

    srand(SEED + myid);

    int is_weak = 0;
    if (argc > 1 && strcmp(argv[1], "weak") == 0)
        is_weak = 1;

    total_tasks = is_weak ? base_tasks * numnodes : base_tasks;

    double start_global = MPI_Wtime();

    if (myid == 0) {
        // ======== MESTRE ========
        int active_workers = numnodes - 1;
        long total_in_circle = 0;

        printf("[MASTER] Iniciando com %d processos (%s scaling)...\n",
               numnodes, is_weak ? "Weak" : "Strong");

        while (active_workers > 0) {
            int worker_id;
            MPI_Recv(&worker_id, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &status);

            if (next_task < total_tasks) {
                MPI_Send(&next_task, 1, MPI_LONG, status.MPI_SOURCE, TASK_TAG, MPI_COMM_WORLD);
                next_task++;
            } else {
                MPI_Send(&next_task, 1, MPI_LONG, status.MPI_SOURCE, TERMINATE_TAG, MPI_COMM_WORLD);
                active_workers--;
            }

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
        double end_global = MPI_Wtime();
        double total_time = end_global - start_global;

        printf("\n[MASTER] PI ≈ %.6f com %ld pontos (%ld tarefas)\n", pi, total_points, total_tasks);
        printf("[MASTER] Tempo total = %.6f segundos\n", total_time);

        // ======== RECEBE TEMPOS DOS TRABALHADORES ========
        double tempos_trabalho[numnodes], tempos_espera[numnodes];
        tempos_trabalho[0] = 0; // mestre não processa
        tempos_espera[0] = 0;

        for (int i = 1; i < numnodes; i++) {
            double twork, twait;
            double data[2];
            MPI_Recv(data, 2, MPI_DOUBLE, i, BALANCE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            tempos_trabalho[i] = data[0];
            tempos_espera[i] = data[1];
        }

        // ======== ANÁLISE DE BALANCEAMENTO ========
        double soma = 0, soma2 = 0;
        double max = 0, min = 1e9;

        for (int i = 1; i < numnodes; i++) {
            soma += tempos_trabalho[i];
            soma2 += tempos_trabalho[i] * tempos_trabalho[i];
            if (tempos_trabalho[i] > max) max = tempos_trabalho[i];
            if (tempos_trabalho[i] < min) min = tempos_trabalho[i];
        }

        double media = soma / (numnodes - 1);
        double variancia = (soma2 / (numnodes - 1)) - (media * media);
        double desvio = sqrt(variancia);
        double desequilibrio = (max - min) / media * 100.0;

        printf("\n[MASTER] --- Análise de Balanceamento ---\n");
        printf("Tempo médio de trabalho: %.6f s\n", media);
        printf("Desvio padrão: %.6f s\n", desvio);
        printf("Diferença máx-mín: %.2f%%\n", desequilibrio);
        printf("-----------------------------------------\n");
    }

    else {
        // ======== TRABALHADOR ========
        double tempo_trabalho = 0.0;
        double tempo_espera = 0.0;
        double t1, t2;

        while (1) {
            t1 = MPI_Wtime();
            MPI_Send(&myid, 1, MPI_INT, 0, REQUEST_TAG, MPI_COMM_WORLD);
            long task_id;
            MPI_Recv(&task_id, 1, MPI_LONG, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            t2 = MPI_Wtime();
            tempo_espera += (t2 - t1);

            if (status.MPI_TAG == TERMINATE_TAG)
                break;

            double inicio_proc = MPI_Wtime();
            long count = 0;
            for (long i = 0; i < points_per_task; i++) {
                double x = (double)rand() / RAND_MAX;
                double y = (double)rand() / RAND_MAX;
                if (x*x + y*y <= 1.0)
                    count++;
            }
            double fim_proc = MPI_Wtime();
            tempo_trabalho += (fim_proc - inicio_proc);

            MPI_Send(&count, 1, MPI_LONG, 0, RESULT_TAG, MPI_COMM_WORLD);
        }

        // Envia tempos para o mestre
        double tempos[2] = {tempo_trabalho, tempo_espera};
        MPI_Send(tempos, 2, MPI_DOUBLE, 0, BALANCE_TAG, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}

