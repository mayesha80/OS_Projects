#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "matmul.h"

int **A, **B, **B_tran, **C_single, **C_multi_proc, **C_multi_thread;
int m, n, p;
int crashRate=0;

int main(int argc, char **argv) 
{
		struct timeval  t_begin, t_end;
		srand(time(NULL));
		if(argc < 2) {
				printf("usage: %s <filename> [crash rate]\n", argv[0]);
				return 0;
		}
		
		/** 1st arg: file name that contains matrices **/
		/** 2nd (optional) arg: crash rate between 0% and 30%. 
			* Each child process has that much chance to crash.
			**/
		if(argc > 2) {
				crashRate = atoi(argv[2]);
				if(crashRate < 0) crashRate = 0;
				if(crashRate > 30) crashRate = 30; 
printf("Child processes' crash rate: %d\%\n", crashRate);

		} else crashRate = 0;

/** Read matrices from the file **/
		read_matrix(argv[1]);


/** Single-process matrix multiplication **/
		gettimeofday(&t_begin, NULL);
		mat_mult_single();
		gettimeofday(&t_end, NULL);
		
		printf("================================================\n");
		printf("Serial multiplication took %f seconds.\n",
		         (double) (t_end.tv_usec - t_begin.tv_usec) / 1000000 +
											(double) (t_end.tv_sec - t_begin.tv_sec));
		
		if(m < 10 && p < 10) {
				printf("The result from the serial calculation: \n");
				print_matrix(C_single, m, p);
		}
		printf("================================================\n\n");



/** Multi-process matrix multiplication **/
		gettimeofday(&t_begin, NULL);
		mat_mult_multi_proc(m);
		gettimeofday(&t_end, NULL);
		
		printf("================================================\n");
		printf("Multi_process multiplication took %f seconds.\n",
		         (double) (t_end.tv_usec - t_begin.tv_usec) / 1000000 +
											(double) (t_end.tv_sec - t_begin.tv_sec));
		
		if(m < 10 && p < 10) {
				printf("The result from the multi_process calculation: \n");
				print_matrix(C_multi_proc, m, p);
		}
		printf("================================================\n");




/** Multi-thread matrix multiplication **/
		gettimeofday(&t_begin, NULL);
		mat_mult_multi_thread(m, crashRate);
		gettimeofday(&t_end, NULL);
		
		printf("================================================\n");
		printf("Multi_threaded multiplication took %f seconds.\n",
		         (double) (t_end.tv_usec - t_begin.tv_usec) / 1000000 +
											(double) (t_end.tv_sec - t_begin.tv_sec));
		
		if(m < 10 && p < 10) {
				printf("The result from the multi_threaded calculation: \n");
				print_matrix(C_multi_thread, m, p);
		}
		printf("================================================\n");




/** Compare the results **/
		if(compare_matrices(C_single, C_multi_proc, m, p) == -1)
		{
				printf("** Single and multi_proc results are NOT matched.**\n");
		} else {
				printf("** Single and multi_proc results are matched. **\n");
		}

		printf("================================================\n");

		if(compare_matrices(C_single, C_multi_thread, m, p) == -1)
		{
				printf("** Single and multi_thread results are NOT matched.**\n");
		} else {
				printf("** Single and multi_thread results are matched. **\n");
		}

		printf("================================================\n");

		if(compare_matrices(C_multi_proc, C_multi_thread, m, p) == -1)
		{
				printf("** Multi_process and multi_thread results are NOT matched.**\n");
		} else {
				printf("** Multi_process and multi_thread results are matched. **\n");
		}

		printf("================================================\n");

		return 0;
}

void mat_mult_single()
{
		int i, j, k;

		for(i = 0; i < m; i++){
				for(j = 0; j < p; j++){
						C_single[i][j] = linear_mult(A[i], B_tran[j], n);
				}
		}
}



