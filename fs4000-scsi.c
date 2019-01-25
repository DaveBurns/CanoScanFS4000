#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs4000-scsi.h"
#include "scsi_wrappers.h"

int fs4000_debug = 1;

/*  -------------------------------------------------

	UTILITY FUNCTIONS

    ------------------------------------------------- */

void swap_endian_4(UINT4 *n)
{
	BYTE *bytes = (BYTE *)n;
	BYTE temp;

	temp = bytes[0];
	bytes[0] = bytes[3];
	bytes[3] = temp;

	temp = bytes[1];
	bytes[1] = bytes[2];
	bytes[2] = temp;
}

void swap_endian_2(UINT2 *n)
{
	BYTE *bytes = (BYTE *)n;
	BYTE temp;

	temp = bytes[0];
	bytes[0] = bytes[1];
	bytes[1] = temp;
}

const char *convert_opcode_to_symbolic_name(BYTE opcode)
{
	static char opcode_temp[200];

	switch (opcode) {
		case 0x00 : return "TEST UNIT READY";
		case 0x12 : return "INQUIRY";
		case 0x15 : return "MODE SELECT";
		case 0x16 : return "RESERVE UNIT";
		case 0x17 : return "RELEASE UNIT";
		case 0x1b : return "SCAN";
		case 0x24 : return "SET WINDOW";
		case 0x25 : return "GET WINDOW";
		case 0x28 : return "READ";
		case 0x34 : return "GET DATA STATUS";
		case 0xd5 : return "GET SCAN MODE";
		case 0xd6 : return "DEFINE SCAN MODE";
		case 0xd8 : return "SCAN FOR THUMBNAIL";
		case 0xe0 : return "EXECUTE AF/AE";
		case 0xe1 : return "GET FILM STATUS";
		case 0xe4 : return "CANCEL";
		case 0xe6 : return "MOVE POSITION";
		case 0xe7 : return "SET LAMP";
		case 0xea : return "GET LAMP";
		case 0xe8 : return "SET FRAME";
		case 0xf0 : return "SET WINDOW FOR THUMBNAIL";
		case 0xf1 : return "GET WINDOW FOR THUMBNAIL";
		case 0xf3 : return "0xF3";
	}
	sprintf(opcode_temp, "Unknown opcode %02x", opcode);
	return opcode_temp;
}

void print_byte_array(BYTE *bytes, unsigned int num_bytes)
{
	unsigned int i;

	for (i = 0; i < num_bytes; i++)
		printf("%02x ", bytes[i]);
}

/*  -------------------------------------------------

	TEST UNIT READY

    ------------------------------------------------- */

void print_fs4000_test_unit_ready(FS4000_TEST_UNIT_READY_CDB *cdb)
{
	static FS4000_TEST_UNIT_READY_CDB expected_values = { 0, 0, 0, 0, 0, 0 };

	printf("TEST UNIT READY ");
	if (memcmp(cdb, &expected_values, sizeof(FS4000_TEST_UNIT_READY_CDB)) != 0) {
		printf("- UNEXPECTED VALUES: ");
		print_byte_array((BYTE *)cdb, sizeof(FS4000_TEST_UNIT_READY_CDB));
	}
	printf("\n");
}

int fs4000_test_unit_ready(void)
{
	FS4000_TEST_UNIT_READY_CDB cdb;

	memset(&cdb, 0, sizeof(cdb));

	if (fs4000_debug) print_fs4000_test_unit_ready(&cdb);

	return scsi_do_command(&cdb, sizeof(cdb), 0, 0, 0);
}

/*  -------------------------------------------------

	INQUIRY

    ------------------------------------------------- */

void print_fs4000_inquiry(FS4000_INQUIRY_CDB *cdb,
						  FS4000_INQUIRY_DATA_IN *data)
{
	static FS4000_INQUIRY_CDB expected_cdb = { 0x12, 0x00, 0x00, 0x00, 0x24, 0x00 };

	if (cdb != NULL) {
		printf("INQUIRY ");
		if (memcmp(cdb, &expected_cdb, sizeof(FS4000_INQUIRY_CDB)) != 0) {
			printf("- UNEXPECTED VALUES: ");
			print_byte_array((BYTE *)cdb, sizeof(FS4000_INQUIRY_CDB));
		}
		printf("\n");
	}
	if (data != NULL) {
		char s[100];

		strncpy(s, data->vendor_id, sizeof(data->vendor_id));
		s[sizeof(data->vendor_id)] = ' ';
		strncpy(&s[sizeof(data->vendor_id) + 1], data->product_id, sizeof(data->product_id));
		s[sizeof(data->vendor_id) + 1 + sizeof(data->product_id)] = ' ';
		strncpy(&s[sizeof(data->vendor_id) + 1 + sizeof(data->product_id) + 1], data->rev_level, sizeof(data->rev_level));
		s[sizeof(data->vendor_id) + 1 + sizeof(data->product_id) + 1] = 0;
		printf("\t%s\n", s);
	}
}

int fs4000_inquiry(FS4000_INQUIRY_DATA_IN *data)
{
	FS4000_INQUIRY_CDB cdb;
	FS4000_INQUIRY_DATA_IN int_data;
	int result;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0x12;
	cdb.data_length = 0x24;

	if (fs4000_debug) print_fs4000_inquiry(&cdb, NULL);

	result = scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_IN, &int_data, sizeof(int_data));
	if (result == 0)
		if (data != NULL)
			memcpy(data, &int_data, sizeof(int_data));

	if (fs4000_debug) print_fs4000_inquiry(NULL, &int_data);

	return result;
}

/*  -------------------------------------------------

	EXTENDED INQUIRY

    ------------------------------------------------- */

void swap_endian_FS4000_EXTENDED_INQUIRY_DATA_IN(FS4000_EXTENDED_INQUIRY_DATA_IN *data)
{
	swap_endian_2(&data->default_x_resolution);
	swap_endian_2(&data->default_y_resolution);

	swap_endian_2(&data->max_x_resolution);
	swap_endian_2(&data->max_y_resolution);
	swap_endian_2(&data->min_x_resolution);
	swap_endian_2(&data->min_y_resolution);

	swap_endian_2(&data->unknown2);

	swap_endian_4(&data->max_x_range);
	swap_endian_4(&data->max_y_range);
}

