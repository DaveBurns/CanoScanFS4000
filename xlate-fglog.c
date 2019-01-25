#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"
#include "fs4000-scsi.h"

/* defines for cross-platform portability */
#ifdef WIN32
#include <io.h>       /* for dup2 */
#define Dup2(x,y) _dup2(x,y)
#define Fileno(x) _fileno(x)
#else
#include <unistd.h>   /* for dup2 */
#define Dup2(x,y) dup2(x,y)
#define Fileno(x) fileno(x)
#endif

/* globals for command-line options */
static FILE *g_inputFile = NULL, *g_outputFile = NULL;
static char g_inputFileName[500], g_outputFileName[500];
int g_vuescan_mode = 0; /* 0 = FilmGet, 1 = VueScan */

#define EXIT_BAD_USAGE           -1
#define EXIT_REWIRE_STDIO_ERROR  -2
#define EXIT_IO_FAILURE          -3

typedef struct {
	char symbolic_name[100];
	unsigned long time_stamp;
	unsigned char cdb[16];
	unsigned int cdb_length;
	unsigned char data[1000];
	unsigned int data_length;
	unsigned int data_logged_length;
	char result_string[100];
} FS4000_TRANSACTION;

static void showUsage(char *name) {
  printf("\n"
         "usage: %s -[h?] -v [-i <inputfile>] [-o <outputfile>]\n\n"

         "\t-i uses <inputfile> instead of stdin\n"
         "\t-o uses <outputfile> instead of stdout\n"
         "\t-v reads a VueScan log instead of FilmGet\n"
         "\t-?, -h this message\n",
         name);

  exit(EXIT_BAD_USAGE);
}


static void process_command_line(int argc, char *argv[])
{
  int c;

  g_inputFileName[0] = '\0';
  g_outputFileName[0] = '\0';

  while((c = getopt(argc, argv, "vi:o:h?")) != EOF) {
    switch(c) {
    case 'i':
      if (strlen(g_inputFileName) > 0) {
        fprintf(stderr, "ERROR: you can not use -i more than once.\n");
        exit(EXIT_BAD_USAGE);
      }
      strcpy(g_inputFileName, optarg);
      break;

    case 'o':
      if (strlen(g_outputFileName) > 0) {
        fprintf(stderr, "ERROR: you can not use -o more than once.\n");
        exit(EXIT_BAD_USAGE);
      }
      strcpy(g_outputFileName, optarg);
      break;

	case 'v':
		g_vuescan_mode = 1;
		break;

    case 'h':
    case '?':
    default:
      showUsage(argv[0]);
      break;
    }
  }

  if ((c == EOF) && (optind < argc)) {
    fprintf(stderr, "ERROR: there is an extra argument that is not recognized: %s\n", argv[optind]);
    exit(EXIT_BAD_USAGE);
  }
}


static void rewire_stdio()
{
  if (strlen(g_inputFileName) > 0) {

    /* get a high-level i/o stream for the file, we handle OS line
       ending differences ourselves */
    g_inputFile = fopen(g_inputFileName, "rb");
    if (g_inputFile == NULL) {
      fprintf(stderr, "ERROR: %s: %s\n", strerror(errno), g_inputFileName);
      exit(EXIT_REWIRE_STDIO_ERROR);
    }

    /* point stdin at the file */
    if (-1 == Dup2(Fileno(g_inputFile), Fileno(stdin))) {
      fprintf(stderr, "ERROR: %s: %s\n", strerror(errno), g_inputFileName);
      exit(EXIT_REWIRE_STDIO_ERROR);
    }
    /* stdin now refers to file g_inputFile */
  }

  if (strlen(g_outputFileName) > 0) {
    /* get a high-level i/o stream for the file, we use UNIX style
       line endings for OS independance */
    g_outputFile = fopen(g_outputFileName, "wb");
    if (g_outputFile == NULL) {
      fprintf(stderr, "ERROR: %s: %s\n", strerror(errno), g_outputFileName);
      exit(EXIT_REWIRE_STDIO_ERROR);
    }

    /* point stdout at the file */
    if (-1 == Dup2(Fileno(g_outputFile), Fileno(stdout))) {
      fprintf(stderr, "ERROR: %s: %s\n", strerror(errno), g_outputFileName);
      exit(EXIT_REWIRE_STDIO_ERROR);
    }
    /* stdout now refers to file g_outputFile */
  }
}

void strip_time_stamp(char *buf)
{
	char *left, *right;

	left = strchr(buf, '[');
	if (left == NULL)
		return;
	right = strchr(left, ']');
	if (right == NULL)
		return;

	strcpy(left + 1, right);
}


