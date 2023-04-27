
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "mpi.h"
#define MAX_SIZE 100000

int read_arr_from_file(char* filename, int* arr) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Could not open file %s\n", filename);
        return -1;
    }

    int size = 0;
    int num;
    while (fscanf(file, "%d", &num) == 1) {
        arr[size++] = num;
    }

    fclose(file);

    return size;
}



//return the minimum value
int imin(int x, int y, int z){
    return x < y ? ((x < z) ? x : z) : ((y < z) ? y : z);
}

int dtw(int a[], int size_a, int b[], int size_b){

    int size;//task number
    int rank;

    MPI_Status Stat;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int from = rank-1; 
    int to = rank+1;

    int msg = -1;
    

    int start_b = (size_b/size)*rank;
    int chunksize = size_b/size;

    int lastrank = (rank == size-1);

    if (lastrank) chunksize += size_b % size;
    int cols = chunksize +1; 
    int rows = size_a+1;
    

    // Initialize matrices
    int **current = (int **)malloc(rows * sizeof(int *));
    int **done = (int **)malloc(rows * sizeof(int *));
    for(int i=0; i<rows; i++) {
        current[i] = (int *)malloc(cols * sizeof(int));
        done[i] = (int *)malloc(cols * sizeof(int));
    }

    // Initialize done matrix with infinity
    for(int i=0; i<rows; i++) {
        for(int j=0; j<cols; j++) {
            done[i][j] = INT_MAX;
        }
    }

    if(rank == 0) {
        done[0][0] = 0;
        //calculate the cost matrix
        for(int i = 1; i < rows; i++){
            current[i][0] = INT_MAX;
            for(int j = 1; j < cols; j++) {
                int cost = abs(a[i-1] - b[start_b + j-1]);
                current[i][j] = cost + imin(done[i-1][j-1],done[i-1][j],current[i][j-1]);
            }

            MPI_Send(&current[i][cols-1], 1, MPI_INT, to, 1, MPI_COMM_WORLD);

            int **temp = current;
            current = done;
            done = temp;
        }

    }
    else {
        for(int i = 1; i < rows; i++){
            MPI_Recv(&msg, 1, MPI_INT, from, 1, MPI_COMM_WORLD, &Stat);

            current[i][0] = msg;
            for(int j = 1; j < cols; j++) {
                int cost = abs(a[i-1] - b[start_b + j-1]);
                current[i][j] = cost + imin(done[i-1][j-1],done[i-1][j],current[i][j-1]);
            }
        


            if (!lastrank) MPI_Send(&current[i][cols-1], 1, MPI_INT, to, 1, MPI_COMM_WORLD);        
            //swap
            int **temp = current;
            current = done;
            done = temp;
        }
    }

    int result = done[rows-1][cols-1];

    // Free memory
    for(int i=0; i<rows; i++) {
        free(current[i]);
        free(done[i]);
    }
    free(current);
    free(done);

    if(lastrank) return result;
    MPI_Finalize();
    exit(0);
    //return -1;
}


int main(int argc,char **argv){

    int *a = (int*)malloc(MAX_SIZE * sizeof(int));
    int *b = (int*)malloc(MAX_SIZE * sizeof(int));


    int size_a = read_arr_from_file(argv[1], a);
    int size_b = read_arr_from_file(argv[2], b);


//swap arrays
    if(size_a > size_b){
        int tmp1 = size_a;
        size_a = size_b;
        size_b = tmp1;

        int *tmp = a;
        a = b;
        b = tmp;
    }


    MPI_Init(&argc,&argv);
    int rank, size;
    int len;
    char hostname[MPI_MAX_PROCESSOR_NAME];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank); //number of current task
    MPI_Comm_size(MPI_COMM_WORLD, &size); //number of tasks/jobs

    
    
    if( size < 2){
        if(rank == 0)
          printf("Error");
        exit(1);
    }
    
    double start = MPI_Wtime();
    int _dtw = dtw(a , size_a, b, size_b);
    double end = MPI_Wtime();
    double time_spent = (double)(end - start);
    printf("DTW distance = %d\n", _dtw);
    printf("Working time: %lf\n", time_spent);


    MPI_Finalize();
}
