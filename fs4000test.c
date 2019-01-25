/* fs4000test.cpp */

#include <windows.h>
#include <stdio.h>
#include "fs4000-scsi.h"
#include "scsi_wrappers.h"

#include <tiffio.h>

/* test_lamp was written to help figure out the set/get lamp commands. */
void test_lamp()
{
	int i;

	fs4000_set_lamp(1, 0);
	for (i = 0; i < 5; i++) {
		fs4000_get_lamp(NULL, NULL, NULL, NULL);
		Sleep(900);
	}
	fs4000_set_lamp(0, 0);

	Sleep(1000);

	fs4000_set_lamp(1, 0);
	for (i = 0; i < 5; i++) {
		fs4000_get_lamp(NULL, NULL, NULL, NULL);
		Sleep(700);
	}
	fs4000_set_lamp(0, 0);
}

/* test_move_pos was written to test move_pos parameters and the interaction with the execute_afae command */
void test_move_pos()
{
	unsigned int i;

	fs4000_set_lamp(1, 0);
	fs4000_get_film_status(0, NULL, NULL, NULL, NULL);
	fs4000_move_position(1, 4, 292);
	for (i = 292; i < 1072; i += 50) {
		fs4000_execute_afae(2, 0, 0, 0, 2000, 3500);
		fs4000_get_film_status(0, NULL, NULL, NULL, NULL);
		printf("\n");
	}
	fs4000_set_lamp(0, 0);
}

/* test_set_frame was written just to find out legal values for the parameter. Changed it while stepping through in a debugger. */
void test_set_frame()
{
    int valid_values[10] = { 0, 1, 2, 3, 4, 8, 9, 10, 11, 12 };
    int i;

	fs4000_set_lamp(1, 0);
	fs4000_move_position(0, 0, 0);
	fs4000_move_position(1, 0, 0);
    for (i = 0; i < 10; i++) {
		fs4000_set_frame(valid_values[i]);
        fs4000_get_film_status(0, NULL, NULL, NULL, NULL);
        fs4000_get_film_status(1, NULL, NULL, NULL, NULL);
        fs4000_get_scan_mode(0, NULL, NULL);
        fs4000_get_scan_mode(1, NULL, NULL);
		Sleep(1000);
        printf("\n");
	}
}

/* test_scan_modes was written to find out legal values for the parameters. Changed them while stepping through in a debugger. */
void test_scan_modes()
{
	FS4000_DEFINE_SCAN_MODE_DATA_OUT define_scan_mode_data_out = { 0, 0, 0, 0, 0x20, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
																	0x3e, 0x3e, 0x3e, 0x115, 0x115, 0x115, 0x3e8, 0x3e8, 0x3e8, 0 };
	FS4000_GET_SCAN_MODE_DATA_IN_38 get_scan_mode_data_in_38;

	while (1) {
		fs4000_define_scan_mode(&define_scan_mode_data_out);
		fs4000_get_scan_mode(0, &get_scan_mode_data_in_38, NULL);
		printf("\n");
	}
}

/* test_do_little_scan was written to check scan mode RGB values and log the
   image data returned from a 1x1 window which could then be graphed in Excel.
   It's been modified and hacked for various other tests since then. */
