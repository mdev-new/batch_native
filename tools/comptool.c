#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef int (*CompressFun)(uint8_t *input, uint8_t *output, uint32_t input_length);

#include "../src/shared.h"

#define SDEFL_IMPLEMENTATION
#include "deflalgo.h"

#include "lz77algo.h"

int noCompress(uint8_t *input, uint8_t *output, uint32_t input_length) {
	memcpy(output, input, input_length);
	return input_length;
}

uint64_t compressions[][3] = {
	{UNCOMPRESSED, 0, noCompress},
	{ALGO_LZ77, LZ77_MATCH_LENGTH_BITS, lz77comp},
	{ALGO_DEFLATE, 0, deflcomp}
};

// todo error check?
int main(int argc, char *argv[]) {
	if(argc < 3) return;

	int algo = atol(argv[1]);
	uint32_t type = compressions[algo][0],
			 bits = compressions[algo][1];

	CompressFun compressPtr = compressions[algo][2];

	FILE *fptr_r = fopen(argv[2], "rb");
	FILE *fptr_w = fopen(argv[3], "wb");

	fseek(fptr_r, 0, SEEK_END);
	int inlen = ftell(fptr_r);
	fseek(fptr_r, 0, SEEK_SET);

	char *inBuffer = alloca(inlen);
	char *outBuffer = alloca(inlen*4 + sizeof(Header));
	fread(inBuffer, inlen, 1, fptr_r);

	Header header = {
		.magic = MAGIC,
		.sizeOfSelf = sizeof(Header),
		.headerVersion = VERSION,
		.compression = (bits << 27) | type,
		.uncompressed_file_size = inlen
	};

	// write the footer
	memcpy(outBuffer, &header, sizeof(Header));

	int res = compressPtr(inBuffer, outBuffer+sizeof(Header), inlen);
	fwrite(outBuffer,res+sizeof(Header), 1, fptr_w);

	fclose(fptr_r);
	fclose(fptr_w);

	return 0;
}
