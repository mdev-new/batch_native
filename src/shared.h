#pragma once

// This needs to be set in stone.
// That means no reordering, and no removal of stuff and no changing types, etc.

enum {
	UNCOMPRESSED,
	ALGO_LZ77,
	ALGO_DEFLATE
};

typedef struct {
	char compression_type;
	short compressor_config;
	long uncompressed_file_size;
	short magic;
} __attribute__((__packed__)) Footer;

#define MAGIC 'kM' // in file its in reverse order