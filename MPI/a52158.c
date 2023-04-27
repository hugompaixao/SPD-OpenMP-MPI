#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mpi.h>

//Funcao do prof Pedro Guerreiro 
int doubles_get_file(double *a, FILE *f)
{
  int result = 0;
  double x;
  while (fscanf(f,"%lf", &x) != EOF)
    a[result++] = x;
  return result;
}

// Swap x and y if size of x is greater than y

double dtw(double *x, double *y, int size_x, int size_y)
{
    // Swap x and y if size of x is greater than y
    if (size_x > size_y)
    {
        double *temp = x;
        x = y;
        y = temp;
        int temp_size = size_x;
        size_x = size_y;
        size_y = temp_size;
    }

    // Memory allocation for previous row and current row
    double *prevRow = (double *)malloc(size_y * sizeof(double));
    double *currRow = (double *)malloc(size_y * sizeof(double));

    // Initialization of previous row
    prevRow[0] = abs(x[0] - y[0]);
    for (int j = 1; j < size_y; j++)
    {
        prevRow[j] = prevRow[j - 1] + abs(x[0] - y[j]);
    }

    // Matrix computation row by row
    for (int i = 1; i < size_x; i++)
    {
        currRow[0] = prevRow[0] + abs(x[i] - y[0]);
        int min = fmax(1, i-1);
        for (int j = min; j < size_y; j++)
        {
            double cost = abs(x[i] - y[j]);
            currRow[j] = cost + fmin(prevRow[j], fmin(currRow[j - 1], prevRow[j - 1]));
        }

        // Swap previous row and current row pointers
        double *tempRow = prevRow;
        prevRow = currRow;
        currRow = tempRow;
    }

    // Retrieve the final DTW distance
    double dtwDistance = prevRow[size_y - 1];

    // Free memory
    free(prevRow);
    free(currRow);

    return dtwDistance;
}


int main(int argc, char *argv[]) {
    // Initialize MPI
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    printf("hello from rank %d of %d\n", rank, size);

    // Check if enough arguments are provided
    if (argc < 3) {
        if (rank == 0) {
            printf("Usage: mpirun -np <num_processes> <program> <file1> <file2>\n");
        }
        MPI_Finalize();
        return 1;
    }

    // Get file paths from command-line arguments
    char *f1_path = argv[1];
    char *f2_path = argv[2];

    // Open files in parallel
    FILE *f1 = fopen(f1_path, "r");
    FILE *f2 = fopen(f2_path, "r");

    // Read file sizes and data in parallel
    int n = 10000000; // max input size
    clock_t start, end;
    double *x = (double *)malloc(n * sizeof(double)); // query
    int size_x = 0;
    double *y = (double *)malloc(n * sizeof(double)); // reference
    int size_y = 0;

    if (rank == 0) {
        size_x = doubles_get_file(x, f1);
        size_y = doubles_get_file(y, f2);
    }

    // Broadcast file sizes to all processes
    MPI_Bcast(&size_x, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&size_y, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Broadcast file data to all processes
    MPI_Bcast(x, size_x, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(y, size_y, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Close files
    fclose(f1);
    fclose(f2);

    // Perform DTW calculation in parallel
    double dtw_distance = 0.0;
    if (rank == 0) {
        start = clock();
        dtw_distance = dtw(x, y, size_x, size_y);
        end = clock();
    }

    // Reduce DTW distances to process 0
    double total_dtw_distance;
    MPI_Reduce(&dtw_distance, &total_dtw_distance, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    // Calculate time taken on process 0
    long time_taken = 0;
    if (rank == 0) {
        time_taken = (long)(end - start);
    }

    // Print result on process 0
    if (rank == 0) {
        printf("distance: %.2lf, time: %lu\n", total_dtw_distance, time_taken);
    }

    // Free memory
    free(x);
    free(y);

    // Finalize MPI
    MPI_Finalize();

    return 0;
}