void parse_line_of_bytes(unsigned char *outbuf, char *inbuf, unsigned int *num_read)
{
	unsigned int bytes_read_this_line = 0;
	char *token;
	const char *delims = " \t";
	char *dummy;

	token = strtok(inbuf, delims);
	while (token != NULL) {
		outbuf[bytes_read_this_line] = (unsigned char)strtoul(token, &dummy, 16);
		bytes_read_this_line++;
        token = strtok(NULL, delims);
	}
	*num_read += bytes_read_this_line;
}


int read_transaction_vuescan(char *linebuf, FS4000_TRANSACTION *t)
{
	char *dummy;
	char cdb_buf[40];

	if (strncmp(linebuf, "VueScan", 7) == 0)
		return 0;

	if (linebuf[87] == 'S') {
		parse_line_of_bytes(t->data, &linebuf[88], &t->data_length);
		strcpy(t->symbolic_name, "SENSE");
		return 1;
	}
	else
		t->symbolic_name[0] = 0;

	if (linebuf[87] == 'R' || linebuf[87] == 'W') {
		parse_line_of_bytes(t->data, &linebuf[88], &t->data_length);
		t->data_logged_length = t->data_length;
	}
	else {
		t->data_length = t->data_logged_length = 0;
	}

	t->time_stamp = strtoul(linebuf, &dummy, 10);

	memcpy(cdb_buf, &linebuf[37], 36);
	cdb_buf[36] = 0;
	parse_line_of_bytes(t->cdb, cdb_buf, &t->cdb_length);

	/* hack this in for now */
	strcpy(t->result_string, "ERR_NO_ERROR");
	return 1;
}

int read_transaction_filmget(char *linebuf, FS4000_TRANSACTION *t)
{
	char *dummy;
	int result = 0;
	char *p, *right;

	memset(t, 0, sizeof(*t));

	p = linebuf + 1;
	right = strchr(p, ']');
	if (right == NULL)
		goto errorout;
	strncpy(t->symbolic_name, p, right - p);
	t->symbolic_name[right - p] = 0;

	if (gets(linebuf) == NULL)
		return 0;
	p = strstr(linebuf, "TIME : ");
	if (p == NULL)
		goto errorout;
	t->time_stamp = strtoul(p + 7, &dummy, 10);

	if (gets(linebuf) == NULL)
		return 0;
	while (strstr(linebuf, "[CDB]") != NULL)
		if (gets(linebuf) == NULL)
			goto errorout;

	linebuf[50] = 0; /* this prunes off the ASCII representation of the byte data */
	parse_line_of_bytes(t->cdb, linebuf, &t->cdb_length);

	if (gets(linebuf) == NULL)
		goto errorout;

	while (strstr(linebuf, "Escape") != NULL)
		if (gets(linebuf) == NULL)
			goto errorout;

	if (strstr(linebuf, "CheckCondition : ") != NULL) {
		strcpy(t->result_string, &linebuf[19]);
		if (t->result_string[strlen(t->result_string) - 1] == '\r')
			t->result_string[strlen(t->result_string) - 1] = 0;
	}
	else if (strstr(linebuf, "[PDB]") != NULL) {
		if (gets(linebuf) == NULL)
			goto errorout;

		if (strstr(linebuf, "SIZE = ") == NULL)
			goto errorout;

		t->data_length = strtoul(&linebuf[9], &dummy, 10);

		if (gets(linebuf) == NULL)
			goto errorout;

		while (strstr(linebuf, "Escape") != NULL)
			if (gets(linebuf) == NULL)
				goto errorout;

		do {
			linebuf[50] = 0; /* this prunes off the ASCII representation of the byte data */
			parse_line_of_bytes(&t->data[t->data_logged_length], linebuf, &t->data_logged_length);
			if (gets(linebuf) == NULL)
				goto errorout;
		} while ((strstr(linebuf, "Escape") == NULL) && (strstr(linebuf, "CheckCondition") == NULL));

		while (strstr(linebuf, "Escape") != NULL)
			if (gets(linebuf) == NULL)
				goto errorout;

		if (strstr(linebuf, "CheckCondition : ") != NULL) {
			strcpy(t->result_string, &linebuf[19]);
			if (t->result_string[strlen(t->result_string) - 1] == '\r')
				t->result_string[strlen(t->result_string) - 1] = 0;
		}
		else {
			fprintf(stderr, "ERROR: not prepared for this format\n");
			exit(-1);
		}
	}

	return 1;

errorout:
	printf(linebuf);
	return 0;
}


