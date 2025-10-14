// gcc -fopenmp MCpi_sequencial.c -o MCpi_sequencial.exe
// 
// srun -N 1 -n 1 --exclusive MCpi_sequencial.exe
// [SEQUENCIAL] Iniciando cálculo de PI com 10000000000 pontos...
// [SEQUENCIAL] PI ≈ 3.141566
// [SEQUENCIAL] Tempo total (T1) = 370 segundos


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SEED 314159

int main(int argc, char* argv[]) {
    long total_tasks = 10000;        // número de blocos (mesmo do paralelo)
    long points_per_task = 1000000;  // pontos por bloco
    long total_points = total_tasks * points_per_task;
    long total_in_circle = 0;

    srand(SEED);

    printf("[SEQUENCIAL] Iniciando cálculo de PI com %ld pontos...\n", total_points);

    time_t tempo_inicial = time(NULL);

    for (long t = 0; t < total_tasks; t++) {
        for (long i = 0; i < points_per_task; i++) {
            double x = (double)rand() / RAND_MAX;
            double y = (double)rand() / RAND_MAX;
            if (x*x + y*y <= 1.0)
                total_in_circle++;
        }
    }

    time_t tempo_final = time(NULL);
    long duracao = tempo_final - tempo_inicial;

    double pi = 4.0 * ((double)total_in_circle / (double)total_points);

    printf("[SEQUENCIAL] PI ≈ %.6f\n", pi);
    printf("[SEQUENCIAL] Tempo total (T1) = %ld segundos\n", duracao);

    return 0;
}
