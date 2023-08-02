/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include "cachelab.h"
#include <stdio.h>

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void deal_32_32(int M, int N, int A[N][M], int B[M][N]) {
  // i和j枚举出了每个8×8的矩阵的左顶点
  for (int i = 0; i < 32; i += 8) {
    for (int j = 0; j < 32; j += 8) {
      // cnt枚举的是小矩阵的行数
      for (int cnt = 0; cnt < 8; cnt++) {
        int temp1 = A[i + cnt][j];
        int temp2 = A[i + cnt][j + 1];
        int temp3 = A[i + cnt][j + 2];
        int temp4 = A[i + cnt][j + 3];
        int temp5 = A[i + cnt][j + 4];
        int temp6 = A[i + cnt][j + 5];
        int temp7 = A[i + cnt][j + 6];
        int temp8 = A[i + cnt][j + 7];

        B[j][i + cnt] = temp1;
        B[j + 1][i + cnt] = temp2;
        B[j + 2][i + cnt] = temp3;
        B[j + 3][i + cnt] = temp4;
        B[j + 4][i + cnt] = temp5;
        B[j + 5][i + cnt] = temp6;
        B[j + 6][i + cnt] = temp7;
        B[j + 7][i + cnt] = temp8;
      }
    }
  }
}
void deal_64_64(int M, int N, int A[N][M], int B[M][N]) {
  // i和j枚举出了每个8×8的矩阵的左顶点
  for (int i = 0; i < 64; i += 8) {
    for (int j = 0; j < 64; j += 8) {
      int temp1, temp2, temp3, temp4, temp5, temp6, temp7, temp8;
      int cnti, cntj;
      // 现在处理A的左上4×4，顺便操作一下A的右上4×4
      for (cnti = 0; cnti < 4; cnti++) {
        // 取出A的左上，以行的方式
        temp1 = A[i + cnti][j];
        temp2 = A[i + cnti][j + 1];
        temp3 = A[i + cnti][j + 2];
        temp4 = A[i + cnti][j + 3];
        // 取出A的右上，以行的方式
        temp5 = A[i + cnti][j + 4];
        temp6 = A[i + cnti][j + 5];
        temp7 = A[i + cnti][j + 6];
        temp8 = A[i + cnti][j + 7];
        // 将A的左上放到正确的地方，以列的方式
        B[j][i + cnti] = temp1;
        B[j + 1][i + cnti] = temp2;
        B[j + 2][i + cnti] = temp3;
        B[j + 3][i + cnti] = temp4;
        // 将A的右上放到现在B的右上，提前处理一下，省的后面还要访问A的这个cacheline
        B[j][i + cnti + 4] = temp5;
        B[j + 1][i + cnti + 4] = temp6;
        B[j + 2][i + cnti + 4] = temp7;
        B[j + 3][i + cnti + 4] = temp8;
      }
      // 至此，A的前4行已经全部处理完了，命中率约为7/8
      // 上述操作处理完之后，B的前4行都在cache中
      // 现在处理A的左下，将其移到B的右上，同时我们已经把A的右上存在了B的右上
      // 所以要记得取下来，放到B的左下去
      for (cntj = 0; cntj < 4; cntj++) {
        // 取出A的左下，一列一列地取
        temp1 = A[i + 4][j + cntj];
        temp2 = A[i + 5][j + cntj];
        temp3 = A[i + 6][j + cntj];
        temp4 = A[i + 7][j + cntj];
        // 取出当前B的右上，一行一行地取，之后还要一行一行地放到B的左下
        temp5 = B[j + cntj][i + 4];
        temp6 = B[j + cntj][i + 5];
        temp7 = B[j + cntj][i + 6];
        temp8 = B[j + cntj][i + 7];
        // 修改B的右上为真正的值，即A的左下
        B[j + cntj][i + 4] = temp1;
        B[j + cntj][i + 5] = temp2;
        B[j + cntj][i + 6] = temp3;
        B[j + cntj][i + 7] = temp4;
        // 至此B的这一行完全OK了，虽然马上就要被自己干掉，但是也没关系，反正也用不上了
        // 接下来别忘了修改B的左下，本来应该是A的右上，但是我们提前存在了B的右上
        // 现在就是temp5-8
        B[j + cntj + 4][i] = temp5;
        B[j + cntj + 4][i + 1] = temp6;
        B[j + cntj + 4][i + 2] = temp7;
        B[j + cntj + 4][i + 3] = temp8;
      }
      // 最后修改B的右下
      for (cnti = 4; cnti < 8; cnti++) {
        temp1 = A[i + cnti][j + 4];
        temp2 = A[i + cnti][j + 5];
        temp3 = A[i + cnti][j + 6];
        temp4 = A[i + cnti][j + 7];
        B[j + 4][i + cnti] = temp1;
        B[j + 5][i + cnti] = temp2;
        B[j + 6][i + cnti] = temp3;
        B[j + 7][i + cnti] = temp4;
      }
    }
  }
}

#define edge 17
void deal_61_67(int M, int N, int A[N][M], int B[M][N]) {
  for (int i = 0; i < N; i += edge) {
    for (int j = 0; j < M; j += edge) {
      for (int x = i; x < i + edge && x < N; x++) {
        for (int y = j; y < j + edge && y < M; y++) {
          B[y][x] = A[x][y];
        }
      }
    }
  }
}

void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
  if (M == 32) {
    deal_32_32(M, N, A, B);
  } else if (M == 64) {
    deal_64_64(M, N, A, B);
  } else if (M == 61) {
    deal_61_67(M, N, A, B);
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
void registerFunctions() {
  /* Register your solution function */
  registerTransFunction(transpose_submit, transpose_submit_desc);

  /* Register any additional transpose functions */
  //   registerTransFunction(trans, trans_desc);
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