void test_do_little_scan()
{
	FS4000_DEFINE_SCAN_MODE_DATA_OUT define_scan_mode_data_out = { 0, 0, 0, 0, 0x20, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
																	47, 36, 36, 281, 264, 261, 750, 352, 235,
																	0 };
	PIXEL data_buffer[1000];
	UINT4 num_blocks;
	UINT4 num_bytes_per_block;
	int i, j, k;
//	FILE *f;

	memset(data_buffer, 0, sizeof(data_buffer));
	fs4000_extended_inquiry(NULL);
	fs4000_get_film_status(0, NULL, NULL, NULL, NULL);
	fs4000_set_lamp(1, 0);

	fs4000_debug = 0;

//	f = fopen("def-scan-mode-curves", "w");

	k = 750;
//	for (k = 0; k <= 1000; k += 250) {
		for (j = 0; j <= 10; j += 1) {
			i = 61;
//			for (i = 0; i <= 63; i += 3) {
				fs4000_set_frame(12);
				define_scan_mode_data_out.unknown3[0] = i;
				define_scan_mode_data_out.unknown3[1] = i;
				define_scan_mode_data_out.unknown3[2] = i;
				define_scan_mode_data_out.unknown4[0] = j;
				define_scan_mode_data_out.unknown4[1] = j;
				define_scan_mode_data_out.unknown4[2] = j;
				define_scan_mode_data_out.unknown5[0] = k;
				define_scan_mode_data_out.unknown5[1] = k;
				define_scan_mode_data_out.unknown5[2] = k;
				fs4000_define_scan_mode(&define_scan_mode_data_out);
				fs4000_set_window(4000, 4000, 0, 0, 1, 1, 14, 5);
				fs4000_scan();
				fs4000_get_data_status(&num_blocks, &num_bytes_per_block);
				fs4000_read(num_bytes_per_block, data_buffer);
				fs4000_set_frame(0);
				{
					short int red, green, blue;
					short int *p;

					p = (short int *)&data_buffer[240];
					red = *p;
					green = *++p;
					blue = *++p;

					printf("%5d\t%5d\t%5d\t%5d\t%5d\t%5d\n", k, j, i, red, green, blue);
//					fprintf(f, "%5d\t%5d\t%5d\t%5d\n", k, j, i, red);
					//printf("red   = %5x\t%d\n", red, red);
					//printf("green = %5x\t%d\n", green, green);
					//printf("blue  = %5x\t%d\n", blue, blue);
				}
//			}
		}
//	}

//	fclose(f);
	fs4000_debug = 1;

	fs4000_set_lamp(0, 0);
	fs4000_extended_inquiry(NULL);
}


