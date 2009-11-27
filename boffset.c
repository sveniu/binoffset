/* Binary file offset tool. License: zlib license. svensven@gmail.com
 *
 * This tool is used to adjust the offset of binary files, mainly with
 * the goal to fix CD audio ripped by a CDROM with a non-zero read
 * offset. You can also negate the offset to produce a file suitable
 * for writing with the same CDROM, assuming it has a non-zero write
 * offset as well.
 *
 *   Usage: ./prog [+|-]<offset> <infile> <outfile>
 *
 * The offset is specified in bytes, not CD audio samples. Multiply CD
 * audio samples by four (16 bits per sample * 2 channels) to get the
 * correct number of bytes. The output file is padded with binary zero
 * (0x00) in the beginning or end depending on offset, so that the
 * file size will be identical to the input file. The input file is
 * essentially shifted back/forth by the specified number of bytes.
 *
 * The same result can be obtained by using dd, but with small offset
 * sizes (which dd will use as block size), the operation will be very
 * slow. This program uses a hardcoded 4kB block size regardless of
 * the specified offset.
 *
 * Examples:
 *   Infile:  0x0102030405060708
 *   ./prog +3 infile outfile
 *   Outfile: 0x0405060708000000
 *
 *   Infile:  0x0102030405060708
 *   ./prog -3 infile outfile
 *   Outfile: 0x0000000102030405
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define BLOCKSIZE 4096

int main (int argc, char **argv)
{
	int len;
	long long offset, fsize;
	FILE *fi, *fo;
	char buffer[BLOCKSIZE];

	if(argc != 4)
	{
		fprintf(stderr, "Usage: %s [+|-]<offset> <infile> <outfile>\n",
				argv[0]);
		return 1;
	}

	offset = atoll(argv[1]);
	if(offset == 0)
	{
		fprintf(stderr, "Zero offset: Nothing to do\n");
		return 0;
	}

	if((fi = fopen(argv[2], "rb")) == NULL)
	{
		perror("Fatal: Input file error");
		return 1;
	}

	/* Determine input file size */
	fseek(fi, 0, SEEK_END);
	fsize = ftell(fi);
	fseek(fi, 0, SEEK_SET);

	if(abs(offset) >= fsize)
	{
		fprintf(stderr, "Fatal: Offset >= input file size\n");
		return 1;
	}

	if((fo = fopen(argv[3], "rb")) != NULL)
	{
		fprintf(stderr, "Fatal: Output file exists\n");
		return 1;
	}

	if((fo = fopen(argv[3], "wb")) == NULL)
	{
		perror("Fatal: Output file error");
		return 1;
	}

	fprintf(stderr, "Using offset: %lld bytes\n", offset);

	/* Negative offset: Pad start of output. Note that
	 * simply using fseek+fwrite produces sparse files.
	 * That should not cause any issues.
	 */
	if(offset < 0)
	{
		memset(buffer, 0, sizeof(buffer));
		fseek(fo, abs(offset)-1, SEEK_SET);
		fwrite(buffer, 1, 1, fo);
	}
	/* Positive offset: Skip start of input */
	else
		fseek(fi, offset, SEEK_SET);

	while((len = fread(buffer, 1, sizeof(buffer), fi)) > 0)
	{
		if(ftell(fo)+sizeof(buffer) > fsize)
			fwrite(buffer, 1, fsize-ftell(fo), fo);
		else
			fwrite(buffer, 1, len, fo);
	}

	/* Positive offset: Pad end of output */
	if(ftell(fo) < fsize)
	{
		memset(buffer, 0, sizeof(buffer));
		fseek(fo, fsize-1, SEEK_SET);
		fwrite(buffer, 1, 1, fo);
	}

	fclose(fi);
	fclose(fo);

	return 0;
}
