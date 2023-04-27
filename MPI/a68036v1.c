#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <string.h>

#define MAX_SIZE 1000

int dtw(int *X, int *Y, int n, int m){
    int **dist = (int **)malloc((m+1) * sizeof(int *)); //allocate memory for 2D array
    for(int i = 0; i <= m; i++){
        dist[i] = (int *)malloc((n+1) * sizeof(int));
    }

    //Inicializar a matriz de distancias
    dist[0][0] = 0;
    
    // Calcular as distâncias
    for(int i = 1; i <= m; i++) {
        for(int j = 1; j <= n; j++) {
            int cost = fabs(X[j-1] - Y[i-1]);
            int minDist = fmin(dist[i-1][j], fmin(dist[i][j-1], dist[i-1][j-1]));
            dist[i][j] = cost + minDist;
        }
    }

    int smallestDist = dist[m][n];
    //Libertar memória
    for(int i=0; i < m; i++){
        free(dist[i]);
    }
    free(dist);
    return smallestDist;
}


int main(int argc, char *argv[]) {
    int rank, size;
    int len1, len2;
    int *num1 = NULL, *num2 = NULL;
    double start_time, end_time, tempo;
    MPI_File fp1, fp2, output;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0) {
            printf("Usage: %s x_file y_file output_file\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    MPI_File_open(MPI_COMM_WORLD, argv[1], MPI_MODE_RDONLY, MPI_INFO_NULL, &fp1);
    MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &fp2);
    MPI_File_open(MPI_COMM_WORLD, argv[3], MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &output);

    MPI_Offset fsize1, fsize2;
    MPI_File_get_size(fp1, &fsize1);
    MPI_File_get_size(fp2, &fsize2);
    len1 = fsize1 / sizeof(int);
    len2 = fsize2 / sizeof(int);
    int *buf1 = (int *)malloc(len1 * sizeof(int));
    int *buf2 = (int *)malloc(len2 * sizeof(int));

    MPI_File_read(fp1, buf1, len1, MPI_INT, MPI_STATUS_IGNORE);
    MPI_File_read(fp2, buf2, len2, MPI_INT, MPI_STATUS_IGNORE);

    MPI_File_close(&fp1);
    MPI_File_close(&fp2);

    int smallestDist = dtw(buf1, buf2, len1, len2);
    free(buf1);
    free(buf2);

    if (rank == 0) {
        char result_string[100];
        snprintf(result_string, sizeof(result_string), "Distância mínima usando DTW: %d\n", smallestDist);
        MPI_File_write(output, result_string, strlen(result_string), MPI_CHAR, MPI_STATUS_IGNORE);
        end_time = MPI_Wtime();
        tempo = end_time - start_time;
        snprintf(result_string, sizeof(result_string), "Execution time: %lf seconds.\n", tempo);
        MPI_File_write(output, result_string, strlen(result_string), MPI_CHAR, MPI_STATUS_IGNORE);

        snprintf(result_string, sizeof(result_string), "Numero de processos: %d\n", size);
        MPI_File_write(output, result_string, strlen(result_string), MPI_CHAR, MPI_STATUS_IGNORE);

        strcpy(result_string, "Concluido\n");
        MPI_File_write(output, result_string, strlen(result_string), MPI_CHAR, MPI_STATUS_IGNORE);
    }

    MPI_File_close(&output);
    MPI_Finalize();
    return 0;
}