void decode_and_print_transaction(FS4000_TRANSACTION *t)
{
	if (strcmp(t->symbolic_name, "SENSE") == 0) {
		swap_endian_SCSI_SENSE((SCSI_SENSE *)t->data);
		print_SCSI_SENSE((SCSI_SENSE *)t->data);
		return;
	}
	switch(t->cdb[0]) {
		case 0x00 :
			print_fs4000_test_unit_ready((FS4000_TEST_UNIT_READY_CDB *)t->cdb);
			break;
		case 0x12 :
			if (t->cdb[1] == 0)
				print_fs4000_inquiry((FS4000_INQUIRY_CDB *)t->cdb, (FS4000_INQUIRY_DATA_IN *)t->data);
			else {
				swap_endian_FS4000_EXTENDED_INQUIRY_DATA_IN((FS4000_EXTENDED_INQUIRY_DATA_IN *)t->data);
				print_fs4000_extended_inquiry((FS4000_INQUIRY_CDB *)t->cdb, (FS4000_EXTENDED_INQUIRY_DATA_IN *)t->data);
			 }
			break;
		case 0x15 :
			swap_endian_FS4000_MODE_SELECT_DATA_OUT((FS4000_MODE_SELECT_DATA_OUT *)t->data);
			print_fs4000_mode_select((FS4000_MODE_SELECT_CDB *)t->cdb, (FS4000_MODE_SELECT_DATA_OUT *)t->data);
			break;
		case 0x16 :
			print_fs4000_reserve_unit((FS4000_RESERVE_UNIT_CDB *)t->cdb);
			break;
		case 0x17 :
			print_fs4000_release_unit((FS4000_RELEASE_UNIT_CDB *)t->cdb);
			break;
		case 0x1b :
			print_fs4000_scan((FS4000_SCAN_CDB *)t->cdb, (FS4000_SCAN_DATA_OUT *)t->data);
			break;
		case 0x24 :
			swap_endian_FS4000_SET_WINDOW_CDB((FS4000_SET_WINDOW_CDB *)t->cdb);
			swap_endian_FS4000_GETSET_WINDOW_DATA((FS4000_GET_WINDOW_DATA_IN *)t->data);
			print_fs4000_set_window((FS4000_SET_WINDOW_CDB *)t->cdb, (FS4000_SET_WINDOW_DATA_OUT *)t->data);
			break;
		case 0x25 :
			swap_endian_FS4000_GET_WINDOW_CDB((FS4000_GET_WINDOW_CDB *)t->cdb);
			swap_endian_FS4000_GETSET_WINDOW_DATA((FS4000_GET_WINDOW_DATA_IN *)t->data);
			print_fs4000_get_window((FS4000_GET_WINDOW_CDB *)t->cdb, (FS4000_GET_WINDOW_DATA_IN *)t->data);
			break;
		case 0x28 :
			swap_endian_FS4000_READ_CDB((FS4000_READ_CDB *)t->cdb);
			print_fs4000_read((FS4000_READ_CDB *)t->cdb);
			break;
		case 0x34 :
			swap_endian_FS4000_GET_DATA_STATUS_CDB((FS4000_GET_DATA_STATUS_CDB *)t->cdb);
			swap_endian_FS4000_GET_DATA_STATUS_DATA_IN((FS4000_GET_DATA_STATUS_DATA_IN *)t->data);
			print_fs4000_get_data_status((FS4000_GET_DATA_STATUS_CDB *)t->cdb, (FS4000_GET_DATA_STATUS_DATA_IN *)t->data);
			break;
		case 0xd5 :
			if (t->cdb[4] == 38) {
				swap_endian_FS4000_GETSET_SCAN_MODE_DATA((FS4000_GET_SCAN_MODE_DATA_IN_38 *)t->data);
				print_fs4000_get_scan_mode((FS4000_GET_SCAN_MODE_CDB *)t->cdb, (FS4000_GET_SCAN_MODE_DATA_IN_38 *)t->data, NULL);			
			}
			else {
				print_fs4000_get_scan_mode((FS4000_GET_SCAN_MODE_CDB *)t->cdb, NULL, (FS4000_GET_SCAN_MODE_DATA_IN_12 *)t->data);			
			}
			break;
		case 0xd6 :
			swap_endian_FS4000_DEFINE_SCAN_MODE_CDB((FS4000_DEFINE_SCAN_MODE_CDB *)t->cdb);
			swap_endian_FS4000_GETSET_SCAN_MODE_DATA((FS4000_GET_SCAN_MODE_DATA_IN_38 *)t->data);
			print_fs4000_define_scan_mode((FS4000_DEFINE_SCAN_MODE_CDB *)t->cdb, (FS4000_DEFINE_SCAN_MODE_DATA_OUT *)t->data);
			break;
		case 0xd8 :
			//print_FS4000_SCAN_FOR_THUMBNAIL_CDB((FS4000_SCAN_FOR_THUMBNAIL_CDB *)t->cdb);
			//print_FS4000_SCAN_DATA_OUT((FS4000_SCAN_DATA_OUT *)t->data);
			break;
		case 0xe0 :
			swap_endian_FS4000_EXECUTE_AFAE_DATA_OUT((FS4000_EXECUTE_AFAE_DATA_OUT *)t->data);
			print_fs4000_execute_afae((FS4000_EXECUTE_AFAE_CDB *)t->cdb, (FS4000_EXECUTE_AFAE_DATA_OUT *)t->data);
			break;
		case 0xe1 :
			swap_endian_FS4000_GET_FILM_STATUS_DATA_IN((FS4000_GET_FILM_STATUS_DATA_IN_25 *)t->data);
			if (t->cdb[8] == 25)
				print_fs4000_get_film_status((FS4000_GET_FILM_STATUS_CDB *)t->cdb, (FS4000_GET_FILM_STATUS_DATA_IN_25 *)t->data, NULL);
			else
				print_fs4000_get_film_status((FS4000_GET_FILM_STATUS_CDB *)t->cdb, NULL, (FS4000_GET_FILM_STATUS_DATA_IN_28 *)t->data);
			break;
		case 0xe4 :
			print_fs4000_cancel((FS4000_CANCEL_CDB *)t->cdb);
			break;
		case 0xe6 :
			swap_endian_FS4000_MOVE_POSITION_CDB((FS4000_MOVE_POSITION_CDB *)t->cdb);
			print_fs4000_move_position((FS4000_MOVE_POSITION_CDB *)t->cdb);
			break;
		case 0xe7 :
			print_fs4000_set_lamp((FS4000_SET_LAMP_CDB *)t->cdb);
			break;
		case 0xea :
			swap_endian_FS4000_GET_LAMP_DATA_IN((FS4000_GET_LAMP_DATA_IN *)t->data);
			print_fs4000_get_lamp((FS4000_GET_LAMP_CDB *)t->cdb, (FS4000_GET_LAMP_DATA_IN *)t->data);
			break;
		case 0xe8 :
			print_fs4000_set_frame((FS4000_SET_FRAME_CDB *)t->cdb);
			break;
		case 0xf0 :
			//print_FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB((FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB *)t->cdb);
			//print_FS4000_SET_WINDOW_DATA_OUT((FS4000_SET_WINDOW_DATA_OUT *)t->data);
			break;
		case 0xf1 :
			//print_FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB((FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB *)t->cdb);
			//print_FS4000_GET_WINDOW_DATA_IN((FS4000_SET_WINDOW_DATA_OUT *)t->data);
			break;
		case 0xf3 :
			print_fs4000_control_led((FS4000_CONTROL_LED_CDB *)t->cdb, (FS4000_CONTROL_LED_DATA_OUT *)t->data);
			break;
		default:
			printf("UNKNOWN TRANSACTION. CDB = ");
            /* comment this out for now. There's a bug when an unknown command appears with an unexpected format
            print_byte_array(t->cdb, t->cdb_length);
			printf("\nDATA = ");
			print_byte_array(t->data, t->data_logged_length);*/
            printf("\n");
			break;
	}
	if (strcmp(t->result_string, "ERR_NO_ERROR") != 0)
		printf("\tERROR: %s\n", t->result_string);
}


void parse_log(void)
{
	char linebuf[1000];
	FS4000_TRANSACTION t;

	while (!feof(stdin)) {
		if (gets(linebuf) == NULL) {
			if (feof(stdin))
				break;
			else
				fprintf(stderr, "ERROR reading from input: %d, %s\n", errno, strerror(errno));
		}
		if (g_vuescan_mode == 0) {
			if (linebuf[0] != '[') {
				strip_time_stamp(linebuf);
				printf("%s\n", linebuf);
			}
			else {
				if (read_transaction_filmget(linebuf, &t) == 1)
					decode_and_print_transaction(&t);
				else
					printf("%s\n", linebuf);
			}
		}
		else {
			if (linebuf[strlen(linebuf) - 1] == '\r')
				linebuf[strlen(linebuf) - 1] = 0;
			if (read_transaction_vuescan(linebuf, &t) == 1)
				decode_and_print_transaction(&t);
			else
				printf("%s\n", linebuf);
		}
	}
}

int main(int argc, char *argv[])
{
  process_command_line(argc, argv);

  rewire_stdio();

  parse_log();

  return 0;
}
