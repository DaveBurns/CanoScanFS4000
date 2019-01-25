#include <stdio.h>

#define NUMFILES 10

int main(int argc, char *argv[])
{
	int i;
	FILE *inputfiles[NUMFILES];
	FILE *outputfile;
	unsigned short int shortavg, samples[NUMFILES];
	unsigned long int avg;

	for (i = 0; i < NUMFILES; i++) {
		char fname[200];

		sprintf(fname, "../image-4000dpi-registration%d.raw", i);
		inputfiles[i] = fopen(fname, "rb");
		if (inputfiles[i] == NULL)
			return -1;
	}

	outputfile = fopen("image-4000dpi-registration-avg.raw", "wb");
	if (outputfile == NULL)
		return -1;

	while (!feof(inputfiles[0])) {
		for (i = 0; i < NUMFILES; i++) {
			fread(&samples[i], sizeof(unsigned short int), 1, inputfiles[i]);
			if (feof(inputfiles[i]))
				break;
		}
		if (i == NUMFILES) { // i.e. if we did not break out of above due to eof
			int j;

			avg = 0;
			for (j = 0; j < NUMFILES; j++)
				avg += samples[j];
			avg /= NUMFILES;

			shortavg = (short)avg;
			fwrite(&shortavg, sizeof(shortavg), 1, outputfile);
		}
	}
	
	for (i = 0; i < NUMFILES; i++)
		fclose(inputfiles[i]);
	fclose(outputfile);

	return 0;
}