void print_fs4000_extended_inquiry(FS4000_INQUIRY_CDB *cdb,
								   FS4000_EXTENDED_INQUIRY_DATA_IN *data)
{
	static FS4000_INQUIRY_CDB expected_cdb = { 0x12, 0x01, 0xf0, 0x00, 0x82, 0x00 };

	if (cdb != NULL) {
		printf("INQUIRY (extended)");
		if (memcmp(cdb, &expected_cdb, sizeof(FS4000_INQUIRY_CDB)) != 0) {
			printf("- UNEXPECTED VALUES: ");
			print_byte_array((BYTE *)cdb, sizeof(FS4000_INQUIRY_CDB));
		}
		printf("\n");
	}
	if (data != NULL) {
		BYTE expected_unknown6[] = { 0x40, 1, 1, 0x68, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 8, 0, 0xe, 4, 1, 0, 0, 0,
										0, 0x10, 0, 4, 0, 0, 0, 0, 0xf, 0xa0, 0, 0xa0, 0, 1, 0, 0x50, 0xf, 0xa0, 0, 0xa0, 0, 0x50, 0, 0x50,
										0, 0, 0xf, 0xa0, 0, 0, 5, 0xc8, 2, 0x26, 0x26, 0x26, 0, 0, 0, 0, 0, 0, 3, 0xd6, 1, 0x9b, 0, 0xdf, 2,
										0x3f, 0x3f, 0x3f, 0, 0, 0, 0, 0, 0, 3, 0xe8, 3, 0xe8, 3, 0xe8 };
	/* converted to decimal in case any of that is more meaningful:
	BYTE expected_unknown6[] = { 64, 1, 1, 104, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 8, 0, 14, 4, 1, 0, 0, 0,
										0, 16, 0, 4, 0, 0, 0, 0, [4000], 0, 160, 0, 1, 0, 80, [4000], 0, 160, 0, 80, 0, 80,
										0, 0, 15, 160, 0, 0, 5, 200, 2, 38, 38, 38, 0, 0, 0, 0, 0, 0, 3, 214, 1, 155, 0, 223, 2,
										63, 63, 63, 0, 0, 0, 0, 0, 0, [1000], [1000], [1000] };
	*/

		printf("\tunknown1: %02x\n", data->unknown1);
		if (data->default_x_resolution != 4000)
			printf("\tdefault_x_resolution = %d\n", data->default_x_resolution);
		if (data->default_y_resolution != 4000)
			printf("\tdefault_y_resolution = %d\n", data->default_y_resolution);
		if (data->resolution_range_quantifier != 0x11)
			printf("\tresolution_range_quantifier = %02x\n", data->resolution_range_quantifier);
		if (data->max_x_resolution != 4000)
			printf("\tmax_x_resolution = %d\n", data->max_x_resolution);
		if (data->max_y_resolution != 4000)
			printf("\tmax_y_resolution = %d\n", data->max_y_resolution);
		if (data->min_x_resolution != 60)
			printf("\tmin_x_resolution = %d\n", data->min_x_resolution);
		if (data->min_y_resolution != 60)
			printf("\tmin_y_resolution = %d\n", data->min_y_resolution);
		printf("\tunknown2: %d\n", data->unknown2);
		if ((data->max_x_range != 4000) || (data->max_y_range != 5904))
			printf("\tmax_x_range = %d (0x%x), max_y_range = %d (0x%x)\n", data->max_x_range, data->max_x_range, data->max_y_range, data->max_y_range);
		if (memcmp(&data->unknown6, expected_unknown6, sizeof(expected_unknown6)) != 0) {
			printf("\t");
			print_byte_array(data->unknown6, sizeof(expected_unknown6));
			printf("\n");
		}
	}
}


int fs4000_extended_inquiry(FS4000_EXTENDED_INQUIRY_DATA_IN *data)
{
	FS4000_INQUIRY_CDB cdb;
	FS4000_EXTENDED_INQUIRY_DATA_IN int_data;
	int result;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0x12;
	cdb.flags = 0x01;
	cdb.page_code = 0xf0;
	cdb.data_length = 0x82;

	if (fs4000_debug) print_fs4000_extended_inquiry(&cdb, NULL);

	result = scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_IN, &int_data, sizeof(int_data));
	if (result == 0) {
		if (data != NULL) {
			memcpy(data, &int_data, sizeof(int_data));
			swap_endian_FS4000_EXTENDED_INQUIRY_DATA_IN(data);
		}
		if (fs4000_debug) print_fs4000_extended_inquiry(NULL, data);
	}

	return result;
}

/*  -------------------------------------------------

	MODE SELECT

    ------------------------------------------------- */

void swap_endian_FS4000_MODE_SELECT_DATA_OUT(FS4000_MODE_SELECT_DATA_OUT *data)
{
	swap_endian_2(&data->measurement_units_page.measurement_unit_divisor);
}

void print_fs4000_mode_select(FS4000_MODE_SELECT_CDB *cdb,
							  FS4000_MODE_SELECT_DATA_OUT *data)
{
	static FS4000_MODE_SELECT_CDB expected_cdb = { 0x15, 0x10, 0, 0, 0xc, 0 };
	static FS4000_MODE_SELECT_DATA_OUT expected_data = { 0, 0, 0, 0, { 3, 6, 0, 0, 4000, 0, 0 }};

	printf("MODE SELECT ");
	if (memcmp(cdb, &expected_cdb, sizeof(FS4000_MODE_SELECT_CDB)) != 0) {
		printf("- UNEXPECTED CDB: ");
		print_byte_array((BYTE *)cdb, sizeof(FS4000_MODE_SELECT_CDB));
	}
	printf("\n");

	if (memcmp(data, &expected_data, sizeof(FS4000_MODE_SELECT_DATA_OUT)) != 0) {
		printf("\tUNEXPECTED VALUES: ");
		print_byte_array((BYTE *)data, sizeof(FS4000_MODE_SELECT_DATA_OUT));
		printf("\n");
	}
}

int fs4000_mode_select()
{
	FS4000_MODE_SELECT_CDB cdb;
	FS4000_MODE_SELECT_DATA_OUT data;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0x15;
	cdb.flags = 0x10;
	cdb.data_length = 0xc;

	memset(&data, 0, sizeof(data));
	data.measurement_units_page.page_code = 3;
	data.measurement_units_page.param_length = 6;
	data.measurement_units_page.basic_measurement_unit = 0;
	data.measurement_units_page.measurement_unit_divisor = 4000;

	if (fs4000_debug) print_fs4000_mode_select(&cdb, &data);

	swap_endian_FS4000_MODE_SELECT_DATA_OUT(&data);
	return scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_OUT, &data, sizeof(data));
}

/*  -------------------------------------------------

	RESERVE UNIT

    ------------------------------------------------- */

void print_fs4000_reserve_unit(FS4000_RESERVE_UNIT_CDB *cdb)
{
	static FS4000_RESERVE_UNIT_CDB expected_cdb = { 0x16, 0, 0, 0, 0, 0 };

	printf("RESERVE UNIT ");
	if (memcmp(cdb, &expected_cdb, sizeof(FS4000_RESERVE_UNIT_CDB)) != 0) {
		printf("- UNEXPECTED VALUES: ");
		print_byte_array((BYTE *)cdb, sizeof(FS4000_RESERVE_UNIT_CDB));
	}
	printf("\n");
}

int fs4000_reserve_unit()
{
	FS4000_RESERVE_UNIT_CDB cdb;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0x16;

	if (fs4000_debug) print_fs4000_reserve_unit(&cdb);

	return scsi_do_command(&cdb, sizeof(cdb), 0, 0, 0);
}

/*  -------------------------------------------------

	RELEASE UNIT

    ------------------------------------------------- */

void print_fs4000_release_unit(FS4000_RELEASE_UNIT_CDB *cdb)
{
	static FS4000_RELEASE_UNIT_CDB expected_cdb = { 0x17, 0, 0, 0, 0, 0 };

	printf("RELEASE UNIT ");
	if (memcmp(cdb, &expected_cdb, sizeof(FS4000_RELEASE_UNIT_CDB)) != 0) {
		printf("- UNEXPECTED VALUES: ");
		print_byte_array((BYTE *)cdb, sizeof(FS4000_RELEASE_UNIT_CDB));
	}
	printf("\n");
}

int fs4000_release_unit()
{
	FS4000_RELEASE_UNIT_CDB cdb;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0x17;

	if (fs4000_debug) print_fs4000_release_unit(&cdb);

	return scsi_do_command(&cdb, sizeof(cdb), 0, 0, 0);
}

/*  -------------------------------------------------

	SCAN

    ------------------------------------------------- */

void print_fs4000_scan(FS4000_SCAN_CDB *cdb,
					   FS4000_SCAN_DATA_OUT *data)
{
	static FS4000_SCAN_CDB expected_cdb = { 0x1b, 0, 0, 0, 1, 0 };

	printf("SCAN ");
	if (memcmp(cdb, &expected_cdb, sizeof(FS4000_SCAN_CDB)) != 0) {
		printf("- UNEXPECTED VALUES: ");
		print_byte_array((BYTE *)cdb, sizeof(FS4000_SCAN_CDB));
	}
	printf("\n");

	if (data->window_id != 0)
		printf("\tUNEXPECTED window_id: %d\n", data->window_id);
}

