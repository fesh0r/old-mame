#ifndef CASSETTE_H
#define CASSETTE_H

#include "driver.h"

struct cassette_args
{
	/* Args that always apply whenever a cassette is there */
	int initial_status;

	/* Args to use when the wave file is already there */
	int (*fill_wave)(INT16 *buffer, int length, UINT8 *bytes);
	void (*calc_chunk_info)(void *file, int *chunk_size, int *chunk_samples);
	int input_smpfreq;
    int header_samples;
    int trailer_samples;
    int chunk_size;		/* used if calc_chunk_info is NULL */
	int chunk_samples;	/* used if calc_chunk_info is NULL */

	/* Args to use when the wave file is being created */
	int create_smpfreq;
};

int cassette_init(int id, const struct cassette_args *args);

#endif /* CASSETTE_H */