/* read_from_fs4000: called after a scan is started to do the reading. The
   overall point is to read data in as large chunks as possible to avoid
   stalling the scanner. That is, if one reads only one scanline at a time
   from the scanner, the overhead involved is so high that the scanner will
   scan faster than the data can be read and eventually has to pause scanning
   until enough of its internal buffer has freed up.

   This routine also makes use of libtiff to write out a TIFF file. If you want
   to avoid libtiff, it's possible to comment those lines out and uncomment the
   simpler stdio fopen()/fwrite()/fclose() routines still here and then open that
   raw file in Photoshop. That got tedious quickly, hence the addition of TIFF support.

   This code is a bit messy and could be cleaned up. The constants 240 and 6 are used
   a lot. 6 is the number of byters per pixel and 240 is the number of bytes in the
   40-pixel margin that is in every scanline (see doc for READ in fs4000-scsi.h).
*/
int read_from_fs4000(UINT4 num_scanlines,
					 UINT4 num_bytes_per_scanline,
					 UINT2 dpi,
					 const char *filename)
{
	UINT4 max_bytes_per_read;
	UINT4 max_scanlines_per_read;
	UINT4 num_scanlines_left;
	UINT4 interleave_amount;
//	FILE *file;
	UINT4 num_lines_written = 0;
    TIFF *tif;
	PIXEL *rgb_line;
	unsigned int i;
	PIXEL *buffer;

	/* keep the read size under 64K since ASPI prefers that */
    max_bytes_per_read = (65536 / num_bytes_per_scanline) * num_bytes_per_scanline;
	max_scanlines_per_read = max_bytes_per_read / num_bytes_per_scanline;

	buffer = malloc(max_bytes_per_read);
	if (buffer == NULL)
		return 1;

	rgb_line = malloc(num_bytes_per_scanline);
	if (rgb_line == NULL) {
		free (buffer);
		return 1;
	}

	switch(dpi) {
		case 160:
			interleave_amount = 0;
			break;
		case 500:
			interleave_amount = 1;
			break;
		case 1000:
			interleave_amount = 2;
			break;
		case 2000:
			interleave_amount = 4;
			break;
		case 4000:
			interleave_amount = 8;
			break;
		default:
			exit(0);
	}
	setup_rgb_deinterleave(num_bytes_per_scanline / 6, interleave_amount);
//	file = fopen(filename, "wb");
	tif = TIFFOpen( filename, "w" );
	if (!tif)
		return 1;

//	TIFFSetField( tif, TIFFTAG_SUBFILETYPE, (uint16)0 );
	TIFFSetField( tif, TIFFTAG_BITSPERSAMPLE, (uint16) 16);
	TIFFSetField( tif, TIFFTAG_SAMPLESPERPIXEL,(uint16) 3);
	TIFFSetField( tif, TIFFTAG_IMAGEWIDTH, (uint32) (num_bytes_per_scanline - 240) / 6 );
	TIFFSetField( tif, TIFFTAG_IMAGELENGTH, (uint32) num_scanlines - (interleave_amount * 2));
	TIFFSetField( tif, TIFFTAG_PLANARCONFIG, (uint16) PLANARCONFIG_CONTIG );
//	TIFFSetField( tif, TIFFTAG_XPOSITION, (float) 0.0f );
//	TIFFSetField( tif, TIFFTAG_YPOSITION, (float) 0.0f);
	TIFFSetField( tif, TIFFTAG_COMPRESSION, (uint16) COMPRESSION_NONE );
//	TIFFSetField( tif, TIFFTAG_ORIENTATION, (uint16)ORIENTATION_TOPLEFT );
	TIFFSetField( tif, TIFFTAG_PHOTOMETRIC, (uint16) PHOTOMETRIC_RGB );
//	TIFFSetField( tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	TIFFSetField( tif, TIFFTAG_XRESOLUTION, (float) dpi );
	TIFFSetField( tif, TIFFTAG_YRESOLUTION, (float) dpi );
	TIFFSetField( tif, TIFFTAG_RESOLUTIONUNIT, (uint16) 1/*RESUNIT_INCH*/ );

    TIFFSetField( tif, TIFFTAG_ROWSPERSTRIP, (uint32) num_scanlines - (interleave_amount * 2));
//	TIFFSetField( tif, TIFFTAG_STRIPOFFSETS, );
//	TIFFSetField( tif, TIFFTAG_STRIPBYTECOUNTS, num_bytes_per_block - 240);

	TIFFSetField( tif, TIFFTAG_SOFTWARE,  "Dave's Scanner Software for CanoScan FS4000");
	TIFFSetField( tif, TIFFTAG_COPYRIGHT, "Copyright (c) 2004 David F. Burns, all rights reserved");
//	TIFFSetField( tif, TIFFTAG, DATETIME, "need code here to set the current date and tod");
	TIFFSetField( tif, TIFFTAG_ARTIST, "David F. Burns");
	TIFFSetField( tif, TIFFTAG_DOCUMENTNAME, "<insert document name here>");
	TIFFSetField( tif, TIFFTAG_IMAGEDESCRIPTION, "Wolf Faust Ektachrome IT8 Target");
	TIFFSetField( tif, TIFFTAG_MAKE, "Canon");
	TIFFSetField( tif, TIFFTAG_MODEL, "CanoScan FS4000US (IX-40015G) ROM: 1.07"); // to do: insert ROM info dynamically later

	num_scanlines_left = num_scanlines;
	while (num_scanlines_left) {
		UINT4 scanlines_to_read = min(num_scanlines_left, max_scanlines_per_read);
		UINT4 bytes_to_read = scanlines_to_read * num_bytes_per_scanline;

		if (fs4000_read(bytes_to_read, buffer) != 0)
			break;

		for (i = 0; i < scanlines_to_read; i++) {
			if (rgb_deinterleave(&buffer[(num_bytes_per_scanline/2) * i], num_bytes_per_scanline, rgb_line) > 0) {
				rgb_mirror_line(rgb_line, num_bytes_per_scanline / 6);
//				fwrite(rgb_line, num_bytes_per_block - 240, 1, file);

                /* this next loop moves the 14-bit image data into the upper 14-bits of each UINT2 */
				{
					UINT4 k;

					for (k = 0; k < num_bytes_per_scanline / 2; k++) {
						rgb_line[k] *= 4;
					}
				}

				if (TIFFWriteScanline( tif, rgb_line, num_lines_written++, 0 ) < 0) {
					printf( "Error writing %s\n", filename );
					break;
				}
			}
		}
		num_scanlines_left -= scanlines_to_read;
	}

//	fclose(file);
	TIFFClose( tif );
	free_rgb_deinterleave();
	free(rgb_line);
	free(buffer);

	return 0;
}


