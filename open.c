#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>
#include<omp.h>

int abs_distance(int a, int b) {
    return abs(a-b);
}


void build_matrix(int *a, int m, int *b, int n, int **matrix) {
    matrix[0][0] = 0;
    #pragma omp parallel sections 
    {
        #pragma omp section
        {
            for (int i = 1; i < m; i++) 
                matrix[0][i] = abs_distance(a[0], b[i]) + matrix[0][i-1];
        }
        
        #pragma omp section
        {
            for (int i = 1; i < n; i++)
                matrix[i][0] = abs_distance(a[i], b[0]) + matrix[i-1][0];
        }

        #pragma omp section
        {
            #pragma omp parallel for shared(a, b, matrix) 
            {   
                for (int i = 1; i < m; i++) {
                    for (int j = 1; j < n; j++) {
                        matrix[i][j] = abs_distance(a[i], b[j]) + fmin(matrix[i-1][j], fmin(matrix[i][j-1], matrix[i-1][j-1]));
                    }
                }
            }
        }
    }
}


int dtw(int **matrix, int m, int n, int *dtw) {
    int count = 0;
    dtw[count++] = matrix[m][n];
    while (m != 0 && n != 0) {
        int min = fmin(matrix[n-1][m-1], fmin(matrix[n][m-1], matrix[n-1][m]));
        if (min == matrix[n-1][m-1]) {
            dtw[count++] = min;
            n--;
            m--;
        } else if (min == matrix[n][m-1]) {
            dtw[count++] = min;
            m--;
        } else {	
            dtw[count++] = min;
            n--;
        }
    }
    return count;
}


int main() {
    FILE* input1;
	FILE* input2;

	int num1, count1 = 0;
	int num2, count2 = 0;

	int *arr1 = NULL, *arr2 = NULL;

	input1 = fopen("input1.txt", "r");
	input2 = fopen("input2.txt", "r");

	while (fscanf(input1, "%d", &num1) == 1) {
		count1++;
	}

	while (fscanf(input2, "%d", &num2) == 1) {
		count2++;
	}

	arr1 = (int*) malloc(count1 * sizeof(int));
	arr2 = (int*) malloc(count2 * sizeof(int));

	fseek(input1, 0, SEEK_SET);
	fseek(input2, 0, SEEK_SET);

	for (int i = 0; i < count1; i++)
		fscanf(input1, "%d", &arr1[i]);

	for (int i = 0; i < count2; i++)
		fscanf(input2, "%d", &arr2[i]);

	for (int i = 0; i < count1; i++)
		printf("%d\n", arr1[i]);
	
	printf("\n");

	for (int i = 0; i < count2; i++)
		printf("%d\n", arr2[i]);

	printf("\n");


	fclose(input1);
	fclose(input2);

	/**
	 * Matrix
	*/
	int **matrix;
	matrix = (int**) malloc(count1 * sizeof(int*));
	for (int i = 0; i < count1; i++) {
		matrix[i] = (int*) malloc(count2 * sizeof(int));	
	}

	build_matrix(arr1, count1, arr2, count2, matrix);

	free(arr1);
	free(arr2);
	
	
	for (int i = 0; i < count1; i++) {
        for (int j = 0; j < count2; j++) {
            printf("[%d]  ", matrix[i][j]);
        }
        printf("\n");
    }

    int path[count1*count2];

    int path_length = dtw(matrix, count1-1, count2-1, path);

    for (int i = 0; i < path_length; i++)
    {
        printf("%d  ", path[i]);
    }
    free(path);
    return 0;
}