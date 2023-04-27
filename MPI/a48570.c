#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "mpi.h"

#define MASTER	0

typedef struct{
  int run_number;
  long executation_time[30];
}runs;

void statistics(runs *s1, double communication_time, FILE *out){
  long time = 0;
  long result;
  for(int i = 0; i < 24; i++){
    for(int j = 1; j < 31; j++){
      time += s1[j].executation_time[i];
    }
    result = (time/30);
    //long communication = communication_time * 1000;
    //long racio = (result /communication);
     printf("Average time for 2^%d in the 31 runs, excluding the first: %lu milisec.\n",i,result);
     fprintf(out, "Average time for 2^%d in the 31 runs, excluding the first: %lu milisec.\n",i,result);
     //printf("racio: %lu\n", racio);
  }
}

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
  int len, rank, size;
  printf("starting...");
  char hostname[MPI_MAX_PROCESSOR_NAME];
  FILE *output = fopen("MPI_output.txt", "w");
  int NRPE;
  int n = 10000000; // max input size

  // Initialize MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Get_processor_name(hostname, &len);
  MPI_Status status;

  printf("hello from rank %d of %d\n on %s!\n", rank, size, hostname);

  if (rank == MASTER)
    printf("MASTER: Number of MPI tasks is: %d\n",rank);

  // Check if enough arguments are provided
  /*if (argc < 3) {
      if (rank == 0) {
          printf("Usage: mpirun -np <num_processes> <program> <file1> <file2>\n");
      }
      MPI_Finalize();
      return 1;
  }*/

  // Get file paths from command-line arguments
  char *f1_path = argv[1];
  char *f2_path = argv[2];

  // Open files in parallel
  FILE *f1 = fopen(f1_path, "r");
  FILE *f2 = fopen(f2_path, "r");

  // Read file sizes and data in parallel
  clock_t start, end;
  double *x = (double *)malloc(n * sizeof(double)); // query
  int size_x = 0;
  double *y = (double *)malloc(n * sizeof(double)); // reference
  int size_y = 0;

  if (rank == 0) {
      size_x = doubles_get_file(x, f1);
      size_y = doubles_get_file(y, f2);
  }

  printf("hello from rank %d of %d\n on %s!\n", rank, size, hostname);
  fprintf(output,"hello from rank %d of %d\n", rank, size);

  if (rank == MASTER)
    printf("MASTER: Number of MPI tasks is: %d\n",rank);

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
  runs s1[40];
  double start_mpi = MPI_Wtime();
  for(int t = 0; t < 1; t++){
    for(int j = 0; j < 24; j++){
      int size = 1*pow(2,j);
      NRPE=size_x/size;
      for(int i=0;i<size;i++)
        for(int s=0;j<size_x;j++){
          MPI_Send(&x[s],size_x*NRPE, MPI_INT, i, 0, MPI_COMM_WORLD);
          MPI_Send(&y[s],size_y*NRPE, MPI_INT, i, 0, MPI_COMM_WORLD);
          NRPE++;
      }
      MPI_Recv(x,size_x, MPI_INT,0,0,MPI_COMM_WORLD,&status);
      MPI_Recv(y,size_y, MPI_INT,0,0,MPI_COMM_WORLD,&status);
      start = clock();
      dtw_distance = dtw(x, y, size, size);
      end = clock();
      long time_taken =(long) (end - start);
      s1[t].run_number = t;
      s1[t].executation_time[j] = time_taken;
      printf("\n2^%d is completed, distance: %.2lf, time: %lu\n", j, dtw_distance, time_taken);
    }
    printf("\nrun number %d is completed, distance: %.2lf\n", t, dtw_distance);
  }
  MPI_Send(&x[size_x-1],size_x*size_x, MPI_INT, 0, 0, MPI_COMM_WORLD);
  MPI_Send(&y[size_x-1],size_x*size_x, MPI_INT, 0, 0, MPI_COMM_WORLD);
  double end_mpi = MPI_Wtime();
  printf("hello from rank %d of %d\n on %s!\n", rank, size, hostname);

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
    fprintf(output,"distance: %.2lf, time: %lu\n", total_dtw_distance, time_taken);
  }

    // Free memory
  free(x);
  free(y);

  // Finalize MPI
  MPI_Finalize();
  double communication_time = end_mpi - start_mpi;
  statistics(s1, communication_time, output);
  printf("mpi_communication: %lf\n", communication_time);
  return 0;
}
