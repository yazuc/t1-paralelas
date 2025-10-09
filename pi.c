#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[]) {
    int rank, size;
    long long int total_points = 1000000;
    long long int local_points, local_hits = 0, total_hits = 0;
    double x, y;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    local_points = total_points / (size - 1); // apenas os slaves processam
    
    if (rank == 0) {
        // Host
        for (int i = 1; i < size; i++) {
            MPI_Send(&local_points, 1, MPI_LONG_LONG_INT, i, 0, MPI_COMM_WORLD);
        }

        for (int i = 1; i < size; i++) {
            MPI_Recv(&local_hits, 1, MPI_LONG_LONG_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            total_hits += local_hits;
        }

        double pi = 4.0 * total_hits / (local_points * (size - 1));
        printf("Estimativa de Pi: %f\n", pi);
    } else {
        // Slaves
        MPI_Recv(&local_points, 1, MPI_LONG_LONG_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        srand(time(NULL) + rank);
        for (long long int i = 0; i < local_points; i++) {
            x = (double)rand() / RAND_MAX;
            y = (double)rand() / RAND_MAX;
            if (x * x + y * y <= 1.0) local_hits++;
        }

        MPI_Send(&local_hits, 1, MPI_LONG_LONG_INT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}

