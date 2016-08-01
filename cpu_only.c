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
#include <stdint.h>
#include <assert.h>
#include <sys/time.h>

/*
 * Maximum NROWS and NCOLS: 16 * (2 ** 7 - 1) = 16 * 127 = 2032
 * Memory size to allocate on GPU side: NROWS * NCOLS * (32 / 8) [B]
 * So the maximum memory size is 2032 * 2032 * 4 = 16.5 [MB]
 */

#define NROWS (16 * 127)
#define NCOLS (16 * 127)

int main()
{
	int i, j;
	struct timeval start, end;
	int32_t *p_in, *p_out;
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

	p_in = malloc(NROWS * NCOLS * (32 / 8));
	assert(p_in != NULL);
	p_out = malloc(NROWS * NCOLS * (32 / 8));
	assert(p_out != NULL);

	{
		struct timeval st;
		gettimeofday(&st, NULL);
		srandom(st.tv_sec * 1e6 + st.tv_usec);
	}

	for (i = 0; i < NROWS; i ++)
		for (j = 0; j < NCOLS; j ++)
			p_in[i * NCOLS + j] = random();

	gettimeofday(&start, NULL);
	for (i = 0; i < NROWS; i ++)
		for (j = 0; j < NCOLS; j ++)
			p_out[j * NROWS + i] = p_in[i * NCOLS + j];
	gettimeofday(&end, NULL);

	time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6;
	elems_per_sec = (NROWS * NCOLS) / time;
	printf("CPU: %f [s]  %g [element/s]\n", time, elems_per_sec);

	((volatile int32_t*) p_out)[random() % (NROWS * NCOLS)];

	return 0;
}
