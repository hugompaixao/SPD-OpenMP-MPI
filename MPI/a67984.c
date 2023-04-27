#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <mpi.h>

#include "lib/util.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))




double **setupCostMatix(int rows, int cols) {
  // Cost matrix will be of size n+1 rows m+1 columns
  double **costMatrix = (double **)malloc((rows + 1) * sizeof(double *));
  for (int i = 0; i <= rows; i++)
    costMatrix[i] = (double *)malloc((cols + 1) * sizeof(double));

  // Fill first row and first column with INFINITY, except the first cell
  costMatrix[0][0] = 0;
  for (int i = 1; i <= rows; i++)
    costMatrix[i][0] = INFINITY;
  for (int j = 1; j <= cols; j++)
    costMatrix[0][j] = INFINITY;

  return costMatrix;
}


void printAbsMatrix(double **matrix, int rows, int cols) {
  printf("\n");
  printf("ABSOLUTE MATRIX:\n");

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++)
      printf("%lf ", matrix[i][j]);
    printf("\n");
  }
  printf("\n");
}


double **calculateAbsoluteDistance(double *n, int rows, double *m, int cols, int rank, int size){
  double **distance = (double **)malloc(rows * sizeof(double *));
  for (int i = 0; i < rows; i++)
    distance[i] = (double *)malloc(cols * sizeof(double));

  int chunk_size = rows / size;                             // Define row size for each process
  int start = rank * chunk_size;                            // On which row do they start working
  int end = (rank == size - 1) ? rows : start + chunk_size; // Where they stop

  for (int i = start; i < end; i++)
    for (int j = 0; j < cols; j++)
      distance[i][j] = fabs(n[i] - m[j]);

  // Gather the results from all processes to the Master process (0)
  if (rank == 0) {
    for (int i = 1; i < size; i++) {
      int recv_start = i * chunk_size;
      int recv_end = (i == size - 1) ? rows : recv_start + chunk_size;

      for (int j = recv_start; j < recv_end; j++)
        MPI_Recv(distance[j], cols, MPI_DOUBLE, i, j, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
  } else {
    for (int i = start; i < end; i++)
      MPI_Send(distance[i], cols, MPI_DOUBLE, 0, i, MPI_COMM_WORLD);
  }

  return distance;
}


double DTWDistance(double *n, int rows, double *m, int cols, int rank, int size) {
  double **costMatrix = setupCostMatix(rows, cols);
  double **distance = calculateAbsoluteDistance(n, rows, m, cols, rank, size);

  if (rank == 0) { // Only process Master does the calculations
    // Calculate Cost
    for (int i = 1; i <= rows; i++)
      for (int j = 1; j <= cols; j++)
        costMatrix[i][j] = distance[i - 1][j - 1] + min(costMatrix[i - 1][j - 1], min(costMatrix[i - 1][j], costMatrix[i][j - 1]));
  }
  double cost = costMatrix[rows][cols]; // Cost is the last element of the matrix

  // Free to prevent memory leaks
  freeMatrix(costMatrix, rows + 1);
  freeMatrix(distance, rows);

  return cost;
}


int main(int argc, char* argv[]){
  int n, m;
  
  FILE *f = openFile(argv[1]);
  fscanf(f,"%d %d", &n ,&m); // Get the current test instance dimensions, written in the first file line

  double* X = malloc(n * sizeof(double));
  double* Y = malloc(m * sizeof(double));

  read_doubles_DTW(f, X, Y, n, m);

  int rank, num_procs;

  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

  struct timeval begin, end;

  gettimeofday(&begin, 0);

  double start = MPI_Wtime();
  double cost = DTWDistance(X, n, Y, m, rank, num_procs);

  if (rank == 0){
    gettimeofday(&end, 0);

    printf(
      "%s PARALLEL (MPI) %s%s        Test Name %s %s\n",
      LOG_COLOR_GRAY, LOG_COLOR_RESET,
      LOG_COLOR_WHITE, LOG_COLOR_RESET,
      argv[1]
    );
    printf(
      "%s PARALLEL (MPI) %s%s      Matrix Size %s %dx%d\n",
      LOG_COLOR_GRAY, LOG_COLOR_RESET,
      LOG_COLOR_WHITE, LOG_COLOR_RESET,
      n, m
    );

    printf(
      "%s PARALLEL (MPI) %s%s  Result Distance %s %f\n",
      LOG_COLOR_GRAY, LOG_COLOR_RESET,
      LOG_COLOR_GREEN, LOG_COLOR_RESET,
      cost
    );
    printf(
      "%s PARALLEL (MPI) %s%s      Result Time %s %.6f\n\n",
      LOG_COLOR_GRAY, LOG_COLOR_RESET,
      LOG_COLOR_BLUE, LOG_COLOR_RESET,
      (double)(MPI_Wtime() - start)
    );
  }

  MPI_Finalize();

  free(X);
  free(Y);

  return 0;
}