// gcc -O2 MCpi_sequencial.c -o MCpi_sequencial -lm
// ./MCpi_sequencial

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SEED 314159

// Função para medir tempo de execução em segundos
double get_time() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

int main(int argc, char* argv[]) {
    long total_tasks = 100;        // número de blocos (mesmo do paralelo)
    long points_per_task = 100000; // pontos por bloco
    long total_points = total_tasks * points_per_task;
    long total_in_circle = 0;

    srand(SEED);

    printf("[SEQUENCIAL] Iniciando cálculo de PI com %ld pontos...\n", total_points);

    double start = get_time();

    for (long t = 0; t < total_tasks; t++) {
        for (long i = 0; i < points_per_task; i++) {
            double x = (double)rand() / RAND_MAX;
            double y = (double)rand() / RAND_MAX;
            if (x*x + y*y <= 1.0)
                total_in_circle++;
        }
    }

    double end = get_time();
    double elapsed = end - start;

    double pi = 4.0 * ((double)total_in_circle / (double)total_points);

    printf("[SEQUENCIAL] PI ≈ %.6f\n", pi);
    printf("[SEQUENCIAL] Tempo total (T1) = %.6f segundos\n", elapsed);

    return 0;
}