void test_do_scan()
{
	FS4000_DEFINE_SCAN_MODE_DATA_OUT define_scan_mode_data_out = { 0, 0, 0, 0, 0x20, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
																	40, 24, 28, 10, 10, 10, 1000, 800, 400, 0 }; /* this shows good range of exposure but lots of moire/aliasing */
//																	47, 36, 36, 281, 264, 261, 750, 352, 235, 130 }; /* this shows ok exposure and no moire/aliasing */
	UINT4 num_blocks;
	UINT4 num_bytes_per_block;
	int i;
	UINT2 dpi = 4000;

	fs4000_extended_inquiry(NULL);
	fs4000_get_film_status(0, NULL, NULL, NULL, NULL);
	fs4000_set_lamp(1, 0);
	printf("warming up lamp for 90 seconds...\n"); /* for faster development test cycles, just comment the Sleep's out and also comment out the set_lamp(0, 0) at the end of this function so that the light just stays on */
	Sleep(60000);
	Sleep(30000);
	for (i = 0; i < 1; i++) {
		fs4000_move_position(1, 0, 0);
		fs4000_move_position(1, 4, 1072);
		fs4000_move_position(0, 0, 0);
		fs4000_execute_afae(1, 0, 0, 0, 500, 3500);
		fs4000_move_position(1, 4, 1332);
		fs4000_define_scan_mode(&define_scan_mode_data_out);
		fs4000_set_frame(0);
		fs4000_set_window(dpi, dpi, 0, 0, 4000, 5904, 14, 5);
		fs4000_scan();
		fs4000_get_data_status(&num_blocks, &num_bytes_per_block);
		read_from_fs4000(num_blocks, num_bytes_per_block, dpi, "fs4000-image-pelican.tiff");
	}
	fs4000_set_lamp(0, 0);
	fs4000_extended_inquiry(NULL);
}


/* test_cal_settings: this was written to figure out empirically an ideal set of RGB gain/exposure
   values to use in DEFINE SCAN MODE. This can take a while to run. It loops through many possible
   sets of values, each time scanning and saving the result. I then looked at histograms for each
   channel in Photoshop and moved towards settings that gave the largest range of luminence values
   without clipping. */
void test_cal_settings()
{
	FS4000_DEFINE_SCAN_MODE_DATA_OUT define_scan_mode_data_out = { 0, 0, 0, 0, 0x20, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
																	47, 36, 36, 281, 264, 261, 750, 352, 235, 0 };
	UINT4 num_blocks;
	UINT4 num_bytes_per_block;
//	int i;
	UINT2 dpi = 1000;
	BYTE unk3;
	UINT2 unk4, unk5;
	char fname[200];
	UINT2 unk4values[] = { 10, 19, 261, 266 };

	// best for red so far: 40, 266, 1000
	// best for green so far: 24, 266, 800
	// best for blue so far: 28, 266, 400

	fs4000_extended_inquiry(NULL);
	fs4000_get_film_status(0, NULL, NULL, NULL, NULL);
	fs4000_set_lamp(1, 0);
	printf("warming up lamp for 90 seconds...\n"); /* 90 seconds is somewhat arbitrary. I think the required time is probably no more than 60. */
	Sleep(60000);
	Sleep(30000);

	fs4000_debug = 0;

	for (unk3 = 24; unk3 <= 40; unk3 += 4) {
		for (unk4 = 0; unk4 < 4; unk4++) {
			for (unk5 = 50; unk5 <= 250; unk5 += 50) {
				define_scan_mode_data_out.unknown3[0] = unk3;
				define_scan_mode_data_out.unknown3[1] = unk3;
				define_scan_mode_data_out.unknown3[2] = unk3;
				define_scan_mode_data_out.unknown4[0] = unk4values[unk4];
				define_scan_mode_data_out.unknown4[1] = unk4values[unk4];
				define_scan_mode_data_out.unknown4[2] = unk4values[unk4];
				define_scan_mode_data_out.unknown5[0] = unk5 + 750;
				define_scan_mode_data_out.unknown5[1] = unk5 + 650;
				define_scan_mode_data_out.unknown5[2] = unk5 + 250;

				fs4000_move_position(1, 0, 0);
				fs4000_move_position(1, 4, 292);
				fs4000_move_position(0, 0, 0);
				fs4000_execute_afae(1, 0, 0, 0, 500, 3500);
				fs4000_move_position(1, 4, 554);
				fs4000_define_scan_mode(&define_scan_mode_data_out);
				fs4000_set_frame(0);
				fs4000_set_window(dpi, dpi, 0, 0, 4000, 5904, 14, 5);
				fs4000_scan();
				fs4000_get_data_status(&num_blocks, &num_bytes_per_block);
				sprintf(fname, "cal-ekta-%d-%d-%d.tiff", unk3, unk4values[unk4], unk5);
				printf("scanning %s\n", fname);
				read_from_fs4000(num_blocks, num_bytes_per_block, dpi, fname);
			}
		}
	}

	fs4000_set_lamp(0, 0);
	fs4000_extended_inquiry(NULL);
}

