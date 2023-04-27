#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

int j = 0;
int i = 0;

double min(double a, double b, double c) {
    double min = a;
    if (b < min) {
        min = b;
    }
    if (c < min) {
        min = c;
    }
    return min;
}

double dtw(double *X, double *Y, int n, int m, FILE *output_file) {
    double **dtw = (double **)malloc((m + 1) * sizeof(double *)); // allocate memory for 2D array
    for (int i = 0; i <= m; i++) {
        dtw[i] = (double *)malloc((n + 1) * sizeof(double));
    }
    
    dtw[0][0] = 0;
    // preencher a 1 linha
    double soma = 0;
    double soma2 = 0;
    for (int j = 0; j < n; j++)
    {
        dtw[0][j] = abs(X[j] - Y[0]) + soma;
        soma += abs(X[j] - Y[0]);
    }

    // preencher a 1 coluna    
    for (int i = 0; i < m; i++)
    {
        dtw[i][0] = abs(X[0] - Y[i]) + soma2;
        soma2 += abs(X[0] - Y[i]);
    }

    // fazer o resto
    for (int i = 1; i < m; i++)
    {
        for (int j = 1; j < n; j++)
        {
            double custo = abs(X[j] - Y[i]);
            dtw[i][j] = custo + min(dtw[i][j-1], dtw[i-1][j], dtw[i-1][j-1]);
        }
    }

    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            fprintf(output_file, "%lf ", dtw[i][j]);;
        }
       fprintf(output_file, "\n");
    }

    int distance = dtw[m-1][n-1];

    // free dynamically allocated memory
    for (int i = 0; i <= m; i++) {
        free(dtw[i]);
    }
    free(dtw);
        
    return distance;
}

int main(int argc, char *argv[]) {

    int rank, size, tag = 0;
    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        printf("Usage: %s x_file y_file output_file\n", argv[0]);
        return 1;
    }



    

    FILE *x, *y, *output;;  // ponteiro para o arquivo
    double *X = NULL, *Y = NULL;  // array para armazenar os nÃºmeros lidos
    int Len1 = 0;
    int Len2 = 0;

    if (rank == tag) {
        // abrir o arquivo x_file
        x = fopen(argv[1], "r");
        if (x == NULL) {
            printf("Error: could not open file %s\n", argv[1]);
            MPI_Finalize();
            return 1;
        }
        // abrir o arquivo y_file
        y = fopen(argv[2], "r");
        if (y == NULL) {
            printf("Error: could not open file %s\n", argv[2]);
            MPI_Finalize();
            return 1;
        }
        // abrir o arquivo output_file
        output = fopen(argv[3], "w");
        if (output == NULL) {
            printf("Error: could not open file %s\n", argv[3]);
            MPI_Finalize();
            return 1;
        }

        // ler a primeira linha de x_file
        
         double num;
    while (fscanf(x, "%lf", &num) == 1) {
        Len1++;
        X = (double *)realloc(X, Len1 * sizeof(double)); // dynamically allocate memory
        X[Len1-1] = num;
    }

    while (fscanf(y, "%lf", &num) == 1) {
        Len2++;
        Y = (double *)realloc(Y, Len2 * sizeof(double)); // dynamically allocate memory
        Y[Len2-1] = num;
    }

    fclose(x);
    fclose(y);

}
      
    MPI_Bcast(&Len1, 1, MPI_INT, tag, MPI_COMM_WORLD);
    MPI_Bcast(&Len2, 1, MPI_INT, tag, MPI_COMM_WORLD);

    if (rank != 0) {
        X = (double *)malloc(Len1 * sizeof(double));
        Y = (double *)malloc(Len2 * sizeof(double));
    }

    MPI_Bcast(X, Len1, MPI_DOUBLE, tag, MPI_COMM_WORLD);
    MPI_Bcast(Y, Len2, MPI_DOUBLE, tag, MPI_COMM_WORLD);
    
    double distance =0;
    clock_t start_time, end_time;
    double total_time = 0.0;
    start_time = clock();
    distance = dtw(X, Y, Len1, Len2,output);
    end_time = clock();
    total_time += ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

       
    if(rank == tag){
    fprintf(output,"DTW distance: %lf\n", distance);
    fprintf(output,"Tempo total de execução: %lf segundos\n", total_time);
    fclose(output);

    }


    MPI_Finalize();
    return 0;
}