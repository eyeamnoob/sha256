#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

// constants
#define __DEBUG__ 0
#define CHUNK_SIZE 64
#define MESSAGE_SIZE 11 * CHUNK_SIZE
#define INPUT_LEN_MAX MESSAGE_SIZE - CHUNK_SIZE // - CHUNK_SIZE because we can process at most 11 chunks
const uint32_t K[64] = {
	0x428a2f98,
	0x71374491,
	0xb5c0fbcf,
	0xe9b5dba5,
	0x3956c25b,
	0x59f111f1,
	0x923f82a4,
	0xab1c5ed5,
	0xd807aa98,
	0x12835b01,
	0x243185be,
	0x550c7dc3,
	0x72be5d74,
	0x80deb1fe,
	0x9bdc06a7,
	0xc19bf174,
	0xe49b69c1,
	0xefbe4786,
	0x0fc19dc6,
	0x240ca1cc,
	0x2de92c6f,
	0x4a7484aa,
	0x5cb0a9dc,
	0x76f988da,
	0x983e5152,
	0xa831c66d,
	0xb00327c8,
	0xbf597fc7,
	0xc6e00bf3,
	0xd5a79147,
	0x06ca6351,
	0x14292967,
	0x27b70a85,
	0x2e1b2138,
	0x4d2c6dfc,
	0x53380d13,
	0x650a7354,
	0x766a0abb,
	0x81c2c92e,
	0x92722c85,
	0xa2bfe8a1,
	0xa81a664b,
	0xc24b8b70,
	0xc76c51a3,
	0xd192e819,
	0xd6990624,
	0xf40e3585,
	0x106aa070,
	0x19a4c116,
	0x1e376c08,
	0x2748774c,
	0x34b0bcb5,
	0x391c0cb3,
	0x4ed8aa4a,
	0x5b9cca4f,
	0x682e6ff3,
	0x748f82ee,
	0x78a5636f,
	0x84c87814,
	0x8cc70208,
	0x90befffa,
	0xa4506ceb,
	0xbef9a3f7,
	0xc67178f2,
};

// globals
static unsigned char *message = NULL;
static uint64_t chunks_count = 0;
static bool is_end_of_file = false;
static uint32_t H[8] = {
	0x6a09e667,
	0xbb67ae85,
	0x3c6ef372,
	0xa54ff53a,
	0x510e527f,
	0x9b05688c,
	0x1f83d9ab,
	0x5be0cd19,
};

// macros
#define CONVERT_ENDIAN64(src, dest) \
	dest = ((src & 0xff) << (8 * 7)) | ((src & 0xff00) << (5 * 8)) | ((src & 0xff0000) << (3 * 8)) | ((src & 0xff000000) << (1 * 8)) | ((src & 0xff00000000) >> (1 * 8)) | ((src & 0xff0000000000) >> (3 * 8)) | ((src & 0xff000000000000) >> (5 * 8)) | ((src & 0xff00000000000000) >> (7 * 8))
#define CONVERT_ENDIAN32(src, dest) \
	dest = ((src & 0xff) << (8 * 3)) | ((src & 0xff00) << (1 * 8)) | ((src & 0xff0000) >> (1 * 8)) | ((src & 0xff000000) >> (3 * 8))

void read_stdin()
{
	// TODO: handle big size
	fgets(message, MESSAGE_SIZE, stdin);
	uint64_t org_input_len = strlen(message) - 1; // -1 for \n when reading from stdin
	uint64_t org_input_lenb = org_input_len * 8;  // org_input_len in bits
	uint64_t message_len_be;					  // message length in big endian

	message[org_input_len] = 0b10000000; // append a single '1'

	uint64_t zero_padd = (unsigned)(CHUNK_SIZE - 8 - (org_input_len + 1)) % CHUNK_SIZE;
#if __DEBUG__ == 1
	printf("padding: %llu\n", zero_padd * 8);
#endif

	memset(message + org_input_len + 1, 0, zero_padd); // apply padding

	CONVERT_ENDIAN64(org_input_lenb, message_len_be);

	memcpy(message + org_input_len + 1 + zero_padd, &message_len_be, sizeof(message_len_be)); // append message length

	chunks_count = (org_input_len + 1 + zero_padd + 8) / CHUNK_SIZE; // +8 for original message length
#if __DEBUG__ == 1
	printf("chunks count: %u\n", chunks_count);
#endif
}

static void __print_var_bytes(const char *var_name, uint64_t var, uint8_t size)
{
	unsigned char *step = (unsigned char *)&var;
	printf("%s: ", var_name);
	for (int i = 0; i < size; i++)
	{
		printf("%02x ", step[i]);
	}
	printf("\n");
}
#define print_var_bytes(var_name, var) __print_var_bytes(var_name, var, sizeof(var))