/* test_lamp_warmup: a test to see how long it takes for scanned image data to
   stabilize after turning the lamp on. I used SETFRAME(12) for this, otherwise
   it would thrash the scanner's drive motor and I didn't want to abuse it. I'm not
   sure if that invalidates my results though.

   The basic idea of the code is to scan a 1x1 window repeatedly. For each loop,
   average the last 5 values for each channel to reduce the effects of sensor
   noise. Then, compare that average to the average from the previous loop.
   Print that difference. I then watched that difference to see if it reached some
   lower bound. Still not conclusive results yet but I haven't run it with a cold
   lamp enough times. */
void test_lamp_warmup()
{
	FS4000_DEFINE_SCAN_MODE_DATA_OUT define_scan_mode_data_out = { 0, 0, 0, 0, 0x20, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
																	62, 62, 62, 85, 85, 85, 750, 750, 750,
																	0 };
	PIXEL data_buffer[1000];
	PIXEL last5red[5], last5green[5], last5blue[5];
	PIXEL avgred = 0, lastavgred = 0;
	PIXEL avggreen = 0, lastavggreen = 0;
	PIXEL avgblue = 0, lastavgblue = 0;
	int deltared = 0, deltagreen = 0, deltablue = 0;
	UINT4 num_blocks;
	UINT4 num_bytes_per_block;
	int i;
	UINT4 duration;

	memset(data_buffer, 0, sizeof(data_buffer));
	fs4000_extended_inquiry(NULL);
	fs4000_get_film_status(0, NULL, NULL, NULL, NULL);
	fs4000_set_lamp(1, 0);
	fs4000_move_position(1, 0, 0);
	fs4000_move_position(1, 0, 694);

	fs4000_debug = 0;

	for (i = 0; i < 120; i++) {
		fs4000_set_frame(12);
		fs4000_define_scan_mode(&define_scan_mode_data_out);
		fs4000_set_window(4000, 4000, 0, 0, 1, 1, 14, 5);
		fs4000_get_lamp(NULL, &duration, NULL, NULL);
		fs4000_scan();
		fs4000_get_data_status(&num_blocks, &num_bytes_per_block);
		fs4000_read(num_bytes_per_block, data_buffer);
//		fs4000_set_frame(0);
		{
			short int red, green, blue;
			PIXEL *p;

			p = &data_buffer[120];
			red = *p;
			green = *++p;
			blue = *++p;

			last5red[i % 5] = red;
			last5green[i % 5] = green;
			last5blue[i % 5] = blue;
			if (i >= 4) {
				int j;
				unsigned int sumred = 0, sumgreen = 0, sumblue = 0;

				for (j = 0; j < 5; j++) {
					sumred += last5red[j];
					sumgreen += last5green[j];
					sumblue += last5blue[j];
				}
				avgred = sumred/5;
				avggreen = sumgreen/5;
				avgblue = sumblue/5;
				deltared = avgred - lastavgred; if (deltared < 0) deltared = -deltared;
				deltagreen = avggreen - lastavggreen; if (deltagreen < 0) deltagreen = -deltagreen;
				deltablue = avgblue - lastavgblue; if (deltablue < 0) deltablue = -deltablue;
			}
			printf("%5d: (%d,%d,%d)\t(%d,%d,%d)\t(%d,%d,%d)\t(%d,%d,%d)\n", duration, red, green, blue,
																					  avgred, avggreen, avgblue,
																					  lastavgred, lastavggreen, lastavgblue,
																					  deltared, deltagreen, deltablue);
		}
		lastavgred = avgred;
		lastavggreen = avggreen;
		lastavgblue = avgblue;
		Sleep(500);
	}

	fs4000_debug = 1;

	fs4000_set_lamp(0, 0);
	fs4000_extended_inquiry(NULL);
}

int main(int argc, char *argv[])
{
	scsiaspi_init();

	fs4000_test_unit_ready();
	fs4000_cancel();
	fs4000_reserve_unit();
	fs4000_control_led(2);

//	test_lamp();
//	test_move_pos();
	test_set_frame();
//	test_scan_modes();
//	test_do_scan();
//	test_do_little_scan();
//	test_lamp_warmup();
//	test_cal_settings();

	fs4000_cancel();
	fs4000_control_led(0);
	fs4000_release_unit();

	scsi_deinit();

	return 0;
}
