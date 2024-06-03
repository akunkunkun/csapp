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
    if ( M == 32 )
    {   int _0, _1, _2, _3, _4, _5, _6, _7;

        for (int m = 0; m < 32; m += 8)
        {
            for (int n = 0; n < 32; n += 8)
            {
                if (m != n)
                {
                    for (int i = m; i < m + 8; ++i)
                    {
                        for (int j = n; j < n + 8; ++j)
                        {
                            B[j][i] = A[i][j];
                        }
                    }
                }
                else
                {
                    for (int i = m; i < m + 8; ++i)
                    {
                        // 必须整行搬运
                        // 这种写法相当于完全展开内层循环
                        _0 = A[i][n];
                        _1 = A[i][n + 1];
                        _2 = A[i][n + 2];
                        _3 = A[i][n + 3];
                        _4 = A[i][n + 4];
                        _5 = A[i][n + 5];
                        _6 = A[i][n + 6];
                        _7 = A[i][n + 7];

                        B[i][n] = _0;
                        B[i][n + 1] = _1;
                        B[i][n + 2] = _2;
                        B[i][n + 3] = _3;
                        B[i][n + 4] = _4;
                        B[i][n + 5] = _5;
                        B[i][n + 6] = _6;
                        B[i][n + 7] = _7;
                    }

                    // 原址转置！
                    for (int i = m; i < m + 8; ++i)
                    {
                        for (int j = n + (i - m + 1); j < n + 8; ++j)
                        {
                            if (i != j)
                            {
                                _0 = B[i][j];
                                B[i][j] = B[j][i];
                                B[j][i] = _0;
                            }
                        }
                    }
                }
            }
        }
    }

    if ( M == 64 ){
        // with the original operation (without blocks-shifting and lazy-transposing), 1176 = 35 * 8 + 16 * 56
        // with block-shifting and lazy-transposing, it reaches theoratical limit: 1024 = 16 * 64
        // loop indices
        int i, j, ii, jj;
        // temporary variables
        int a0, a1, a2, a3, a4, a5, a6, a7;
        // main loop: ii, jj for each block of size 8x8

        for (jj = 0; jj < N; jj += 8){
            // process diagonal blocks first
            
            // ii: j-index of target block (block-shifting)
            // more specifically, use the upper half of [jj, ii] to transpose [jj, jj] block 
            // the target block is the one that will be used immediately after the diagonal processing
            if (jj == 0) ii = 8; else ii = 0;

            // move the lower 4x8 blocks from A to B, with block-shifting to the target block 
            for (i = jj; i < jj + 4; ++i){
                a0 = A[i+4][jj+0];
                a1 = A[i+4][jj+1];
                a2 = A[i+4][jj+2];
                a3 = A[i+4][jj+3];
                a4 = A[i+4][jj+4];
                a5 = A[i+4][jj+5];
                a6 = A[i+4][jj+6];
                a7 = A[i+4][jj+7];

                B[i][ii+0] = a0;
                B[i][ii+1] = a1;
                B[i][ii+2] = a2;
                B[i][ii+3] = a3;
                B[i][ii+4] = a4;
                B[i][ii+5] = a5;
                B[i][ii+6] = a6;
                B[i][ii+7] = a7;
            }

            // taking transpose of lower-left and lower-right 4x4 within themselves respectively
            for (i = 0; i < 4; ++ i){
                for (j = i + 1; j < 4; ++j){
                    a0 = B[jj+i][ii+j];
                    B[jj+i][ii+j] = B[jj+j][ii+i];
                    B[jj+j][ii+i] = a0;

                    a0 = B[jj+i][ii+j+4];
                    B[jj+i][ii+j+4] = B[jj+j][ii+i+4];
                    B[jj+j][ii+i+4] = a0;
                }
            }

            // moving the upper 4x8 blocks from A to B
            for (i = jj; i < jj + 4; ++i){
                a0 = A[i][jj+0];
                a1 = A[i][jj+1];
                a2 = A[i][jj+2];
                a3 = A[i][jj+3];
                a4 = A[i][jj+4];
                a5 = A[i][jj+5];
                a6 = A[i][jj+6];
                a7 = A[i][jj+7];

                B[i][jj+0] = a0;
                B[i][jj+1] = a1;
                B[i][jj+2] = a2;
                B[i][jj+3] = a3;
                B[i][jj+4] = a4;
                B[i][jj+5] = a5;
                B[i][jj+6] = a6;
                B[i][jj+7] = a7;
            }

            // taking transpose of upper-left and upper-right 4x4 within themselves respectively
            for (i = jj; i < jj + 4; ++i){
                for (j = i + 1; j < jj + 4; ++j){
                    a0 = B[i][j];
                    B[i][j] = B[j][i];
                    B[j][i] = a0;

                    a0 = B[i][j+4];
                    B[i][j+4] = B[j][i+4];
                    B[j][i+4] = a0;
                }
            }
            
            // swaping the lower-left and upper-right
            for (i = 0; i < 4; ++ i){
                a0 = B[jj+i][jj+4];
                a1 = B[jj+i][jj+5];
                a2 = B[jj+i][jj+6];
                a3 = B[jj+i][jj+7];

                B[jj+i][jj+4] = B[jj+i][ii+0];
                B[jj+i][jj+5] = B[jj+i][ii+1];
                B[jj+i][jj+6] = B[jj+i][ii+2];
                B[jj+i][jj+7] = B[jj+i][ii+3];

                B[jj+i][ii+0] = a0;
                B[jj+i][ii+1] = a1;
                B[jj+i][ii+2] = a2;
                B[jj+i][ii+3] = a3;

            }

            // filling the original lower 4x8 from the block-shifting block
            for (i = 0; i < 4; ++ i){
                B[jj+i+4][jj+0] = B[jj+i][ii+0];
                B[jj+i+4][jj+1] = B[jj+i][ii+1];
                B[jj+i+4][jj+2] = B[jj+i][ii+2];
                B[jj+i+4][jj+3] = B[jj+i][ii+3];
                B[jj+i+4][jj+4] = B[jj+i][ii+4];
                B[jj+i+4][jj+5] = B[jj+i][ii+5];
                B[jj+i+4][jj+6] = B[jj+i][ii+6];
                B[jj+i+4][jj+7] = B[jj+i][ii+7];
            }

            // processing off-diagonal blocks
            for (ii = 0; ii < M; ii += 8){
                if (ii == jj){
                    // skip diagonal blocks
                    continue;
                }else{
                    // taking transpose of upper-left 4x4 and upper-right 4x4 within themselves respectively
                    for (i = ii; i < ii + 4; ++i){
                        a0 = A[i][jj+0];
                        a1 = A[i][jj+1];
                        a2 = A[i][jj+2];
                        a3 = A[i][jj+3];
                        a4 = A[i][jj+4];
                        a5 = A[i][jj+5];
                        a6 = A[i][jj+6];
                        a7 = A[i][jj+7];

                        B[jj+0][i] = a0;
                        B[jj+1][i] = a1;
                        B[jj+2][i] = a2;
                        B[jj+3][i] = a3;
                        B[jj+0][i+4] = a4;
                        B[jj+1][i+4] = a5;
                        B[jj+2][i+4] = a6;
                        B[jj+3][i+4] = a7;
                    }

                    // taking transpose of lower-left 4x4 and store to upper-right 4x4, and move upper-right 4x4 to lower-left 4x4
                    for (j = jj; j < jj + 4; ++j){
                        a0 = A[ii+4][j];
                        a1 = A[ii+5][j];
                        a2 = A[ii+6][j];
                        a3 = A[ii+7][j];

                        a4 = B[j][ii+4];
                        a5 = B[j][ii+5];
                        a6 = B[j][ii+6];
                        a7 = B[j][ii+7];

                        B[j][ii+4] = a0;
                        B[j][ii+5] = a1;
                        B[j][ii+6] = a2;
                        B[j][ii+7] = a3;

                        B[j+4][ii+0] = a4;
                        B[j+4][ii+1] = a5;
                        B[j+4][ii+2] = a6;
                        B[j+4][ii+3] = a7;
                    }

                    // taking transpose of lower-right 4x4
                    for (i = ii + 4; i < ii + 8; ++i){
                        a0 = A[i][jj+4];
                        a1 = A[i][jj+5];
                        a2 = A[i][jj+6];
                        a3 = A[i][jj+7];

                        B[jj+4][i] = a0;
                        B[jj+5][i] = a1;
                        B[jj+6][i] = a2;
                        B[jj+7][i] = a3;
                    }
                }
            }
        }
    }else if (M == 61)
    {
        for (int m = 0; m < 67; m += 17)
        {
            for (int n = 0; n < 61; n += 4)
            {
                int x = m + 17 < 67 ? m + 17 : 67 ;
                int y = n+4 < 61 ? n + 4 : 61;
                for (int i = m; i < x; ++i)
                // min 自己实现一下就好，宏或者内联什么的都是可以的。
                {
                    for (int j = n; j < y; ++j)
                    {
                        B[j][i] = A[i][j];
                    }
                }
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
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
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
    //registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