static uint32_t sigma0(uint32_t word)
{
	uint32_t p1, p2, p3;
	uint32_t dropped_bits1 = word & 0b1111111u;			   // 7 bits
	uint32_t dropped_bits2 = word & 0b111111111111111111u; // 18 bits

	p1 = (word >> 7) & ~((dropped_bits1) << (32 - 7)) | (dropped_bits1 << (32 - 7));
	p2 = (word >> 18) & ~((dropped_bits2) << (32 - 18)) | (dropped_bits2 << (32 - 18));
	p3 = word >> 3;

	return p1 ^ p2 ^ p3;
}

static uint32_t sigma1(uint32_t word)
{
	uint32_t p1, p2, p3;
	uint32_t dropped_bits1 = word & 0b11111111111111111u;	// 17 bits
	uint32_t dropped_bits2 = word & 0b1111111111111111111u; // 19 bits

	p1 = (word >> 17) & ~((dropped_bits1) << (32 - 17)) | (dropped_bits1 << (32 - 17));
	p2 = (word >> 19) & ~((dropped_bits2) << (32 - 19)) | (dropped_bits2 << (32 - 19));
	p3 = word >> 10;

	return p1 ^ p2 ^ p3;
}

static uint32_t sigma2(uint32_t word)
{
	uint32_t p1, p2, p3;
	uint32_t dropped_bits1 = word & 0b11u;					   // 2 bits
	uint32_t dropped_bits2 = word & 0b1111111111111u;		   // 13 bits
	uint32_t dropped_bits3 = word & 0b1111111111111111111111u; // 22 bits

	p1 = (word >> 2) & ~((dropped_bits1) << (32 - 2)) | (dropped_bits1 << (32 - 2));
	p2 = (word >> 13) & ~((dropped_bits2) << (32 - 13)) | (dropped_bits2 << (32 - 13));
	p3 = (word >> 22) & ~((dropped_bits3) << (32 - 22)) | (dropped_bits3 << (32 - 22));

	return p1 ^ p2 ^ p3;
}

static uint32_t sigma3(uint32_t word)
{
	uint32_t p1, p2, p3;
	uint32_t dropped_bits1 = word & 0b111111u;					  // 6 bits
	uint32_t dropped_bits2 = word & 0b11111111111u;				  // 11 bits
	uint32_t dropped_bits3 = word & 0b1111111111111111111111111u; // 25 bits

	p1 = (word >> 6) & ~((dropped_bits1) << (32 - 6)) | (dropped_bits1 << (32 - 6));
	p2 = (word >> 11) & ~((dropped_bits2) << (32 - 11)) | (dropped_bits2 << (32 - 11));
	p3 = (word >> 25) & ~((dropped_bits3) << (32 - 25)) | (dropped_bits3 << (32 - 25));

	return p1 ^ p2 ^ p3;
}

static void hex_digest()
{
	for (int i = 0; i < sizeof(H) / sizeof(H[0]); i++)
	{
		printf("%08x", H[i]);
	}
	printf("\n");
}

