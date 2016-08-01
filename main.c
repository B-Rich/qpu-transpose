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

/*
 * Maximum NROWS and NCOLS: 16 * (2 ** 7 - 1) = 16 * 127 = 2032
 * Memory size to allocate on GPU side: NROWS * NCOLS * (32 / 8) [B]
 * So the maximum memory size is 2032 * 2032 * 4 = 16.5 [MB]
 */

#define NROWS (16 * 127)
#define NCOLS (16 * 127)

//#define DEBUG


const unsigned code[] = {
#include "transpose.qhex"
};
const int code_size = sizeof(code);
const int unif_len = 1024;


int main()
{
	int i, j;
	struct vc4vec_mem mem_out, mem_out_addr, mem_in, mem_unif, mem_code;
	struct timeval start, end;
	unsigned *p;
	unsigned *p_in, *p_out;
	float time, elems_per_sec;

	if (
		   (NROWS % 16 != 0)
		|| (NCOLS % 16 != 0)
		|| ((NROWS * 4) & ~((1 << (12 + 1)) - 1))
		|| ((NCOLS * 4) & ~((1 << (12 + 1)) - 1))
	) {
		fprintf(stderr, "error: Invalid values for NROWS and NCOLS\n");
		return 1;
	}

	printf("P = %d, Q = %d\n", NROWS, NCOLS);

	vc4vec_init();

	vc4vec_mem_alloc(&mem_out, NCOLS * NROWS * (32 / 8));
	vc4vec_mem_alloc(&mem_out_addr, 1 * (32 / 8));
	vc4vec_mem_alloc(&mem_in, NROWS * NCOLS * (32 / 8));
	vc4vec_mem_alloc(&mem_unif, unif_len * (32 / 8));
	vc4vec_mem_alloc(&mem_code, code_size);

	p = mem_out_addr.cpu_addr;
	*p++ = mem_out.gpu_addr;

	p = mem_unif.cpu_addr;
	*p++ = NROWS / 16;
	*p++ = NCOLS / 16;
	*p++ = mem_in.gpu_addr;
	*p++ = mem_out_addr.gpu_addr;

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
	p_out = mem_out.cpu_addr;
	for (i = 0; i < NROWS; i ++)
		for (j = 0; j < NCOLS; j ++)
			p_out[j * NROWS + i] = p_in[i * NCOLS + j];
	gettimeofday(&end, NULL);

	time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6;
	elems_per_sec = (NROWS * NCOLS) / time;
	printf("CPU: %f [s]  %g [element/s]\n", time, elems_per_sec);

	p = mem_out.cpu_addr;
	for (i = 0; i < NROWS; i ++)
		for (j = 0; j < NCOLS; j ++)
			p[i * NCOLS + j] = 0;

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

	time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6;
	elems_per_sec = (NROWS * NCOLS) / time;
	printf("QPU: %f [s]  %g [element/s]\n", time, elems_per_sec);

	{
		int maximum_error = 0;
		p = mem_out.cpu_addr;
		p_in = mem_in.cpu_addr;
		for (i = 0; i < NROWS; i ++) {
			for (j = 0; j < NCOLS; j ++) {
				int error = abs(p_in[j * NROWS + i] - p[i * NCOLS + j]);
				if (error > maximum_error)
					maximum_error = error;
			}
		}
		printf("Maximum error: %d\n", maximum_error);
	}

	vc4vec_mem_free(&mem_code);
	vc4vec_mem_free(&mem_unif);
	vc4vec_mem_free(&mem_in);
	vc4vec_mem_free(&mem_out_addr);
	vc4vec_mem_free(&mem_out);

	vc4vec_finalize();

	return 0;
}