int fs4000_scan()
{
	FS4000_SCAN_CDB cdb;
	FS4000_SCAN_DATA_OUT data;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0x1b;
	cdb.data_length = 1;

	data.window_id = 0;

	if (fs4000_debug) print_fs4000_scan(&cdb, &data);

	return scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_OUT, &data, sizeof(data));
}

/*  -------------------------------------------------

	GET/SET WINDOW UTILITY FUNCS

    ------------------------------------------------- */


void swap_endian_FS4000_GETSET_WINDOW_DATA(FS4000_GET_WINDOW_DATA_IN *data)
{
	SCSI_WINDOW_DESCRIPTOR *w = &data->window[0];

	swap_endian_2(&data->header.window_data_length);
	swap_endian_2(&data->header.window_descriptor_length);

	swap_endian_2(&w->x_resolution);
	swap_endian_2(&w->y_resolution);
	swap_endian_4(&w->x_upper_left);
	swap_endian_4(&w->y_upper_left);
	swap_endian_4(&w->window_width);
	swap_endian_4(&w->window_length);
	swap_endian_2(&w->halftone_pattern);
	swap_endian_2(&w->bit_order);
}


void print_SCSI_WINDOW_DESCRIPTOR(SCSI_WINDOW_DESCRIPTOR *w)
{
	static char *image_comp_descriptions[] = {
		"Bi-level black & white",
		"Dithered/halftone black & white",
		"Multi-level black & white (gray scale)",
		"Bi-level RGB colour",
		"Dithered/halftone RGB colour",
		"Multi-level RGB colour" };
	static char *padding_type_descriptions[] = {
		"No padding",
		"Pad with 0’s to byte boundary",
		"Pad with 1’s to byte boundary",
		"Truncate to byte boundary" };

	if (w->window_id != 0)
		printf("\twindow_id = %d\n", w->window_id);
	if (w->flags != 0)
		printf("\tflags = %d\n", w->flags);
	printf("\tx_res %d, y_res %d\n", w->x_resolution, w->y_resolution);
	printf("\tscan window = (%d, %d, %d, %d)\n", w->x_upper_left, w->y_upper_left, w->window_width, w->window_length);
	if ((w->brightness != 0) || (w->threshold != 0) || (w->contrast != 0))
		printf("\tbrightness = %d, threshold = %d, contrast = %d\n", w->brightness, w->threshold, w->contrast);
	printf("\timage_composition = %s\n", image_comp_descriptions[w->image_composition]);
	if (w->halftone_pattern != 0)
		printf("\thalftone_pattern = %d\n", w->halftone_pattern);
	if ((w->image_flags & 0x80) != 0)
		printf("\tRIF = %d\n", w->image_flags & 0x80);
	printf("\tbpp = %d and padding = %s\n", w->bits_per_pixel, padding_type_descriptions[w->image_flags & 0x07]);
	if (w->bit_order != 0)
		printf("\tbit_order = %d\n", w->bit_order);
	if (w->compression_type != 0)
		printf("\tcompression_type = %d\n", w->compression_type);
	if (w->compression_arg != 0)
		printf("\tcompression_arg = %d\n", w->compression_arg);
}

/*  -------------------------------------------------

	SET WINDOW

    ------------------------------------------------- */

void swap_endian_FS4000_SET_WINDOW_CDB(FS4000_SET_WINDOW_CDB *cdb)
{
	swap_endian_2(&cdb->data_length);
}

void print_fs4000_set_window(FS4000_SET_WINDOW_CDB *cdb,
							 FS4000_SET_WINDOW_DATA_OUT *data)
{
	static FS4000_SET_WINDOW_CDB expected_cdb = { 0x24, 0, 0, 0, 0, 0, 0, 72, 0 };
	static SCSI_WINDOW_HEADER expected_values_header = { 0, 0, 0, 0, 0, 64 };
	SCSI_WINDOW_DESCRIPTOR *w = &data->window[1];

	printf("SET WINDOW ");
	if (memcmp(cdb, &expected_cdb, sizeof(FS4000_SET_WINDOW_CDB)) != 0) {
		printf("- UNEXPECTED VALUES: ");
		print_byte_array((BYTE *)cdb, sizeof(FS4000_SET_WINDOW_CDB));
	}
	printf("\n");

	if (memcmp(&data->header, &expected_values_header, sizeof(SCSI_WINDOW_HEADER)) != 0) {
		printf("\tUNEXPECTED VALUES: header: ");
		print_byte_array((BYTE *)&data->header, sizeof(SCSI_WINDOW_HEADER));
		printf("\n");
	}

	print_SCSI_WINDOW_DESCRIPTOR(&data->window[0]);
}


int fs4000_set_window(UINT2 x_res,
					  UINT2 y_res,
					  UINT4 x_upper_left,
					  UINT4 y_upper_left,
					  UINT4 width,
					  UINT4 height,
					  BYTE bits_per_pixel,
                      BYTE image_composition)
{
	FS4000_SET_WINDOW_CDB cdb;
	FS4000_SET_WINDOW_DATA_OUT data;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0x24;
	cdb.data_length = 72;

	memset(&data, 0, sizeof(data));
	data.header.window_descriptor_length = 64;
	data.window[0].x_resolution = x_res;
	data.window[0].y_resolution = y_res;
	data.window[0].x_upper_left = x_upper_left;
	data.window[0].y_upper_left = y_upper_left;
	data.window[0].window_width = width;
	data.window[0].window_length = height;
	data.window[0].bits_per_pixel = bits_per_pixel;
	data.window[0].image_composition = image_composition;
	data.window[0].image_flags = ((bits_per_pixel % 8) ? 3 : 0);
	data.window[0].reserved[12] = 0x02;

	if (fs4000_debug == 1) print_fs4000_set_window(&cdb, &data);

	swap_endian_FS4000_SET_WINDOW_CDB(&cdb);
	swap_endian_FS4000_GETSET_WINDOW_DATA((FS4000_GET_WINDOW_DATA_IN *)&data);
	return scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_OUT, &data, sizeof(data));
}

/*  -------------------------------------------------

	GET WINDOW

    ------------------------------------------------- */

void swap_endian_FS4000_GET_WINDOW_CDB(FS4000_GET_WINDOW_CDB *cdb)
{
	swap_endian_2(&cdb->data_length);
}

void print_fs4000_get_window(FS4000_GET_WINDOW_CDB *cdb,
							 FS4000_GET_WINDOW_DATA_IN *data)
{
	static FS4000_GET_WINDOW_CDB expected_cdb = { 0x25, 1, 0, 0, 0, 0, 0, 72, 0 };
	static SCSI_WINDOW_HEADER expected_values_header = { 70, 0, 0, 0, 0, 64 };

	if (cdb != NULL) {
		printf("GET WINDOW ");
		if (memcmp(cdb, &expected_cdb, sizeof(FS4000_GET_WINDOW_CDB)) != 0) {
			printf("- UNEXPECTED VALUES: ");
			print_byte_array((BYTE *)cdb, sizeof(FS4000_GET_WINDOW_CDB));
		}
		printf("\n");
	}
	if (data != NULL) {
		if (memcmp(&data->header, &expected_values_header, sizeof(SCSI_WINDOW_HEADER)) != 0) {
			printf("UNEXPECTED VALUES: ");
			printf("header: ");
			print_byte_array((BYTE *)&data->header, sizeof(SCSI_WINDOW_HEADER));
			printf("\n");
		}

		print_SCSI_WINDOW_DESCRIPTOR(&data->window[0]);
	}
}

