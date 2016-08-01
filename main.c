/*
 * Copyright (c) 2016 Sugizaki Yukimasa
 * All rights reserved.
 *
 * This software is licensed under a Modified (3-Clause) BSD License.
 * You should have received a copy of this license along with this
 * software. If not, contact the copyright holder above.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <vc4vec.h>

#define NROWS (16 * 80)
#define NCOLS (16 * 80)

//#define DEBUG


const unsigned code[] = {
#include "transpose.qhex"
};
const int code_size = sizeof(code);
const int unif_len = 1024;


int main()
{
	int i, j;
	struct vc4vec_mem mem_out, mem_in, mem_unif, mem_code, mem_out_cpu;
	struct timeval start, end;
	unsigned *p;
	unsigned *p_in, *p_out;
	unsigned *p_cpu, *p_qpu;

	printf("P = %d, Q = %d\n", NROWS, NCOLS);

	vc4vec_init();

	vc4vec_mem_alloc(&mem_out, NCOLS * NROWS * (32 / 8));
	vc4vec_mem_alloc(&mem_in, NROWS * NCOLS * (32 / 8));
	vc4vec_mem_alloc(&mem_unif, unif_len * (32 / 8));
	vc4vec_mem_alloc(&mem_code, code_size);
	vc4vec_mem_alloc(&mem_out_cpu, NCOLS * NROWS * (32 / 8));

	p = mem_unif.cpu_addr;
	*p++ = NROWS / 16;
	*p++ = NCOLS / 16;
	*p++ = mem_in.gpu_addr;
	*p++ = mem_out.gpu_addr;

	memcpy(mem_code.cpu_addr, code, code_size);

	{
		struct timeval st;
		gettimeofday(&st, NULL);
		srandom(st.tv_sec * 1e6 + st.tv_usec);
	}

#ifndef DEBUG
	p = mem_in.cpu_addr;
	for (i = 0; i < NROWS; i ++)
		for (j = 0; j < NCOLS; j ++)
			p[i * NCOLS + j] = random();
#else /* DEBUG */
	p = mem_in.cpu_addr;
	for (i = 0; i < NROWS; i ++)
		for (j = 0; j < NCOLS; j ++)
			p[i * NCOLS + j] = (i + j * 16) % 1000;
#endif /* DEBUG */

#ifdef DEBUG
	p = mem_out.cpu_addr;
	for (i = 0; i < NROWS; i ++)
		for (j = 0; j < NCOLS; j ++)
			p[i * NCOLS + j] = 3;
#endif /* DEBUG */

#ifdef DEBUG
	p = mem_in.cpu_addr;
	for (i = 0; i < NROWS; i ++) {
		printf("%3d:", i);
		for (j = 0; j < NCOLS; j ++) {
			printf(" %3d", p[i * NCOLS + j]);
			if (j % 16 == 16 - 1)
				printf(" ");
		}
		printf("\n");
		if (i % 16 == 16 - 1)
			printf("\n");
	}
#endif /* DEBUG */

	gettimeofday(&start, NULL);
	p_in = mem_in.cpu_addr;
	p_out = mem_out_cpu.cpu_addr;
	for (i = 0; i < NROWS; i ++)
		for (j = 0; j < NCOLS; j ++)
			p_out[j * NROWS + i] = p_in[i * NCOLS + j];
	gettimeofday(&end, NULL);

	printf("CPU: %f [s]\n", (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6);

	gettimeofday(&start, NULL);
	launch_qpu_job_mailbox(1, 1, 3e3, mem_unif.gpu_addr, mem_code.gpu_addr);
	gettimeofday(&end, NULL);

#ifdef DEBUG
	p = mem_out.cpu_addr;
	for (i = 0; i < NCOLS; i ++) {
		printf("%3d:", i);
		for (j = 0; j < NROWS; j ++) {
			printf(" %3d", p[i * NROWS + j]);
			if (j % 16 == 16 - 1)
				printf(" ");
		}
		printf("\n");
		if (i % 16 == 16 - 1)
			printf("\n");
	}
#endif /* DEBUG */

	printf("QPU: %f [s]\n", (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6);

	{
		int maximum_error = 0;
		p_cpu = mem_out_cpu.cpu_addr;
		p_qpu = mem_out.cpu_addr;
		for (i = 0; i < NROWS; i ++) {
			for (j = 0; j < NCOLS; j ++) {
				int error = abs(p_cpu[i * NCOLS + j] - p_qpu[i * NCOLS + j]);
				if (error > maximum_error)
					maximum_error = error;
			}
		}
		printf("Maximum error: %d\n", maximum_error);
	}

	vc4vec_mem_free(&mem_out_cpu);
	vc4vec_mem_free(&mem_code);
	vc4vec_mem_free(&mem_unif);
	vc4vec_mem_free(&mem_in);
	vc4vec_mem_free(&mem_out);

	vc4vec_finalize();

	return 0;
}
