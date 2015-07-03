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
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void result32by32(int A[32][32], int B[32][32]);
void result64by64(int A[64][64], int B[64][64]);
void result61by67(int A[67][61], int B[61][67]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
	REQUIRES(M > 0);
	REQUIRES(N > 0);

	if (M == 32) {
		result32by32(A, B);
	} else if (M == 64) {
		result64by64(A, B);
	} else {
		result61by67(A, B);
	}

	ENSURES(is_transpose(M, N, A, B));
}

void result32by32(int A[32][32], int B[32][32]) {
	int stride;
	int i, j, i1;

	stride = 8;

	for (i = 0; i < 32; i += stride) {
		for (j = 0; j < 32; j += stride) {
			for (i1 = i; i1 < i + stride; i1++) {
				int t = A[i1][j];
				int t1 = A[i1][j + 1];
				int t2 = A[i1][j + 2];
				int t3 = A[i1][j + 3];
				int t4 = A[i1][j + 4];
				int t5 = A[i1][j + 5];
				int t6 = A[i1][j + 6];
				int t7 = A[i1][j + 7];

				B[j][i1] = t;
				B[j + 1][i1] = t1;
				B[j + 2][i1] = t2;
				B[j + 3][i1] = t3;
				B[j + 4][i1] = t4;
				B[j + 5][i1] = t5;
				B[j + 6][i1] = t6;
				B[j + 7][i1] = t7;
			}
		}
	}
}

void result64by64(int A[64][64], int B[64][64]) {
	int stride;
	int i, j, i1;

	int t, t1, t2, t3, t4, t5, t6, t7;

	stride = 8;

	for (j = 0; j < 64; j += stride) {
		for (i = 0; i < 64; i += stride) {
			if (i + stride < 64) {
				/* step 1 */
				for (i1 = i; i1 < i + stride; i1++) {
					t = A[i1][j];
					t1 = A[i1][j + 1];
					t2 = A[i1][j + 2];
					t3 = A[i1][j + 3];
					t4 = A[i1][j + 4];
					t5 = A[i1][j + 5];
					t6 = A[i1][j + 6];
					t7 = A[i1][j + 7];

					B[j][i1] = t;
					B[j + 1][i1] = t1;
					B[j + 2][i1] = t2;
					B[j + 3][i1] = t3;
					B[j][i1 + stride] = t4;
					B[j + 1][i1 + stride] = t5;
					B[j + 2][i1 + stride] = t6;
					B[j + 3][i1 + stride] = t7;
				}

				/* step 2 */
				for (i1 = i; i1 < i + stride; i1++) {
					t4 = B[j][i1 + stride];
					t5 = B[j + 1][i1 + stride];
					t6 = B[j + 2][i1 + stride];
					t7 = B[j + 3][i1 + stride];

					B[j + 4][i1] = t4;
					B[j + 5][i1] = t5;
					B[j + 6][i1] = t6;
					B[j + 7][i1] = t7;
				}
			}

			else if (j + stride < 64) {
				for (i1 = i; i1 < i + stride; i1++) {
					t = A[i1][j];
					t1 = A[i1][j + 1];
					t2 = A[i1][j + 2];
					t3 = A[i1][j + 3];
					t4 = A[i1][j + 4];
					t5 = A[i1][j + 5];
					t6 = A[i1][j + 6];
					t7 = A[i1][j + 7];

					B[j][i1] = t;
					B[j + 1][i1] = t1;
					B[j + 2][i1] = t2;
					B[j + 3][i1] = t3;
					B[j + stride][i1 + stride - 64] = t4;
					B[j + 1 + stride][i1 + stride - 64] = t5;
					B[j + 2 + stride][i1 + stride - 64] = t6;
					B[j + 3 + stride][i1 + stride - 64] = t7;
				}

				/* step 2 */
				for (i1 = i; i1 < i + stride; i1++) {
					t4 = B[j + stride][i1 + stride - 64];
					t5 = B[j + 1 + stride][i1 + stride - 64];
					t6 = B[j + 2 + stride][i1 + stride - 64];
					t7 = B[j + 3 + stride][i1 + stride - 64];

					B[j + 4][i1] = t4;
					B[j + 5][i1] = t5;
					B[j + 6][i1] = t6;
					B[j + 7][i1] = t7;
				}

			} else {
				for (i1 = i; i1 < i + stride; i1++) {
					t = A[i1][j];
					t1 = A[i1][j + 1];
					t2 = A[i1][j + 2];
					t3 = A[i1][j + 3];
					t4 = A[i1][j + 4];
					t5 = A[i1][j + 5];
					t6 = A[i1][j + 6];
					t7 = A[i1][j + 7];

					B[j][i1] = t;
					B[j + 1][i1] = t1;
					B[j + 2][i1] = t2;
					B[j + 3][i1] = t3;
					B[j + 4][i1] = t4;
					B[j + 5][i1] = t5;
					B[j + 6][i1] = t6;
					B[j + 7][i1] = t7;
				}

			}
		}
	}
}