int fs4000_get_window(UINT2 *x_res,
					  UINT2 *y_res,
					  UINT4 *x_upper_left,
					  UINT4 *y_upper_left,
					  UINT4 *width,
					  UINT4 *height,
					  BYTE *bits_per_pixel)
{
	FS4000_GET_WINDOW_CDB cdb;
	FS4000_GET_WINDOW_DATA_IN data;
	int result;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0x25;
	cdb.flags = 0x01;
	cdb.data_length = 72;

	if (fs4000_debug == 1) print_fs4000_get_window(&cdb, NULL);
	swap_endian_FS4000_GET_WINDOW_CDB(&cdb);

	result = scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_OUT, &data, sizeof(data));
	if (result == 0) {
		swap_endian_FS4000_GETSET_WINDOW_DATA(&data);
		if (fs4000_debug == 1) print_fs4000_get_window(NULL, &data);

		*x_res = data.window[0].x_resolution;
		*y_res = data.window[0].y_resolution;
		*x_upper_left = data.window[0].x_upper_left;
		*y_upper_left = data.window[0].y_upper_left;
		*width = data.window[0].window_width;
		*height = data.window[0].window_length;
		*bits_per_pixel = data.window[0].bits_per_pixel;
	}

	return result;
}

/*  -------------------------------------------------

	READ

    ------------------------------------------------- */

void swap_endian_FS4000_READ_CDB(FS4000_READ_CDB *cdb)
{
	swap_endian_2(&cdb->data_type_qualifier);
	swap_endian_2(&cdb->data_length);
}

void print_fs4000_read(FS4000_READ_CDB *cdb)
{
	printf("READ ");

	if (cdb->opcode != 0x28)
		printf("SOMETHING IS WRONG: MISMATCHED OPCODE: %02x ", cdb->opcode);
	if (cdb->flags != 0x00)
		printf("flags: %02x ", cdb->flags);
	if (cdb->data_type_code != 0x00)
		printf("data_type_code: %d ", cdb->data_type_code);
	if (cdb->reserved != 0x00)
		printf("reserved: %02x ", cdb->reserved);
	if (cdb->data_type_qualifier != 0x00)
		printf("data_type_qualifier: %02x ", cdb->data_type_qualifier);
	if (cdb->data_length_msb != 0x00)
		printf("data_length_msb: %02x ", cdb->data_length_msb);

	printf("data_length: %d ", cdb->data_length);
	if (cdb->control != 0x00)
		printf("control: %02x ", cdb->control);
	printf("\n");
}

int fs4000_read(int buffer_size, PIXEL *buffer)
{
	FS4000_READ_CDB cdb;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0x28;
	cdb.flags = 0x10;
	cdb.data_type_code = 0;
	cdb.data_length_msb = ((buffer_size & 0xff0000) >> 16);
	cdb.data_length = (buffer_size & 0xffff);

	if (fs4000_debug == 1) print_fs4000_read(&cdb);
	swap_endian_FS4000_READ_CDB(&cdb);

	return scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_IN, buffer, buffer_size);
}

/*  -------------------------------------------------

	GET DATA STATUS

    ------------------------------------------------- */

void swap_endian_FS4000_GET_DATA_STATUS_CDB(FS4000_GET_DATA_STATUS_CDB *cdb)
{
	swap_endian_2(&cdb->data_length);
}

void swap_endian_FS4000_GET_DATA_STATUS_DATA_IN(FS4000_GET_DATA_STATUS_DATA_IN *data)
{
	swap_endian_4(&data->total_bytes_to_read);
	swap_endian_4(&data->bytes_per_scanline);
	swap_endian_4(&data->num_scanlines);
	swap_endian_4(&data->total_bytes_to_read2);
}

void print_fs4000_get_data_status(FS4000_GET_DATA_STATUS_CDB *cdb,
								  FS4000_GET_DATA_STATUS_DATA_IN *data)
{
	static FS4000_GET_DATA_STATUS_CDB expected_cdb = { 0x34, 0, 0, 0, 0, 0, 0, 28, 0 };

	if (cdb != NULL) {
		printf("GET DATA STATUS ");
		if (memcmp(cdb, &expected_cdb, sizeof(FS4000_GET_DATA_STATUS_CDB)) != 0) {
			printf("- UNEXPECTED VALUES: ");
			print_byte_array((BYTE *)cdb, sizeof(FS4000_GET_DATA_STATUS_CDB));
		}
		printf("\n");
	}

	if (data != NULL) {
		int i;

		for (i = 0; i < 8; i++)
			if (data->unknown1[i] != 0)
				printf("\tunknown1[%d]=%02x\n", i, data->unknown1[i]);

		printf("\ttotal_bytes_to_read: %d, bytes_per_scanline: %d, num_scanlines: %d\n", data->total_bytes_to_read, data->bytes_per_scanline, data->num_scanlines);
		if (data->total_bytes_to_read != data->total_bytes_to_read2)
			printf("\tUNEXPECTED mismatch between total_bytes_to_read and total_bytes_to_read2 (which=%d)\n", data->total_bytes_to_read2);

		for (i = 0; i < 4; i++)
			if (data->unknown2[i] != 0)
				printf("\tunknown2[%d]=%02x\n", i, data->unknown2[i]);
	}
}

int fs4000_get_data_status(UINT4 *num_scanlines, UINT4 *num_bytes_per_scanline)
{
	FS4000_GET_DATA_STATUS_CDB cdb;
	FS4000_GET_DATA_STATUS_DATA_IN data;
	int result;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0x34;
	cdb.data_length = 28;

	if (fs4000_debug == 1) print_fs4000_get_data_status(&cdb, NULL);
	swap_endian_FS4000_GET_DATA_STATUS_CDB(&cdb);

	result = scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_IN, &data, sizeof(data));
	if (result == 0) {
		swap_endian_FS4000_GET_DATA_STATUS_DATA_IN(&data);
		if (fs4000_debug == 1) print_fs4000_get_data_status(NULL, &data);

		if (num_scanlines != NULL)
			*num_scanlines = data.num_scanlines;
		if (num_bytes_per_scanline != NULL)
			*num_bytes_per_scanline = data.bytes_per_scanline;
	}

	return result;
}

/*  -------------------------------------------------

	GET/SET SCAN MODE SHARED FUNCS

    ------------------------------------------------- */

void swap_endian_FS4000_GETSET_SCAN_MODE_DATA(FS4000_GET_SCAN_MODE_DATA_IN_38 *data)
{
	int i;

	for (i = 0; i < 3; i++)
		swap_endian_2(&data->unknown4[i]);

	for (i = 0; i < 3; i++)
		swap_endian_2(&data->unknown5[i]);
}

/*  -------------------------------------------------

	GET SCAN MODE

    ------------------------------------------------- */

