#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

int j = 0;
int i = 0;

int min(int a, int b, int c) {
    int min = a;
    if (b < min) {
        min = b;
    }
    if (c < min) {
        min = c;
    }
    return min;
}

int dtw(int *X, int *Y, int n, int m, FILE *output_file) {
    int **dtw = (int **)malloc((m + 1) * sizeof(int *)); // allocate memory for 2D array
    for (int i = 0; i <= m; i++) {
        dtw[i] = (int *)malloc((n + 1) * sizeof(int));
    }

    dtw[0][0] = 0;
    // preencher a 1 linha
    int soma = 0;
    int soma2 = 0;
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
    for (i = 1; i < m; i++)
    {
        for (j = 1; j < n; j++)
        {
            int custo = abs(X[j] - Y[i]);
            dtw[i][j] = custo + min(dtw[i][j-1], dtw[i-1][j], dtw[i-1][j-1]);
        }
    }

    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            fprintf(output_file, "%d ", dtw[i][j]);;
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

    int rank, size, tag;
    MPI_Status status;

    if (argc < 3) {
        printf("Usage: %s x_file y_file output_file\n", argv[0]);
        return 1;
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("Error: number of processes must be at least 2\n");
        MPI_Finalize();
        return 1;
    }

    FILE *x, *y, *output;;  // ponteiro para o arquivo
    int *X = NULL, *Y = NULL;  // array para armazenar os números lidos
    int Len1 = 0;
    int Len2 = 0;

    if (rank == 0) {
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
        char line[1024];
        fgets(line, 1024, x);
        // extrair o primeiro número da linha como sendo o tamanho de X
        Len1 = atoi(strtok(line, " \n"));
        // alocar memória para X
        X = (int *)malloc(Len1 * sizeof(int));
        // ler a segunda linha de x_file
        fgets(line, 1024, x);
        // extrair os números da linha e armazenar em X
        for (int i = 0; i < Len1; i++) {
            X[i] = atoi(strtok(i == 0 ? line : NULL, " \n"));
        }

        // ler a primeira linha de y_file
        fgets(line, 1024, y);
        // extrair o primeiro número da linha como sendo o tamanho de Y
        Len2 = atoi(strtok(line, " \n"));
        // alocar memória para Y
        Y = (int *)malloc(Len2 * sizeof(int));
        // ler a segunda linha de y_file
        fgets(line, 1024, y);
        // extrair os números da linha e armazenar em Y
        for (int i = 0; i < Len2; i++) {
            Y[i] = atoi(strtok(i == 0 ? line : NULL, " \n"));
        }

        // enviar o tamanho de Y para todos os outros processos
        for (int i = 1; i < size; i++) {
            MPI_Send(&Len2, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
        }
    }
     else {
        // receber o tamanho de Y
        MPI_Recv(&Len2, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
        // alocar memória para Y
        Y = (int *)malloc(Len2 * sizeof(int));
        // receber Y do processo 0
        MPI_Bcast(Y, Len2, MPI_INT, 0, MPI_COMM_WORLD);

        // calcular o tamanho da parte de X que este processo irá receber
        int part_size = Len1 / (size - 1);
        if (rank == size - 1) {
            // o último processo pode receber um tamanho diferente
            part_size += Len1 % (size - 1);
        }
        // alocar memória para a parte de X
        int *X_part = (int *)malloc(part_size * sizeof(int));
        // receber a parte de X correspondente a este processo
        MPI_Scatter(X, part_size, MPI_INT, X_part, part_size, MPI_INT, 0, MPI_COMM_WORLD);

        // calcular a parte do produto correspondente a este processo
        int *result_part = (int *)malloc(Len2 * sizeof(int));
        for (int i = 0; i < Len2; i++) {
            result_part[i] = 0;
            for (int j = 0; j < part_size; j++) {
                result_part[i] += X_part[j] * Y[i];
            }
        }

        // coletar todos os resultados em um único array
        int *result = NULL;
        if (rank == 0) {
            result = (int *)malloc(Len1 * sizeof(int));
        }
        MPI_Gather(result_part, Len2, MPI_INT, result, Len2, MPI_INT, 0, MPI_COMM_WORLD);

        if (rank == 0) {
            // escrever o resultado no arquivo de saída
            fprintf(output, "%d\n", Len1 + Len2 - 1);
            for (int i = 0; i < Len1 + Len2 - 1; i++) {
                fprintf(output, "%d ", result[i]);
            }
            fprintf(output, "\n");

            // liberar a memória
            free(result);
        }

        // liberar a memória
        free(X_part);
        free(result_part);
        free(Y);
    }

    // fechar os arquivos
    if (rank == 0) {
        fclose(x);
        fclose(y);
        fclose(output);
    }

    MPI_Finalize();
    return 0;
}