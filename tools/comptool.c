#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef int (*CompressFun)(uint8_t *input, uint8_t *output, uint32_t input_length);

#include "../src/shared.h"

#define USE_DEFLATE

#if defined(DONT_COMPRESS)
	int func(uint8_t *input, uint8_t *output, uint32_t input_length) {
		memcpy(output, input, input_length);
	}
	char type = UNCOMPRESSED;
	char bits = 0;
	CompressFun compressPtr = func;
#elif defined(USE_LZ77)
	#include "lz77algo.h"
	char type = ALGO_LZ77;
    char bits = LZ77_MATCH_LENGTH_BITS;
	CompressFun compressPtr = lz77comp;
#elif defined(USE_DEFLATE)
	#define SDEFL_IMPLEMENTATION
	#include "deflalgo.h"
	char type = ALGO_DEFLATE;
	char bits = 0;
	CompressFun compressPtr = deflcomp;
#endif

// todo error check?
int main(int argc, char *argv[]) {
	FILE *fptr_r = fopen(argv[1], "rb");
	FILE *fptr_w = fopen(argv[2], "wb");

    fseek(fptr_r, 0, SEEK_END);
    int inlen = ftell(fptr_r);
    fseek(fptr_r, 0, SEEK_SET);

	char *inBuffer = malloc(inlen);
	char *outBuffer = malloc(inlen*2 + sizeof(Footer));
	fread(inBuffer, inlen, 1, fptr_r);
	int res = compressPtr(inBuffer, outBuffer, inlen);

	Footer footer = {
		.compression_type = type,
		.compressor_config = bits,
		.uncompressed_file_size = inlen,
		.magic = MAGIC
	};

	// write the footer
    memcpy(outBuffer+res+0, &footer, sizeof(Footer));
	fwrite(outBuffer,res+sizeof(Footer), 1, fptr_w);

	free(inBuffer);
	free(outBuffer);
	fclose(fptr_r);
	fclose(fptr_w);

	return res;
}