void print_fs4000_get_scan_mode(FS4000_GET_SCAN_MODE_CDB *cdb,
								FS4000_GET_SCAN_MODE_DATA_IN_38 *data_38,
								FS4000_GET_SCAN_MODE_DATA_IN_12 *data_12)
{
	static FS4000_GET_SCAN_MODE_CDB expected_cdb1 = { 0xd5, 0, 0x20, 0, 38, 0 };
	static FS4000_GET_SCAN_MODE_CDB expected_cdb2 = { 0xd5, 0, 0x02, 0, 12, 0 };

	if (cdb != NULL) {
		printf("GET SCAN MODE ");
		if (memcmp(cdb, &expected_cdb1, sizeof(FS4000_GET_SCAN_MODE_CDB)) == 0) {
		}
		else if (memcmp(cdb, &expected_cdb2, sizeof(FS4000_GET_SCAN_MODE_CDB)) == 0)
			printf("(rare) ");
		else {
			printf("- UNEXPECTED VALUES: ");
			print_byte_array((BYTE *)cdb, sizeof(FS4000_GET_SCAN_MODE_CDB));
		}
		printf("\n");
	}

	if (data_38 != NULL) {
		static BYTE expected_data38_1[] = { 0x25, 0, 0, 0, 0x20, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		static BYTE expected_data38_2[] = { 0x01, 0x19, 0, 0, 0, 0 };

		if (memcmp(data_38->unknown1, expected_data38_1, sizeof(data_38->unknown1)) != 0) {
			printf("\tUNEXPECTED unknown1: ");
			print_byte_array(data_38->unknown1, sizeof(data_38->unknown1));
			printf("\n");
		}

		if (data_38->scanning_speed != 2)
			printf("\tscanning_speed: %d\n", data_38->scanning_speed);

		if (memcmp(data_38->unknown2, expected_data38_2, sizeof(data_38->unknown2)) != 0) {
			printf("\tUNEXPECTED unknown2: ");
			print_byte_array(data_38->unknown2, sizeof(data_38->unknown2));
			printf("\n");
		}
		printf("\tunknown3: %4xh %4d\t%4xh %4d\t%4xh %4d\n", data_38->unknown3[0], data_38->unknown3[0], data_38->unknown3[1], data_38->unknown3[1], data_38->unknown3[2], data_38->unknown3[2]);
		printf("\tunknown4: %4xh %4d\t%4xh %4d\t%4xh %4d\n", data_38->unknown4[0], data_38->unknown4[0], data_38->unknown4[1], data_38->unknown4[1], data_38->unknown4[2], data_38->unknown4[2]);
		printf("\tunknown5: %4xh %4d\t%4xh %4d\t%4xh %4d\n", data_38->unknown5[0], data_38->unknown5[0], data_38->unknown5[1], data_38->unknown5[1], data_38->unknown5[2], data_38->unknown5[2]);

		if (data_38->unknown6 != 0)
			printf("\tunknown6: %4xh %4d\n", data_38->unknown6, data_38->unknown6);
	}

	if (data_12 != NULL) {
		static FS4000_GET_SCAN_MODE_DATA_IN_12 expected_data12 = { 0x0b, 0, 0, 0, 0x02, 0x06, 0x80, 0x05, 0x27, 0x10, 0, 0 };

		if (memcmp(data_12, &expected_data12, sizeof(FS4000_GET_SCAN_MODE_DATA_IN_12)) != 0) {
			printf("\tUNEXPECTED VALUES: ");
			print_byte_array((BYTE *)data_12, sizeof(FS4000_GET_SCAN_MODE_DATA_IN_12));
			printf("\n");
		}
	}
}

int fs4000_get_scan_mode(int rare,
						 FS4000_GET_SCAN_MODE_DATA_IN_38 *data_38,
						 FS4000_GET_SCAN_MODE_DATA_IN_12 *data_12)
{
	FS4000_GET_SCAN_MODE_CDB cdb;
	FS4000_GET_SCAN_MODE_DATA_IN_38 int_data_38;
	FS4000_GET_SCAN_MODE_DATA_IN_12 int_data_12;
	int result;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0xd5;
	if (rare) {
		cdb.unknown2 = 0x02;
		cdb.data_length = 12;

		if (fs4000_debug == 1) print_fs4000_get_scan_mode(&cdb, NULL, NULL);

		result = scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_IN, &int_data_12, sizeof(int_data_12));
		if (result == 0) {
			if (fs4000_debug == 1) print_fs4000_get_scan_mode(NULL, NULL, &int_data_12);
			if (data_12 != NULL)
				memcpy(data_12, &int_data_12, sizeof(int_data_12));
		}
	}
	else {
		cdb.unknown2 = 0x20;
		cdb.data_length = 38;

		if (fs4000_debug == 1) print_fs4000_get_scan_mode(&cdb, NULL, NULL);

		result = scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_IN, &int_data_38, sizeof(int_data_38));
		if (result == 0) {
			swap_endian_FS4000_GETSET_SCAN_MODE_DATA(&int_data_38);
			if (fs4000_debug == 1) print_fs4000_get_scan_mode(NULL, &int_data_38, NULL);
			if (data_38 != NULL)
				memcpy(data_38, &int_data_38, sizeof(int_data_38));
		}
	}

	return result;
}

/*  -------------------------------------------------

	DEFINE SCAN MODE

    ------------------------------------------------- */

void swap_endian_FS4000_DEFINE_SCAN_MODE_CDB(FS4000_DEFINE_SCAN_MODE_CDB *cdb)
{
}

void print_fs4000_define_scan_mode(FS4000_DEFINE_SCAN_MODE_CDB *cdb,
								   FS4000_DEFINE_SCAN_MODE_DATA_OUT *data)
{
	static FS4000_DEFINE_SCAN_MODE_CDB expected_cdb = { 0xd6, 0x10, 0, 0, 38, 0 };
	static BYTE expected_data1[] = { 0, 0, 0, 0, 0x20, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	static BYTE expected_data2[] = { 0, 0, 0, 0, 0, 0 };

	printf("DEFINE SCAN MODE ");
	if (memcmp(cdb, &expected_cdb, sizeof(FS4000_DEFINE_SCAN_MODE_CDB)) != 0) {
		printf("- UNEXPECTED: ");
		print_byte_array((BYTE *)cdb, sizeof(FS4000_DEFINE_SCAN_MODE_CDB));
	}
	printf("\n");

	if (memcmp(data->unknown1, expected_data1, sizeof(data->unknown1)) != 0) {
		printf("\tUNEXPECTED unknown1: ");
		print_byte_array(data->unknown1, sizeof(data->unknown1));
		printf("\n");
	}

	if (data->scanning_speed != 2)
		printf("  scanning_speed: %d\n", data->scanning_speed);

	if (memcmp(data->unknown2, expected_data2, sizeof(data->unknown2)) != 0) {
		printf("\tUNEXPECTED unknown2: ");
		print_byte_array(data->unknown2, sizeof(data->unknown2));
		printf("\n");
	}

	printf("\tunknown3: %4xh %4d\t%4xh %4d\t%4xh %4d\n", data->unknown3[0], data->unknown3[0], data->unknown3[1], data->unknown3[1], data->unknown3[2], data->unknown3[2]);
	printf("\tunknown4: %4xh %4d\t%4xh %4d\t%4xh %4d\n", data->unknown4[0], data->unknown4[0], data->unknown4[1], data->unknown4[1], data->unknown4[2], data->unknown4[2]);
	printf("\tunknown5: %4xh %4d\t%4xh %4d\t%4xh %4d\n", data->unknown5[0], data->unknown5[0], data->unknown5[1], data->unknown5[1], data->unknown5[2], data->unknown5[2]);

	if (data->unknown6 != 0)
		printf("\tunknown6: %4xh %4d\n", data->unknown6, data->unknown6);
}

int fs4000_define_scan_mode(FS4000_DEFINE_SCAN_MODE_DATA_OUT *data)
{
	FS4000_DEFINE_SCAN_MODE_CDB cdb;
	int result;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0xd6;
	cdb.unknown1 = 0x10;
	cdb.data_length = 38;

	if (fs4000_debug == 1) print_fs4000_define_scan_mode(&cdb, data);
	swap_endian_FS4000_DEFINE_SCAN_MODE_CDB(&cdb);
	swap_endian_FS4000_GETSET_SCAN_MODE_DATA(data);
	result = scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_OUT, data, sizeof(*data));
	swap_endian_FS4000_GETSET_SCAN_MODE_DATA(data);
	return result;
}

/*  -------------------------------------------------

	SCAN FOR THUMBNAIL

    ------------------------------------------------- */

void print_fs4000_scan_for_thumbnail(FS4000_SCAN_FOR_THUMBNAIL_CDB *cdb)
{
	static FS4000_SCAN_FOR_THUMBNAIL_CDB expected_values = { 0xd8, 0, 0, 0, 1, 0 };

	printf("SCAN FOR THUMBNAIL ");
	if (memcmp(cdb, &expected_values, sizeof(FS4000_SCAN_FOR_THUMBNAIL_CDB)) != 0)
		print_byte_array((BYTE *)cdb, sizeof(FS4000_SCAN_FOR_THUMBNAIL_CDB));
	printf("\n");
}

