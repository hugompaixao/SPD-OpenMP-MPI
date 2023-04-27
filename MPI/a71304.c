#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <mpi.h>

#define infinity INFINITY

int main(int argc, char *argv[]) {
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    FILE *fp1;
    FILE *fp2;
    double *s;
    double *t;

    int n = 0;
    int m = 0;
    char c;

    if (rank == 0) {
        fp1 = fopen(argv[1], "r");
        while ((c = fgetc(fp1)) != EOF) {
            if (c == ' ') {
                n++;
            }
        }
        fclose(fp1);

        s = (double *) malloc(n * sizeof(double));
        fp1 = fopen(argv[1], "r");
        for (int i = 0; i < n; i++) {
            if (fscanf(fp1, "%lf", &s[i]) != 1) {
                return 1;
            }
        }
        fclose(fp1);

        fp2 = fopen(argv[2], "r");
        while ((c = fgetc(fp2)) != EOF) {
            if (c == ' ') {
                m++;
            }
        }
        fclose(fp2);

        t = (double *) malloc(m * sizeof(double));
        fp2 = fopen(argv[2], "r");
        for (int i = 0; i < m; i++) {
            if (fscanf(fp2, "%lf", &t[i]) != 1) {
                return 1;
            }
        }
        fclose(fp2);
    }

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0) {
        s = (double *) malloc(n * sizeof(double));
        t = (double *) malloc(m * sizeof(double));
    }

    MPI_Bcast(s, n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(t, m, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double *DTW = malloc((n + 1) * (m + 1) * sizeof(double));

    for (int i = 1; i <= n; i++)
        DTW[i * (m + 1)] = infinity;

    for (int j = 1; j <= m; j++)
        DTW[j] = infinity;
    DTW[0] = 0;

    double start = MPI_Wtime();

    for (int k = 1; k <= n + m - 1; k++) {
        for (int i = 1; i <= n; i++) {
            int j = k - i + 1;
            if (j >= 1 && j <= m) {
                double cost = fabs(s[i - 1] - t[j - 1]);
                double coord1 = DTW[i * (m + 1) + (j - 1)];
                double coord2 = DTW[(i - 1) * (m + 1) + (j - 1)];
                double coord3 = DTW[(i - 1) * (m + 1) + j];
                double min = fmin(coord1, coord2);
                double min1 = fmin(coord3, min);
                DTW[i * (m + 1) + j] = cost + min1;
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    double end = MPI_Wtime();

    if (rank == 0) {
        FILE *fp3;
        fp3 = fopen(argv[3], "w");
        fprintf(fp3, "Running with %d processes: \n", size);
        fprintf(fp3, "%lf \n", end - start);
        fprintf(fp3, "%lf \n", DTW[n * (m + 1) + m]);
        fclose(fp3);
    }

    free(s);
    free(t);
    free(DTW);

    MPI_Finalize();
    return 0;
}