void result61by67(int A[67][61], int B[61][67]) {
	int stride;
	int i, j, i1;

	stride = 8;
	int t, t1, t2, t3, t4, t5, t6, t7;

	for (i = 0; i < 67; i += stride) {
		for (j = 0; j < 61; j += stride) {
			if (i + stride <= 64 && j + stride <= 56) {
				for (i1 = i; i1 < i + stride; i1++) {

					t = A[i1][j];
					t1 = A[i1][j + 1];
					t2 = A[i1][j + 2];
					t3 = A[i1][j + 3];
					t4 = A[i1][j + 4];
					t5 = A[i1][j + 5];
					t6 = A[i1][j + 6];
					t7 = A[i1][j + 7];

					B[j][i1] = t;
					B[j + 1][i1] = t1;
					B[j + 2][i1] = t2;
					B[j + 3][i1] = t3;
					B[j + 4][i1] = t4;
					B[j + 5][i1] = t5;
					B[j + 6][i1] = t6;
					B[j + 7][i1] = t7;
				}
			}

			else if (i + stride > 64 && j + stride <= 56) {
				for (i1 = i; i1 < 67; i1++) {
					t = A[i1][j];
					t1 = A[i1][j + 1];
					t2 = A[i1][j + 2];
					t3 = A[i1][j + 3];
					t4 = A[i1][j + 4];
					t5 = A[i1][j + 5];
					t6 = A[i1][j + 6];
					t7 = A[i1][j + 7];

					B[j][i1] = t;
					B[j + 1][i1] = t1;
					B[j + 2][i1] = t2;
					B[j + 3][i1] = t3;
					B[j + 4][i1] = t4;
					B[j + 5][i1] = t5;
					B[j + 6][i1] = t6;
					B[j + 7][i1] = t7;
				}
			}

			else if (i + stride <= 64 && j + stride > 56) {
				for (i1 = i; i1 < i + stride; i1++) {
					t = A[i1][j];
					t1 = A[i1][j + 1];
					t2 = A[i1][j + 2];
					t3 = A[i1][j + 3];
					t4 = A[i1][j + 4];

					B[j][i1] = t;
					B[j + 1][i1] = t1;
					B[j + 2][i1] = t2;
					B[j + 3][i1] = t3;
					B[j + 4][i1] = t4;
				}
			}

			else {
				for (i1 = i; i1 < 67; i1++) {
					t = A[i1][j];
					t1 = A[i1][j + 1];
					t2 = A[i1][j + 2];
					t3 = A[i1][j + 3];
					t4 = A[i1][j + 4];

					B[j][i1] = t;
					B[j + 1][i1] = t1;
					B[j + 2][i1] = t2;
					B[j + 3][i1] = t3;
					B[j + 4][i1] = t4;
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
void trans(int M, int N, int A[N][M], int B[M][N]) {
	int i, j, tmp;

	REQUIRES(M > 0);
	REQUIRES(N > 0);

	for (i = 0; i < N; i++) {
		for (j = 0; j < M; j++) {
			tmp = A[i][j];
			B[j][i] = tmp;
		}
	}

	ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
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
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
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