int fs4000_scan_for_thumbnail()
{
	FS4000_SCAN_FOR_THUMBNAIL_CDB cdb;
    BYTE pdb;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0xd8;
	cdb.data_length = 1;

    pdb = 0;

    if (fs4000_debug == 1) print_fs4000_scan_for_thumbnail(&cdb);

	return scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_OUT, &pdb, 1);
}

/*  -------------------------------------------------

	EXECUTE AF AE

    ------------------------------------------------- */

void swap_endian_FS4000_EXECUTE_AFAE_DATA_OUT(FS4000_EXECUTE_AFAE_DATA_OUT *data)
{
	swap_endian_2(&data->unknown2);
	swap_endian_2(&data->unknown3);
}

void print_fs4000_execute_afae(FS4000_EXECUTE_AFAE_CDB *cdb,
							   FS4000_EXECUTE_AFAE_DATA_OUT *data)
{
	static FS4000_EXECUTE_AFAE_CDB expected_cdb = { 0xe0, 0, 0, 0, 0, 0, 0, 0, 8, 0 };

	printf("EXECUTE AF/AE ");
	if (memcmp(cdb, &expected_cdb, sizeof(FS4000_EXECUTE_AFAE_CDB)) != 0) {
		printf("cdb: ");
		print_byte_array((BYTE *)cdb, sizeof(FS4000_EXECUTE_AFAE_CDB));
	}

	printf("data: ");
	printf("unknown1: %d %d %d ", data->unknown1[0], data->unknown1[1], data->unknown1[2]);
	printf("focus_pos: %d ", data->focus_position);
	printf("unknown2 %d unknown3 %d ", data->unknown2, data->unknown3);
	printf("\n");
}

int fs4000_execute_afae(BYTE unknown1a, BYTE unknown1b, BYTE unknown1c,
						BYTE focus_position,
						UINT2 unknown2,
						UINT2 unknown3)
{
	FS4000_EXECUTE_AFAE_CDB cdb;
	FS4000_EXECUTE_AFAE_DATA_OUT data;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0xe0;
	cdb.data_length = 8;

	memset(&data, 0, sizeof(data));
	data.unknown1[0] = unknown1a;
	data.unknown1[1] = unknown1b;
	data.unknown1[2] = unknown1c;
	data.focus_position = focus_position;
	data.unknown2 = unknown2;
	data.unknown3 = unknown3;

	if (fs4000_debug == 1) print_fs4000_execute_afae(&cdb, &data);
	swap_endian_FS4000_EXECUTE_AFAE_DATA_OUT(&data);

	return scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_OUT, &data, sizeof(data));	
}

/*  -------------------------------------------------

	GET FILM STATUS

    ------------------------------------------------- */

void swap_endian_FS4000_GET_FILM_STATUS_DATA_IN(FS4000_GET_FILM_STATUS_DATA_IN_25 *data)
{
	swap_endian_2(&data->film_position);
	swap_endian_2(&data->unknown2);
	swap_endian_2(&data->unknown3);
}

void print_fs4000_get_film_status(FS4000_GET_FILM_STATUS_CDB *cdb,
								  FS4000_GET_FILM_STATUS_DATA_IN_25 *data25,
								  FS4000_GET_FILM_STATUS_DATA_IN_28 *data28)
{
	static FS4000_GET_FILM_STATUS_CDB expected_cdb1 = { 0xe1, 00, 00, 00, 00, 00, 00, 00, 28, 00 };
	static FS4000_GET_FILM_STATUS_CDB expected_cdb2 = { 0xe1, 00, 00, 00, 00, 00, 00, 00, 25, 00 };

	if (cdb != NULL) {
		printf("GET FILM STATUS ");
		if (memcmp(cdb, &expected_cdb1, sizeof(FS4000_GET_FILM_STATUS_CDB)) == 0) {
			printf("(long) ");
		}
		else if (memcmp(cdb, &expected_cdb2, sizeof(FS4000_GET_FILM_STATUS_CDB)) == 0)
			printf("(short) ");
		else {
			print_byte_array((BYTE *)cdb, sizeof(FS4000_GET_FILM_STATUS_CDB));
		}
		printf("\n");
	}

	if ((data25 != NULL) || (data28 != NULL)) {
		FS4000_GET_FILM_STATUS_DATA_IN_28 *data;
		if (data25)
			data = (FS4000_GET_FILM_STATUS_DATA_IN_28 *)data25;
		else
			data = data28;
		printf("\tfilm_holder: %s", (data->film_holder_type == 0) ? "none/empty APS" : (data->film_holder_type == 1 ? "neg" : "pos"));
		printf(" w/ %d frames\n", data->num_frames);
		printf("\tfilm position: 0x%04x (%d)\n\t", data->film_position, data->film_position);
		print_byte_array(data->unknown1, sizeof(data->unknown1));
		printf("\n\tfocus position: 0x%02x (%d)\n", data->focus_position, data->focus_position);
		printf("\tunknown2: %d unknown3: %d\n\t", data->unknown2, data->unknown3);
		if (data25)
			print_byte_array(data25->unknown4, sizeof(data25->unknown4));
		else
			print_byte_array(data28->unknown4, sizeof(data28->unknown4));
		printf("\n");
	}
}

int fs4000_get_film_status(int shorter,
						   BYTE *film_holder, BYTE *num_frames, UINT2 *film_position, BYTE *focus_position)
{
	FS4000_GET_FILM_STATUS_CDB cdb;
	FS4000_GET_FILM_STATUS_DATA_IN_28 data;
	int result;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0xe1;
	if (shorter)
		cdb.data_length = 25;
	else
		cdb.data_length = 28;

	if (fs4000_debug == 1) print_fs4000_get_film_status(&cdb, NULL, NULL);

	result = scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_IN, &data, shorter ? sizeof(FS4000_GET_FILM_STATUS_DATA_IN_25) : sizeof(data));
	if (result == 0) {
		swap_endian_FS4000_GET_FILM_STATUS_DATA_IN((FS4000_GET_FILM_STATUS_DATA_IN_25 *)&data);
		if (fs4000_debug == 1) print_fs4000_get_film_status(NULL, shorter ? (FS4000_GET_FILM_STATUS_DATA_IN_25 *)&data : NULL, shorter ? NULL : &data);

		if (film_holder != NULL)
			*film_holder = data.film_holder_type;
		if (num_frames != NULL)
			*num_frames = data.num_frames;
		if (film_position != NULL)
			*film_position = data.film_position;
		if (focus_position != NULL)
			*focus_position = data.focus_position;
	}

	return result;
}

/*  -------------------------------------------------

	CANCEL

    ------------------------------------------------- */

