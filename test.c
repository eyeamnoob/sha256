#include <stdio.h>
#include <stdint.h>
#define CONVERT_ENDIAN64(src, dest)                                                                                                                                                                                                                                                                     \
	do                                                                                                                                                                                                                                                                                                \
	{                                                                                                                                                                                                                                                                                                 \
		dest = ((src & 0xff) << (8 * 7)) | ((src & 0xff00) << (5 * 8)) | ((src & 0xff0000) << (3 * 8)) | ((src & 0xff000000) << (1 * 8)) | ((src & 0xff00000000) >> (1 * 8)) | ((src & 0xff0000000000) >> (3 * 8)) | ((src & 0xff000000000000) >> (5 * 8)) | ((src & 0xff00000000000000) >> (7 * 8)); \
	} while (0)

int main(int argc, char const *argv[])
{
	uint64_t x = 0xafbfcfdfef1f2f3f;
	uint64_t y;
	uint64_t c;
	unsigned char *cx = (char *)&x;
	unsigned char *cy = (char *)&y;
	unsigned char *cc = (char *)&c;

	printf("0x%llx\n", x);
	for (int i = 0; i < 8; i++)
	{
		printf("%p: 0x%llx\n", (cx + i), *(cx + i));
	}

	CONVERT_ENDIAN(x, y);
	printf("0x%llx\n", y);
	for (int i = 0; i < 8; i++)
	{
		printf("%p: 0x%llx\n", (cy + i), *(cy + i));
	}

	CONVERT_ENDIAN(y, c);
	printf("0x%llx\n", c);
	for (int i = 0; i < 8; i++)
	{
		printf("%p: 0x%llx\n", (cc + i), *(cc + i));
	}

	return 0;
}
