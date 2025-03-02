/** Global variables **/
/** Matrices: C = A * B 
	* B_tran is a transposed matrix of B,	which flips a matrix over its diagonal.
	* B_tran[i][j] = B[j][i]
	* We use the trnasposed matrix B for easier matrix multiplication.
	*
	* C_single stores the result from the single process calcuation.
	* C_multi_proc stores the result from the multi-process calculation.
	* C_multi_thread stores the result from the multi-thread calculation.
	* We compare C_single, C_multi_proc, and C_multi_thread at the end of the exeuciton, and they should match.
	**/

extern int **A, **B, **B_tran, **C_single, **C_multi_proc, **C_multi_thread;

/** matrix size: 
	* A = m x n, B = n x p, B_tran = p x n, C = m x p
	**/
extern int m, n, p;
extern int crashRate;

/** function definition **/
void init_matrix();
void alloc_matrix();
void matrix_tranpose();
int linear_mult(int *col_a, int *row_b, int n);

void read_matrix(char *fname);
void print_matrix(int **mat, int l, int m);
int compare_matrices(int **mat1, int **mat2, int m, int n);
void simulate_process_crash(int i);
void simulate_thread_crash(int i);

void mat_mult_single();
void mat_mult_multi_proc(int numProc);
void mat_mult_multi_thread(int numThread, int crashRate);
