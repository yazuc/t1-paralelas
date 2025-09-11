#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 334             // Tamanho do sistema (Ax = b)
#define MAX_ITER 10000000   // Número máximo de iterações
#define TOL 1e-6        // Tolerância para convergência

void print_vector(double *v) {
    for (int i = 0; i < N; i++) {
        printf("%f ", v[i]);
    }
    printf("\n");
}

int main() {
    double A[N][N] = {
        {10, -1, 2},
        {-1, 11, -1},
        {2, -1, 10}
    };

    double b[N] = {6, 25, -11};
    double x[N] = {0};      // Inicialização com zero
    double x_new[N] = {0};

    for (int iter = 0; iter < MAX_ITER; iter++) {
        for (int i = 0; i < N; i++) {
            double sigma = 0.0;
            for (int j = 0; j < N; j++) {
                if (j != i) {
                    sigma += A[i][j] * x[j];
                }
            }
            x_new[i] = (b[i] - sigma) / A[i][i];
        }

        // Verifica convergência
        double err = 0.0;
        for (int i = 0; i < N; i++) {
            err += fabs(x_new[i] - x[i]);
            x[i] = x_new[i];
        }

        if (err < TOL) {
            printf("Convergiu em %d iterações.\n", iter+1);
            break;
        }
    }

    printf("Solução:\n");
    print_vector(x);

    return 0;
}