void print_fs4000_cancel(FS4000_CANCEL_CDB *cdb)
{
	static FS4000_CANCEL_CDB expected_cdb = { 0xe4, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	printf("CANCEL ");
	if (memcmp(cdb, &expected_cdb, sizeof(FS4000_CANCEL_CDB)) != 0) {
		printf("- UNEXPECTED VALUES: ");
		print_byte_array((BYTE *)cdb, sizeof(FS4000_CANCEL_CDB));
	}
	printf("\n");
}

int fs4000_cancel()
{
	FS4000_CANCEL_CDB cdb;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0xe4;

	if (fs4000_debug) print_fs4000_cancel(&cdb);

	return scsi_do_command(&cdb, sizeof(cdb), 0, 0, 0);
}


/*  -------------------------------------------------

	MOVE POSITION

    ------------------------------------------------- */

void swap_endian_FS4000_MOVE_POSITION_CDB(FS4000_MOVE_POSITION_CDB *cdb)
{
	swap_endian_2(&cdb->position);
}

void print_fs4000_move_position(FS4000_MOVE_POSITION_CDB *cdb)
{
	printf("MOVE POSITION ");
	printf("%02x %02x pos: %d\n", cdb->unknown1, cdb->unknown2, cdb->position);
}

int fs4000_move_position(BYTE unknown1, BYTE unknown2, UINT2 position)
{
	FS4000_MOVE_POSITION_CDB cdb;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0xe6;
	cdb.unknown1 = unknown1;
	cdb.unknown2 = unknown2;
	cdb.position = position;

	if (fs4000_debug) print_fs4000_move_position(&cdb);
	swap_endian_FS4000_MOVE_POSITION_CDB(&cdb);

	return scsi_do_command(&cdb, sizeof(cdb), 0, 0, 0);
}

/*  -------------------------------------------------

	SET LAMP

    ------------------------------------------------- */

void print_fs4000_set_lamp(FS4000_SET_LAMP_CDB *cdb)
{
	printf("SET LAMP ");
	if (cdb->visible_lamp_on == 1)
		printf("visible lamp on");
	if (cdb->infrared_lamp_on == 1)
		printf("infrared lamp on");
	if ((cdb->visible_lamp_on == 0) && (cdb->infrared_lamp_on == 0))
		printf("lamps off");
	printf("\n");
}

int fs4000_set_lamp(BYTE visible, BYTE infrared)
{
	FS4000_SET_LAMP_CDB cdb;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0xe7;
	cdb.visible_lamp_on = visible;
	cdb.infrared_lamp_on = infrared;

	if (fs4000_debug) print_fs4000_set_lamp(&cdb);

	return scsi_do_command(&cdb, sizeof(cdb), 0, 0, 0);
}

/*  -------------------------------------------------

	GET LAMP

    ------------------------------------------------- */

void swap_endian_FS4000_GET_LAMP_DATA_IN(FS4000_GET_LAMP_DATA_IN *data)
{
	swap_endian_4(&data->visible_lamp_duration);
	swap_endian_4(&data->infrared_lamp_duration);
}

void print_fs4000_get_lamp(FS4000_GET_LAMP_CDB *cdb,
						   FS4000_GET_LAMP_DATA_IN *data)
{
	static FS4000_GET_LAMP_CDB expected_cdb = { 0xea, 0, 0, 0, 0, 0, 0, 0, 10, 0 };

	if (cdb != NULL) {
		printf("GET LAMP ");
		if (memcmp(cdb, &expected_cdb, sizeof(FS4000_GET_LAMP_CDB)) != 0) {
			printf("- UNEXPECTED VALUES: cdb: ");
			print_byte_array((BYTE *)cdb, sizeof(FS4000_GET_LAMP_CDB));
		}
	}

	if (data != NULL) {
		if (data->is_visible_lamp_on == 1)
			printf("visible lamp has been on for %d seconds ", data->visible_lamp_duration);
		if (data->is_infrared_lamp_on == 1)
			printf("infrared lamp has been on for %d seconds ", data->infrared_lamp_duration);
		if ((data->is_visible_lamp_on == 0) && (data->is_infrared_lamp_on == 0))
			printf("lamps off");
		printf("\n");
	}
}

int fs4000_get_lamp(BYTE *is_visible_lamp_on,
					UINT4 *visible_lamp_duration,
					BYTE *is_infrared_lamp_on,
					UINT4 *infrared_lamp_duration)
{
	FS4000_GET_LAMP_CDB cdb;
	FS4000_GET_LAMP_DATA_IN data;
	int result;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0xea;
	cdb.data_length = 10;

	if (fs4000_debug) print_fs4000_get_lamp(&cdb, NULL);

	result = scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_IN, &data, sizeof(data));
	if (result == 0) {
		swap_endian_FS4000_GET_LAMP_DATA_IN(&data);
		if (fs4000_debug) print_fs4000_get_lamp(NULL, &data);

		if (is_visible_lamp_on != NULL)
			*is_visible_lamp_on = data.is_visible_lamp_on;
		if (visible_lamp_duration != NULL)
			*visible_lamp_duration = data.visible_lamp_duration;
		if (is_infrared_lamp_on != NULL)
			*is_infrared_lamp_on = data.is_infrared_lamp_on;
		if (infrared_lamp_duration != NULL)
			*infrared_lamp_duration = data.infrared_lamp_duration;
	}

	return result;
}

/*  -------------------------------------------------

	SET FRAME

    ------------------------------------------------- */

void print_fs4000_set_frame(FS4000_SET_FRAME_CDB *cdb)
{
	printf("SET FRAME ");
	printf("unknown1 = %d\n", cdb->unknown1);
}

int fs4000_set_frame(BYTE frame)
{
	FS4000_SET_FRAME_CDB cdb;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0xe8;
	cdb.unknown1 = frame;

	if (fs4000_debug) print_fs4000_set_frame(&cdb);

	return scsi_do_command(&cdb, sizeof(cdb), 0, 0, 0);
}

/*  -------------------------------------------------

	SET WINDOW FOR THUMBNAIL

    ------------------------------------------------- */

void swap_endian_FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB(FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB *cdb)
{
	swap_endian_2(&cdb->data_length);
}

void print_fs4000_set_window_for_thumbnail(FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB *cdb,
                                           FS4000_SET_WINDOW_DATA_OUT *data)
{
    static FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB expected_cdb = { 0xF0, 0, 0, 0, 0, 0, 0, 72, 0 };
    static SCSI_WINDOW_HEADER expected_values_header = { 0, 0, 0, 0, 0, 64 };

	printf("SET WINDOW FOR THUMBNAIL ");
    if (memcmp(cdb, &expected_cdb, sizeof(FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB)) != 0) {
        printf("\tUNEXPECTED CDB: ");
		print_byte_array((BYTE *)cdb, sizeof(FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB));
    }
	printf("\n");

    if (memcmp(&data->header, &expected_values_header, sizeof(SCSI_WINDOW_HEADER)) != 0) {
        printf("\tUNEXPECTED VALUES: header: ");
        print_byte_array((BYTE *)&data->header, sizeof(SCSI_WINDOW_HEADER));
        printf("\n");
    }

    print_SCSI_WINDOW_DESCRIPTOR(&data->window[0]);
}

int fs4000_set_window_for_thumbnail(UINT2 x_res,
                                    UINT2 y_res,
                                    UINT4 x_upper_left,
                                    UINT4 y_upper_left,
                                    UINT4 width,
                                    UINT4 height,
                                    BYTE bits_per_pixel)
{
    FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB cdb;
    FS4000_SET_WINDOW_DATA_OUT data;

    memset(&cdb, 0, sizeof(cdb));
    cdb.opcode = 0xF0;
    cdb.data_length = 72;

    memset(&data, 0, sizeof(data));
    data.header.window_descriptor_length = 64;
    data.window[0].x_resolution = x_res;
    data.window[0].y_resolution = y_res;
    data.window[0].x_upper_left = x_upper_left;
    data.window[0].y_upper_left = y_upper_left;
    data.window[0].window_width = width;
    data.window[0].window_length = height;
    data.window[0].image_composition = 5;
    data.window[0].bits_per_pixel = bits_per_pixel;
    if (bits_per_pixel == 14)
        data.window[0].image_flags = 3;
    data.window[0].reserved[12] = 2;              //  Need this !!!, WHY ?

    if (fs4000_debug == 1) print_fs4000_set_window_for_thumbnail(&cdb, &data);

    swap_endian_FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB(&cdb);
    swap_endian_FS4000_GETSET_WINDOW_DATA((FS4000_SET_WINDOW_DATA_OUT *)&data);
    return scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_OUT, &data, sizeof(data));
}


