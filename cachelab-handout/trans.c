/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    int i_, j_;
    int a0, a1, a2, a3, a4, a5, a6, a7;

    for (i = 0; i < N; i += 8)
    {
        for (j = 0; j < M; j += 8)
        {
            i_ = j, j_ = i;
            for (int x = 0; x < 8; x++)
            {
                a0 = A[i + x][j + 0];
                a1 = A[i + x][j + 1];
                a2 = A[i + x][j + 2];
                a3 = A[i + x][j + 3];
                a4 = A[i + x][j + 4];
                a5 = A[i + x][j + 5];
                a6 = A[i + x][j + 6];
                a7 = A[i + x][j + 7];

                B[i_ + 0][j_ + x] = a0;
                B[i_ + 1][j_ + x] = a1;
                B[i_ + 2][j_ + x] = a2;
                B[i_ + 3][j_ + x] = a3;
                B[i_ + 4][j_ + x] = a4;
                B[i_ + 5][j_ + x] = a5;
                B[i_ + 6][j_ + x] = a6;
                B[i_ + 7][j_ + x] = a7;
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            B[j][i] = A[i][j];
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