static void chunk_loop()
{
	unsigned char chunk[CHUNK_SIZE * 4] = {0}; // 64 entries such that each entry is 32 bit.
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
	uint32_t e;
	uint32_t f;
	uint32_t g;
	uint32_t h;
	for (int i = 0; i < chunks_count; i += 1)
	{
		memcpy(chunk, message + (i * CHUNK_SIZE), sizeof(char) * CHUNK_SIZE);

		for (int j = 16; j < CHUNK_SIZE; j++)
		{
			uint32_t w0, w1, w2, w3, *wr; // wr = w[j-16] + w[j-7] + sigama0 + sigma1

			wr = (uint32_t *)&chunk[4 * j];
			w0 = *((uint32_t *)&chunk[4 * (j - 16)]);
			w1 = *((uint32_t *)&chunk[4 * (j - 7)]);
			w2 = *((uint32_t *)&chunk[4 * (j - 15)]);
			w3 = *((uint32_t *)&chunk[4 * (j - 2)]);

#if __DEBUG__ == 1

			print_var_bytes("w0", w0);
			print_var_bytes("w1", w1);
			print_var_bytes("w2", w2);
			print_var_bytes("w3", w3);
#endif

			CONVERT_ENDIAN32(w0, w0);
			CONVERT_ENDIAN32(w1, w1);
			CONVERT_ENDIAN32(w2, w2);
			CONVERT_ENDIAN32(w3, w3);

			/* Debug
			print_var_bytes("w0", w0);
			print_var_bytes("w1", w1);
			print_var_bytes("w2", w2);
			print_var_bytes("w3", w3);
			print_var_bytes("sigma0", sigma0(w2));
			print_var_bytes("sigma1", sigma1(w3));
			Debug */

			*wr = w0 + w1 + sigma0(w2) + sigma1(w3);

			CONVERT_ENDIAN32(*wr, *wr);
#if __DEBUG__ == 1
			print_var_bytes("wr", *wr);
#endif
		}

		// initialize working variables
		a = H[0];
		b = H[1];
		c = H[2];
		d = H[3];
		e = H[4];
		f = H[5];
		g = H[6];
		h = H[7];

		// update working variable
		for (int k = 0; k < 64; k++)
		{
			uint32_t chunk_i = (*((uint32_t *)&chunk[k * 4])); // due to endians, we define a var and copy word from chunk to here
			CONVERT_ENDIAN32(chunk_i, chunk_i);

			uint32_t choice = (e & f) ^ ((~e) & g);
			uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
			uint32_t temp1 = h + sigma3(e) + choice + K[k] + chunk_i;
			uint32_t temp2 = sigma2(a) + majority;

			h = g;
			g = f;
			f = e;
			e = d + temp1;
			d = c;
			c = b;
			b = a;
			a = temp1 + temp2;

#if __DEBUG__ == 1
			print_var_bytes("a", a);
			print_var_bytes("b", b);
			print_var_bytes("c", c);
			print_var_bytes("d", d);
			print_var_bytes("e", e);
			print_var_bytes("f", f);
			print_var_bytes("g", g);
			print_var_bytes("h", h);
#endif
		}
		H[0] += a;
		H[1] += b;
		H[2] += c;
		H[3] += d;
		H[4] += e;
		H[5] += f;
		H[6] += g;
		H[7] += h;
	}
}

static void read_file(const char *file_name)
{
	FILE *file = fopen(file_name, "rb");
	static size_t file_offset = 0;

	if (!file)
	{
		fprintf(stderr, "can not open file'%s' error: %s\n", file_name, strerror(errno));
		exit(EXIT_FAILURE);
	}

	fseek(file, 0, SEEK_END);
	size_t file_len = ftell(file);
	fseek(file, file_offset, SEEK_SET);

	// if (file_len > INPUT_LEN_MAX)
	// {
	// 	fprintf(stderr, "can not read file bigger than %llu bytes\n", INPUT_LEN_MAX);
	// 	exit(EXIT_FAILURE);
	// }

	uint64_t org_input_len = fread(message, sizeof(unsigned char), INPUT_LEN_MAX, file);
	uint64_t message_len_be; // message length in big endian

	file_offset += org_input_len;
	is_end_of_file = file_offset == file_len;

	uint64_t zero_padd = 0;
	if (is_end_of_file)
	{
		message[org_input_len] = 0b10000000; // append a single '1'
		zero_padd = (unsigned)(CHUNK_SIZE - 8 - (org_input_len + 1)) % CHUNK_SIZE;
		chunks_count = (org_input_len + 1 + zero_padd + 8) / CHUNK_SIZE; // +8 for original message length
		memset(message + org_input_len + 1, 0, zero_padd);				 // apply padding
		file_len *= 8;
		CONVERT_ENDIAN64(file_len, message_len_be);
		memcpy(message + org_input_len + 1 + zero_padd, &message_len_be, sizeof(message_len_be)); // append message length
	}
	else
	{
		chunks_count = (INPUT_LEN_MAX) / CHUNK_SIZE;
	}

#if __DEBUG__ == 1
	printf("offset: %llu\n", file_offset);
	printf("chunks count: %llu\n", chunks_count);
	printf("padding: %llu\n", zero_padd * 8);
#endif

	fclose(file);
}

void calc_file_hash(const char *file_name)
{
	do
	{
		read_file(file_name);
		chunk_loop();
	} while (!is_end_of_file);

	hex_digest();
}

int main(int argc, char const *argv[])
{
	message = (char *)malloc(sizeof(unsigned char) * MESSAGE_SIZE);
	chunks_count = 0;

	if (argc == 1)
	{
		read_stdin();
	}
	else if (argc == 2)
	{
		calc_file_hash(argv[1]);
	}
	else
	{
		printf("usage:\n%s <file_name>\t read from a file\n%s\t read from stdin\n", argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}

#if __DEBUG__ == 1
	for (int i = 0; i < 64; i++)
	{
		printf("%02x ", message[i]);
	}
	printf("\n");
#endif

	return 0;
}