/*  -------------------------------------------------

	GET WINDOW FOR THUMBNAIL

    ------------------------------------------------- */

void swap_endian_FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB(FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB *cdb)
{
	swap_endian_2(&cdb->data_length);
}

void print_fs4000_get_window_for_thumbnail(FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB *cdb,
                                           FS4000_GET_WINDOW_DATA_IN *data)
{
    static FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB expected_cdb = { 0xF1, 1, 0, 0, 0, 0, 0, 72, 0 };
    static SCSI_WINDOW_HEADER expected_values_header = { 70, 0, 0, 0, 0, 64 };

    if (cdb != NULL) {
        printf ("GET THUMBNAIL WINDOW ");
        if (memcmp(cdb, &expected_cdb, sizeof(FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB)) != 0) {
            printf("\tUNEXPECTED VALUES: ");
            print_byte_array((BYTE *)cdb, sizeof(FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB));
        }
        printf("\n");
    }

    if (data != NULL) {
        if (memcmp(&data->header, &expected_values_header, sizeof(SCSI_WINDOW_HEADER)) != 0) {
            printf("\tUNEXPECTED VALUES: ");
            printf("header: ");
            print_byte_array((BYTE *)&data->header, sizeof(SCSI_WINDOW_HEADER));
            printf("\n");
        }
        print_SCSI_WINDOW_DESCRIPTOR(&data->window[0]);
    }
}


int fs4000_get_window_for_thumbnail(UINT2 *x_res,
                                    UINT2 *y_res,
                                    UINT4 *x_upper_left,
                                    UINT4 *y_upper_left,
                                    UINT4 *width,
                                    UINT4 *height,
                                    BYTE *bits_per_pixel)
{
    FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB cdb;
    FS4000_GET_WINDOW_DATA_IN data;
    int result;

    memset(&cdb, 0, sizeof(cdb));
    cdb.opcode = 0xF1;
    cdb.flags = 0x01;
    cdb.data_length = 72;

    if (fs4000_debug == 1) print_fs4000_get_window_for_thumbnail(&cdb, NULL);
    swap_endian_FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB(&cdb);

    result = scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_IN, &data, sizeof(data));
    if (result == 0) {
        swap_endian_FS4000_GETSET_WINDOW_DATA(&data);
        if (fs4000_debug == 1) print_fs4000_get_window_for_thumbnail(NULL, &data);

        *x_res = data.window[0].x_resolution;
        *y_res = data.window[0].y_resolution;
        *x_upper_left = data.window[0].x_upper_left;
        *y_upper_left = data.window[0].y_upper_left;
        *width = data.window[0].window_width;
        *height = data.window[0].window_length;
        *bits_per_pixel = data.window[0].bits_per_pixel;
    }

    return result;
}

/*  -------------------------------------------------

	CONTROL LED

    ------------------------------------------------- */

void print_fs4000_control_led(FS4000_CONTROL_LED_CDB *cdb,
							  FS4000_CONTROL_LED_DATA_OUT *data)
{
	static FS4000_CONTROL_LED_CDB expected_cdb = { 0xf3, 0, 0, 0, 0, 0, 0, 0, 4, 0 };

	printf("CONTROL LED ");
	if (memcmp(cdb, &expected_cdb, sizeof(FS4000_CONTROL_LED_CDB)) != 0) {
		printf("- UNEXPECTED CDB: ");
		print_byte_array((BYTE *)cdb, sizeof(FS4000_CONTROL_LED_CDB));
	}

	printf("blink rate %d\n", data->blink_status_led);
}

int fs4000_control_led(int speed)
{
	FS4000_CONTROL_LED_CDB cdb;
	FS4000_CONTROL_LED_DATA_OUT data;

	memset(&cdb, 0, sizeof(cdb));
	cdb.opcode = 0xf3;
	cdb.data_length = 4;

	memset(&data, 0, sizeof(data));
	data.blink_status_led = speed;

	if (fs4000_debug) print_fs4000_control_led(&cdb, &data);

	return scsi_do_command(&cdb, sizeof(cdb), SRB_DIR_OUT, &data, sizeof(data));
}

void swap_endian_SCSI_SENSE(SCSI_SENSE *data)
{
	swap_endian_4(&data->information);
	swap_endian_4(&data->command_specific_information);
}

/*  -------------------------------------------------

	SCSI SENSE

    ------------------------------------------------- */

void print_SCSI_SENSE(SCSI_SENSE *data)
{
	BYTE error = data->error_code & 0x7f;
	BYTE sense_key = data->flags_and_sense_key & 0x0f;
	BYTE asc = data->additional_sense_code;
	BYTE ascq = data->additional_sense_code_qualifier;

	printf("SENSE ");
	if ((asc == 0x80) && (ascq == 0x1c))
		printf("film loaded");
	else if ((asc == 0x80) && (ascq == 0x1d))
		printf("film unloaded");
	else
		printf("unknown codes: asc %02x ascq %02x", asc, ascq);
	printf("\n");
}

static unsigned short int current_line;
static unsigned short int num_bytes_per_line;
static unsigned int rgb_interleave_amount;
static unsigned int rgb_num_lines_buffered;
static PIXEL *rgb_lines[17]; /* 17 being the max # of buffered lines needed. (2n + 1) and the max is 8 lines for 4000dpi mode */

void free_rgb_deinterleave()
{
	unsigned int i;

	for (i = 0; i < rgb_num_lines_buffered; i++) {
		if (rgb_lines[i] != NULL)
			free(rgb_lines[i]);
	}
}

int setup_rgb_deinterleave(unsigned int pixels_per_line, unsigned int interleave_amount)
{
	unsigned int i;
	int error = 0;

	rgb_interleave_amount = interleave_amount;
	rgb_num_lines_buffered = (2 * rgb_interleave_amount) + 1;

	num_bytes_per_line = pixels_per_line * 6;
	for (i = 0; i < rgb_num_lines_buffered; i++) {
		rgb_lines[i] = malloc(num_bytes_per_line);
		if (rgb_lines[i] == NULL)
			error = 1;
	}

	if (error == 1) {
		free_rgb_deinterleave();
	}

	current_line = 0;

	return error;
}

int rgb_deinterleave(PIXEL *inbuf,
					 unsigned short int inbuf_numbytes,
					 PIXEL *outbuf)
{
	int i;
	int retval;

	if (inbuf_numbytes > num_bytes_per_line)
		return -1;

	for (i = 0; i < inbuf_numbytes/2; i++) {
		rgb_lines[current_line % rgb_num_lines_buffered][i] = inbuf[i];
		i++;
		if (current_line >= rgb_interleave_amount) {
			rgb_lines[(current_line - rgb_interleave_amount) % rgb_num_lines_buffered][i] = inbuf[i];
		}
		i++;
		if (current_line >= (rgb_interleave_amount*2)) {
			rgb_lines[(current_line - (rgb_interleave_amount*2)) % rgb_num_lines_buffered][i] = inbuf[i];
		}
	}

	if (current_line < (rgb_interleave_amount*2))
		retval = 0;
	else {
		memcpy(outbuf, rgb_lines[(current_line - (rgb_interleave_amount*2)) % rgb_num_lines_buffered], num_bytes_per_line);
		retval = 1;
	}

	current_line++;
	return retval;
}

int rgb_mirror_line(PIXEL *inbuf,
					unsigned int num_pixels)
{
	/* reverse the order of pixels in the input scan-line, keeping in mind that there are three values per pixel */
	unsigned int i;
	unsigned short int temp;
	PIXEL *p, *q;

	p = inbuf;
	q = &inbuf[(num_pixels * 3) - 3];

	while (p < q) {
		for (i = 0; i < 3; i++) {
			temp = *p;
			*p = *q;
			*q = temp;

			p++;
			q++;
		}
		q -= 6;
	}

	return 0;
}



