#ifndef _FS4000_SCSI_H_
#define _FS4000_SCSI_H_

#ifdef _cplusplus
#extern "C" {
#endif

/* pack the structures tightly so that there are no unexpected fill bytes.
   These need to map to SCSI CDBs exactly */
#pragma pack(push, 1)

/* data types used by the FS4000.
   IMPORTANT NOTE: all multi-byte values used by the FS4000 are big-endian
   (with one major exception - see READ_CDB below). Therefore, note how all
   routines in the .c file swap endians before setting/reading values.
*/
#ifndef BYTE
typedef unsigned char BYTE;
#endif
#ifndef UINT2
typedef short unsigned int UINT2;
#endif
#ifndef UINT4
typedef unsigned int UINT4;
#endif
#ifndef PIXEL
/* this is truly a bad name since a pixel is really made up of three of these. To be fixed. */
typedef unsigned short int PIXEL;
#endif

#include <windows.h>

/* if this is set to 0, no debug output to stdout. If <> 0, debug output.
   this can be turned off and on on-the-fly. */
extern int fs4000_debug;

/* This next section contains structure definitions that describe the SCSI CDBs
   (command descriptor blocks) and associated data for those commands.

   Structures with names ending in CDB describe the various SCSI CDBs (intuitive, no?).
   Structures with names ending in DATA_OUT describe data associated with a CDB that goes from the computer to the scanner.
   Structures with names ending in DATA_IN describe data resulting from issuing a CDB that goes from the scanner to the computer.

   If a field is named "reserved", then it is known that it is unused and
             should typically be set to zero. 
   If a field is named "unknown" then it probably has purpose but it is not
             known what the useful values are or what they do.

   When bit numbers are mentioned (e.g. bits 5-7), bit number zero starts at the LSB.
*/

typedef struct {
	BYTE opcode;		/* set this to 0x00 */
	BYTE flags;			/* bits 5-7 = LUN. the rest are reserved. */
	BYTE reserved[3];	/* set this to zero */
	BYTE control;		/* set this to zero */
} FS4000_TEST_UNIT_READY_CDB;

typedef struct {
	BYTE opcode;		/* set this to 0x12 */
	BYTE flags;			/* bits 5-7 = LUN, if bit 0 is 1 then page_code is valid and different data is returned */
	BYTE page_code;		/* for FS4000, set this to 0xf0 for extended data (see flags field). Not sure if other values are legit.  */
	BYTE reserved;		/* set this to zero */
	BYTE data_length;	/* set this to 0x24 for normal inquiry, or 0x82 for extended data (see page_code field) */
	BYTE control;		/* set this to zero */
} FS4000_INQUIRY_CDB;

typedef struct {
	BYTE device_type;				/* 06 is the code for scanner */
	BYTE removable_medium;			/* high bit means removable medium */
	BYTE scsi_version_supported;	/* 2 = this is a SCSI-2 device */
	BYTE io_info;					/* 0 = asynch events not supported, term i/o req not supported, INQUIRY data format is as per SCSI-1 */
	BYTE additional_length;
	BYTE reserved[2];
	BYTE data_xfer_flags;			/* rel addressing not supp, 16 or 32-but data xfers not supp, synchornous data xfer not supp, supports linked commands, tagged command queueing, device responds to the RESET condition with the soft RESET alternative */
	char vendor_id[8];				/* ASCII string, not guaranteed to be null-terminated. FS4000 returns "CANON   "           */
	char product_id[16];			/* ASCII string, not guaranteed to be null-terminated. FS4000 returns "IX-40015G       "   */
	char rev_level[4];				/* ASCII string, not guaranteed to be null-terminated. FS4000 returns "1.07"               */
} FS4000_INQUIRY_DATA_IN;

typedef struct {
	BYTE device_type;				/* 06 is the code for scanner */
	BYTE removable_medium;			/* high bit means removable medium : FS4000 returns 0xf0 */
	BYTE scsi_version_supported;	/* 2 = this is a SCSI-2 device */
	BYTE io_info;					/* FS4000 returns 0 = asynch events not supported, term i/o req not supported, INQUIRY data format is as per SCSI-1 */
	BYTE unknown1;					/* FS4000: seen values of 0x45 */
	UINT2 default_x_resolution;		/* FS4000: 4000 */
	UINT2 default_y_resolution;		/* FS4000: 4000 */
	BYTE resolution_range_quantifier; /* lower 4 bits is Y, bits 4-7 are X. FS4000 is 0x11 */
	UINT2 max_x_resolution;			/* FS4000: 4000 */
	UINT2 max_y_resolution;			/* FS4000: 4000 */
	UINT2 min_x_resolution;			/* FS4000: 60 */
	UINT2 min_y_resolution;			/* FS4000: 60 */
	UINT2 unknown2;					/* FS4000: 0x1fff */
	UINT4 max_x_range;				/* FS4000: 4000 */
	UINT4 max_y_range;				/* FS4000: 5904 (0x1710) */
	UINT2 unknown3;					/* FS4000: 0800 */
	UINT4 unknown4;					/* FS4000: 4000 */
	UINT4 unknown5;					/* FS4000: 5904 (0x1710) */
	BYTE unknown6[92];				/* FS4000: see the .c file for values returned. */
} FS4000_EXTENDED_INQUIRY_DATA_IN;

/* FS4000_MODE_SELECT_CDB: FilmGet does not use this command but VueScan does.
   I believe Canon didn't bother since they already know the specs for the scanner. */
typedef struct {
	BYTE opcode;		/* set this to 0x15 */
	BYTE flags;			/* for FS4000, set to 0x10. bits 5-7 = LUN, bit 4 = page format: 0 for SCSI1, 1 for SCSI2, bits 1-3 are reserved, and bit 0("save pages") tells the device to save these pages. */
	BYTE reserved[2];	/* set this to zero */
	BYTE data_length;	/* set this to 0xc */
	BYTE control;		/* set this to zero */
} FS4000_MODE_SELECT_CDB;

typedef struct {
	BYTE page_code;					/* bits 0-5 should be 3 for Measurement Units Page. Bit 6 is reserved and bit 7 is only used for MODE_SENSE command. */
	BYTE param_length;				/* should be 6 for this page */
	BYTE basic_measurement_unit;	/* 00h Inch, 01h Millimetre, 02h Point, 03h - FFh Reserved (from SCSI 2 spec, table 298) FS4000 = 0 */
	BYTE reserved1;
	UINT2 measurement_unit_divisor;	/* the number of units needed to equal one basic measurement unit. FS4000 = 4000 */
	BYTE reserved2;
	BYTE reserved3;
} SCSI2_MEASUREMENT_UNITS_PAGE;

typedef struct {
	BYTE unknown[4];
	SCSI2_MEASUREMENT_UNITS_PAGE measurement_units_page;
} FS4000_MODE_SELECT_DATA_OUT;

/* RESERVE UNIT (0x16) reserves device for the caller. If already reserved,
   RESOLUTION CONFLICT (0x18) is returned except any device can always do
   INQUIRY and REQUEST SENSE. It's ok for one initiator to call RESERVE UNIT
   multiple times on one device. */
typedef struct {
	BYTE opcode;		/* set this to 0x16 */
	BYTE flags;			/* for FS4000, set to zero. Bits 5-7 = LUN, bits 1-4 = 3rd party initiator stuff, bit 0 = reserved */
	BYTE reserved[3];	/* set this to zero */
	BYTE control;		/* set this to zero */
} FS4000_RESERVE_UNIT_CDB;

/* RELEASE UNIT (0x17). Always returns GOOD even if not reserved. */
typedef struct {
	BYTE opcode;		/* set this to 0x17 */
	BYTE flags;			/* for FS4000, set to zero. Bits 5-7 = LUN, bits 1-4 = 3rd party initiator stuff, bit 0 = reserved */
	BYTE reserved[3];	/* set this to zero */
	BYTE control;		/* set this to zero */
} FS4000_RELEASE_UNIT_CDB;

typedef struct {
	BYTE opcode;		/* set this to 0x1b */
	BYTE flags;			/* for FS4000, set to zero. bits 5-7 = LUN, the rest are reserved. */
	BYTE reserved[2];	/* set this to zero */
	BYTE data_length;	/* set this to 0x1 */
	BYTE control;		/* set this to zero */
} FS4000_SCAN_CDB;

typedef struct {
	BYTE window_id;
} FS4000_SCAN_DATA_OUT;

typedef struct {
	BYTE opcode;			/* set this to 0x24 */
	BYTE flags;				/* for FS4000, set to 0. bits 5-7 = LUN, the rest are reserved. */
	BYTE reserved[4];		/* set this to zero */
	BYTE data_length_msb;	/* set this to zero */
	UINT2 data_length;		/* set this to 0x48 */
	BYTE control;			/* set this to zero */
} FS4000_SET_WINDOW_CDB;

typedef struct {
	BYTE opcode;			/* set this to 0x25 */
	BYTE flags;				/* for FS4000, set this to 0x01. bits 5-7 = LUN, bits 1-4 are reserved, bit 0 = "single" - if 1 then return only the descriptor asked for. */
	BYTE reserved[3];		/* set this to zero */
	BYTE window_id;			/* id of window to retrieve descriptor for */
	BYTE data_length_msb;	/* set this to zero */
	UINT2 data_length;		/* set this to 0x48 */
	BYTE control;			/* set this to zero */
} FS4000_GET_WINDOW_CDB;

typedef struct {
	UINT2 window_data_length; /* window data length not including the 8-byte header - for SET WINDOW, this is reserved and should be zero */
	BYTE reserved[4];		/* set this to zero */
	UINT2 window_descriptor_length; /* length in bytes of a single window descriptor. Each descriptor shall be of equal length. The first forty-eight bytes are defined in this International Standard and the remaining bytes (in this case 72 - 8 - 48 = 16) in each descriptor are vendor-specific. */
} SCSI_WINDOW_HEADER;

typedef struct {
	BYTE window_id;		/* starting from zero. typically the only value since
						   I've never seen more than one window used on the FS4000 */
	BYTE flags;			/* LSB = Auto: When used with the GET WINDOW command,
						   an auto bit of zero indicates that the window was defined
						   directly by the SET WINDOW command. */
	UINT2 x_resolution;	/* FS4000: known valid values: 4000, 2000, 1000, 500, 160, 0 = 'default' */
	UINT2 y_resolution;	/* FS4000: known valid values: 4000, 2000, 1000, 500, 160, 0 = 'default' */

    /* For scanners, the y-axis is parallel to the direction of scanner travel
       so, for the FS4000, coord (0, 0) is in the lower left corner as you
       look at the holder */
    UINT4 x_upper_left;	/* FS4000: Specifies the x-axis coordinate of the upper
						   left corner of the window. This coordinate is measured
						   from the scan line using the target's current measurement
						   unit divisor (see 15.3.3.1). */
	UINT4 y_upper_left;	/* FS4000: Specifies the y-axis coordinate of the upper
						   left corner of the window. This coordinate is measured
						   from the scan line using the target's current measurement
						   unit divisor (see 15.3.3.1). */
	UINT4 window_width;	/* FS4000: Width of window in the scan line direction.
						   The window width is measured using the target's current
						   measurement unit divisor (see 15.3.3.1). For example,
						   even if the scanning resolution is 160dpi, to scan a
						   full width frame, this should be set to 4000 */
	UINT4 window_length;/* FS4000: Length of window in the scan line direction.
						   The window length is measured using the target's current
						   measurement unit divisor (see 15.3.3.1). For example,
						   even if the scanning resolution is 160dpi, to scan a
						   full length frame, this should be set to 5904 */
	BYTE brightness;	/* FS4000: seen 0x80. zero specifies the default brightness
						   or automatic brightness control, if it is supported.
						   Any other value indicates a relative brightness setting,
						   with 255 being the highest setting, one being the lowest
						   setting, and 128 being the nominal setting. */
	BYTE threshold;		/* FS4000: seen 0x80. the threshold at which scan data is
						   converted to binary data. A value of zero specifies the
						   default threshold or automatic threshold control if it
						   is supported. Any other value indicates relative
						   threshold setting, with 255 being the highest setting,
						   one being the lowest setting, and 128 being the nominal setting. */
	BYTE contrast;		/* FS4000: seen 0x80. zero specifies the default contrast
						   or automatic contrast control, if it is supported.
						   Any other value indicates a relative contrast setting,
						   with 255 being the highest setting, one being the lowest
						   setting, and 128 being the nominal setting. */
	BYTE image_composition;	/* FS4000: seen 5. Possible values per SCSI2 spec:
												00h Bi-level black & white
												01h Dithered/halftone black & white
												02h Multi-level black & white (gray scale)
												03h Bi-level RGB colour
												04h Dithered/halftone RGB colour
												05h Multi-level RGB colour */
	BYTE bits_per_pixel;	/* FS4000: seen both 8 and 14. */
	UINT2 halftone_pattern;	/* FS4000: seen 0. the level of halftone at which
							   the scan data is converted to binary data. The
							   values in this field are vendor-specific. The halftone
							   field is used in conjunction with the image composition field. */
	BYTE image_flags;		/* if MSB = zero, white pixels are zeros and black
							   pixels are ones. If MSB = one, white pixels are
							   ones and black pixels are zeros. This field is
							   applicable only for images represented by one
							   bit per pixel. */
							/* bits 0-2 = padding type:
									00h No padding (above when BPP = 8)
									01h Pad with 0's to byte boundary
									02h Pad with 1's to byte boundary
									03h Truncate to byte boundary (FS4000: Seen this when BPP = 14 */
	UINT2 bit_order;		/* FS4000: seen 0. specifies the order in which data
							   is transferred to the host from the window. The bit
							   ordering specifies the direction of pixels in a scan
							   line, the direction of scan lines within a window
							   and the image data packing within a byte. The values
							   in this field are vendor-specific. */
	BYTE compression_type;	/* FS4000: seen 0 which means no compression. */
	BYTE compression_arg;	/* FS4000: seen 0. seems unused since compression_type == 0. */
	BYTE reserved[14];		/* value seen in vuescan for byte 13 of 14: 2 <-- seems to be required or else error occurs */ 
	BYTE vendor_specific[16]; /* set to zero */
} SCSI_WINDOW_DESCRIPTOR;

typedef struct {
	SCSI_WINDOW_HEADER header;
	SCSI_WINDOW_DESCRIPTOR window[1];
} FS4000_GET_WINDOW_DATA_IN, FS4000_SET_WINDOW_DATA_OUT;

/* FS4000_READ_CDB: Notes about image data format.
   - Although all other data types used in CDBs and parameter structures are
     big-endian, image data is returned in little-endian format.
   - The FS4000 is a 14-bit scanner but values are returned in 16-bits - no
     packing to save bits but simpler to handle.
   - Image data is returned "mirrored" horizontally so it must be mirrored again
     in software to be correct.
   - The image data is returned in RGB order. So each pixel is 6 bytes: 2 bytes R,
     2 bytes G, 2 bytes B.
   - No matter what the size of the window to be scanned, whether 1x1 or 4000x5904,
     each scanline is returned with a "margin" of 40 pixels (240 bytes). It's not
     clear what the 40 pixels are for. They might be meaningless. Currently, I throw
     them away. It seems to be black except for sensor noise. The only time the
     margin is not black is when SET_FRAME is set to 12 when FilmGet prepares
     to do pre-scans before a scan. Strange.
   - The image scanlines are interleaved and the amount of the interleaving
     depends on the resolution.
     If the scan resolution is 160,  the interleave amount is 0.
     If the scan resolution is 500,  the interleave amount is 1.
     If the scan resolution is 1000, the interleave amount is 2.
     If the scan resolution is 2000, the interleave amount is 4.
     If the scan resolution is 4000, the interleave amount is 8.
     What this means, for instance, is that if you scan at 4000dpi, a pixel's
     R-value comes from the image data 8 lines before and the B-value comes
     from 8 lines ahead. If you don't de-interleave the data and then load the
     image in Photoshop, you'll see an image where the RGB channels are offset
     from each other. This interleaving is the reason why, although the extended
     inquiry command returns a max vertical resolution of 5904, FilmGet will
     only allow the scan window to be 5888 (which is 5904 - 16 where the 16
     comes from the total amount of offset 2 channels has at 4000dpi).
   - When reading data, it must be read in multiples of bytes_per_scanline as
     returned from GET_DATA_STATUS. Note that it is worth writing the
     extra code to read multiple scanlines in each read operation. If you don't,
     the overhead of a read operation is large enough to make the scanner
     pause until the application can catch up.
   - The gamma of the image data is not known except for a given set of gain/exposure
     settings and even then it may change over the lifetime of the scanner's lamp. Still,
     with reasonable settings, applying a gamma ramp of about 2.2 gives a reasonable image.

*/
typedef struct {
	BYTE opcode;				/* set this to 0x28 */
	BYTE flags;					/* for FS4000, set to 0x10. bits 5-7 = LUN, the rest are reserved. */
	BYTE data_type_code;		/* I've only seen 00h Image on the FS4000. Other SCSI valid values: 01h Vendor-specific, 02h Halftone mask, 03h Gamma function */
	BYTE reserved;				/* set this to zero */
	UINT2 data_type_qualifier;	/* vendor-specific. For FS4000, set this to zero. */
	BYTE data_length_msb;		/* set this and data_length to whatever the bytes_per_block returned from GET DATA STATUS */
	UINT2 data_length;			/* see above */
	BYTE control;				/* set this to zero */
} FS4000_READ_CDB;

typedef struct {
	BYTE opcode;		/* set this to 0x34 */
	BYTE flags;			/* for FS4000, set to 0x0. bits 5-7 = LUN, bits 1-4 are reserved, bit 0 = "wait". */
	BYTE reserved[5];	/* set this to zero */
	UINT2 data_length;	/* set this to 0x1c */
	BYTE control;		/* set this to zero */
} FS4000_GET_DATA_STATUS_CDB;

/* FS4000_GET_DATA_STATUS_DATA_IN: this doesn't seem to follow the structure defined in the SCSI-2 spec */
typedef struct {
	BYTE unknown1[8];			/* these seem to be zero always */
	UINT4 total_bytes_to_read;	/* total number of bytes to read, separated into blocks of bytes per scan line */
	UINT4 bytes_per_scanline;	/* number of bytes in each scan line */
	UINT4 num_scanlines;		/* number of scan lines to read, i.e. max number of times to invoke the READ command. total_bytes_to_read = num_scanlines * bytes_per_scanline */
	UINT4 total_bytes_to_read2;	/* almost always seems to be the same as total_bytes_to_read */
	BYTE unknown2[4];			/* FS4000: always seem to be zero */
} FS4000_GET_DATA_STATUS_DATA_IN;

typedef struct {
	BYTE opcode;		/* set this to 0xd5 */
	BYTE unknown1;		/* FS4000: I've only seen 0 here. */
	BYTE unknown2;		/* FS4000: I've only seen 0x20 or 0x02 here but 0x02 is very rare */
	BYTE unknown3;
	BYTE data_length;	/* set to 0x26 if unknown2 is 0x20, to 0x0c if unknown2 is 02 - NOTE: these are the sizes of the respective DATA_OUTs below*/
	BYTE control;		/* should be zero */
} FS4000_GET_SCAN_MODE_CDB;

typedef struct {
	BYTE opcode;		/* set this to 0xd6 */
	BYTE unknown1;		/* set this to 0x10 */
	BYTE unknown2;		/* set this to 0 */
	BYTE unknown3;		/* set this to 0 */
	BYTE data_length;	/* set this to 0x26 */
	BYTE control;		/* set this to zero */
} FS4000_DEFINE_SCAN_MODE_CDB;

typedef struct {
	BYTE unknown1[15];		/* always seems to be 25 00 00 00 20 20 00 00 00 00 00 00 00 00 00 for GET SCAN MODE and */
							/*                    00 00 00 00 20 20 00 00 00 00 00 00 00 00 00 for DEFINE SCAN MODE */
							/* first byte is length of rest of data (in this case 37 bytes) */
	BYTE scanning_speed;	/* 2 for 'normal'. Use 4 for 2x or +1 stop, 8 for 4x or +2 stops, vuescan uses 12 for low-exp pass. Legal range is [0-12] */
	BYTE unknown2[6];		/* always seems to be 01 19 00 00 00 00 for GET SCAN MODE and */
							/*                    00 00 00 00 00 00 for DEFINE SCAN MODE - except that calibration sees 03 for byte 5 of both */
							/* for bytes 0-5, tested valid value ranges seem to be: 0-1, 0-255, 0, 0, 0-35, 0-255 */

	/* The next three arrays control the RGB gain and exposure in some TBD way.
	   Each array's values are arranged in RGB order.
	   FilmGet iterates through several sets of values for these while calculating exposure.
	   VueScan does not do any of that - it always uses the same values (at
	   least for slides). VueScan uses 47, 36, 36 and 281, 264, 261 and 750, 352, 235. */

	/* unknown3:
	   Valid values are [0-63]. Typical is 0x3e 0x3e 0x3e and 0x2f 0x24 0x24.
	   Is this some black-point value? Someone suggested that this
	   might be the start time for exposure. */
	BYTE unknown3[3];

	/* unknown4:
	   Valid values are [0-511]. I've seen 0x116 0x115 0x117 and 0x116, 5, 7 plus variations +/- 1 or 2.
	   Also seen 0xff 0xff 0xff and 0x55 0x55 0x55 during calibration.
	   Is this the end time for exposure? Something gamma-related?
	   I'm no longer 100% sure that this is a two byte value. it's possible that
	   it's a one byte integer value plus a one-byte boolean value, per channel.
	   I wonder this because I did an experiment where I scanned a 1x1 window
	   many times, looping through several values for this field. I then took
	   the resulting image data and graphed it. There was an increasing curve
	   from 0-255 and then a discontinuity at 256 and then it decreased from there. */
	UINT2 unknown4[3];

	/* unknown5:
	   Valid values are [0-1000]. Typical FilmGet values for slides are 750, 750, 443(+/-).
	   For FilmGet -2 exp, 488, 204, 110. For -1 exp, 750, 408, 221.
       For FilmGet +1 and +2 exp, use 750, 750, 443 but slow scanning speed down (see scanning_speed above)
	   For FilmGet FARE IR pass, 1000, 1000, 1000.
	   Typical values for FilmGet scanning negatives are 750, 750, 750.
	   During auto-exposure, FilmGet sets this to zero.
	   I'm pretty sure that this is the gain setting for the sensor. */
	UINT2 unknown5[3];

	BYTE unknown6;			/* Valid values seem to be [0-2], [129-132]. No idea what this does. */
} FS4000_GET_SCAN_MODE_DATA_IN_38, FS4000_DEFINE_SCAN_MODE_DATA_OUT;

typedef struct {
	BYTE unknown[12];	/* first byte is 11 which is the length of the rest of the data in bytes */
} FS4000_GET_SCAN_MODE_DATA_IN_12;

/* FS4000_SCAN_FOR_THUMBNAIL_CDB: I haven't experimented with this at all. */
typedef struct {
	BYTE opcode;		/* set this to 0xd8 */
	BYTE flags;			/* for FS4000, set to zero. bits 5-7 = LUN, the rest are reserved. */
	BYTE reserved[2];	/* set this to zero */
	BYTE data_length;	/* set this to 0x1 */
	BYTE control;		/* set this to zero */
} FS4000_SCAN_FOR_THUMBNAIL_CDB;

typedef struct {
	BYTE opcode;		/* set this to 0xe0 */
	BYTE unknown[7];	/* set this to zero */
	BYTE data_length;	/* set this to 0x8 */
	BYTE control;		/* set this to zero */
} FS4000_EXECUTE_AFAE_CDB;

typedef struct {
	BYTE unknown1[3];		/* have seen values 1,0,0 and 2,0,0 - thought first byte is probably lamp # (1 for vis, 2 for IR) but vuescan only uses 2,0,0 */
	BYTE focus_position;	/* Valid values are [75-200]. These correspond to FilmGet's GUI values of 0-100.
							   0x80 for auto-focus?, 20 in GUI meant 0x64 (100), 80 in GUI meant 0xAF(175), 100 in GUI = 0xC8 (200), 0 in GUI meant 0x4B (75) */

	/* for the next two values, usually see FilmGet use 500, 3500 for slides, and 263, 3738 for negs */
	UINT2 unknown2;			/* valid values are [0, 1000] if unknown1 = 1, [0, 4000] for unknown1 = 2 */
	UINT2 unknown3;			/* valid values are [unknown2, 4000] */
} FS4000_EXECUTE_AFAE_DATA_OUT;

typedef struct {
	BYTE opcode;		/* set this to 0xe1 */
	BYTE unknown[7];	/* set this to zero */
	BYTE data_length;	/* sometimes set to 0x19, others 0x1c */
	BYTE control;		/* set this to zero */
} FS4000_GET_FILM_STATUS_CDB;

typedef struct {
	BYTE film_holder_type;	/* 0 = none, 1 = neg holder, 2 = slide holder, 0 = empty APS holder */
	BYTE num_frames;		/* number of frames the current holder has.
							   6 for neg holder
							   4 for slide holder
							   ?? for APS holder */
	UINT2 film_position;	/* what was set in MOVE POSITION */
	BYTE unknown1[3];
	BYTE focus_position;	/* what was set in EXECUTE_AFAE? */
	UINT2 unknown2;			/* this == whatever was unknown2 for the most recent execute af/ae */
	UINT2 unknown3;			/* this == whatever was unknown3 for the most recent execute af/ae */
	BYTE unknown4[13];		/* first 12 bytes look like 3 long ints. These only
							   change when an EXECUTE AFAE is done but no idea
							   what they mean. RGB-related values? */
} FS4000_GET_FILM_STATUS_DATA_IN_25;

typedef struct {
	BYTE film_holder_type;	/* 0 = none, 1 = neg holder, 2 = slide holder, 0 = empty APS holder */
	BYTE num_frames;		/* number of frames the current holder has. 
							   6 for neg holder
							   4 for slide holder
							   ?? for APS holder */
	UINT2 film_position;	/* what was set in MOVE POSITION */
	BYTE unknown1[3];
	BYTE focus_position;	/* what was set in EXECUTE_AFAE? */
	UINT2 unknown2;			/* this == whatever was unknown2 for the most recent execute af/ae */
	UINT2 unknown3;			/* this == whatever was unknown3 for the most recent execute af/ae */
	BYTE unknown4[16];		/* first 12 bytes look like 3 long ints. These only
							   change when an EXECUTE AFAE is done but no idea
							   what they mean. */
} FS4000_GET_FILM_STATUS_DATA_IN_28;

typedef struct {
	BYTE opcode;		/* set this to 0xe4 */
	BYTE reserved[8];	/* set this to zero */
	BYTE control;		/* set this to zero */
} FS4000_CANCEL_CDB;

typedef struct {
	BYTE opcode;		/* set this to 0xe6 */

    /* for unknown1 and 2, valid values:
	   unknown1     unknown2
       0            0           not sure. Seems required before some (all?) EXECUTE AF/AE operations
       1            0           moves the holder all the way into the scanner. This is
                                used to reset the scanner's knowledge of where the holder is in case
                                it was moved manually by the user.
       1            1           eject the holder
       1            2           move relative to the current position? (never seen FilmGet or VueScan use this)
       1            3           ?
       1            4           absolute positioning. What FilmGet and VueScan use most. */
	BYTE unknown1;
	BYTE unknown2;

    /* position:
       It's not clear what the units are for this. When unknown1/2 have been 1 and 4, the following values have been seen.
       Each of the following lists has two numbers for each frame in each holder. The first is the position where FilmGet
       moves to before doing an EXECUTE AF/AE and the second is the position to begin the scan.
       slide    holder: (292, 554), (1072, 1332), (1850, 2110), (2628, 2888)
       negative holder: (358, 606),  (836, 1084), (1316, 1564), (1794, 2042), (2272, 2520), (2750, 3000)
       position used for slide calibration: 694 (not sure if this differs for negs)
       */
    UINT2 position;
	BYTE reserved[4];	/* set this to zero */
	BYTE control;		/* set this to zero */
} FS4000_MOVE_POSITION_CDB;

/* NOTES: FS4000 doesn't complain if you turn on both lamps at once. Not sure if this actually works or, if it does, what the implications are */
typedef struct {
	BYTE opcode;			/* set this to 0xe7 */
	BYTE reserved1;			/* set this to zero */
	BYTE visible_lamp_on;	/* if 1, visible lamp is turned on, if 0 then off. */
	BYTE infrared_lamp_on;	/* if 1, infrared lamp is turned on, if 0 then off. */
	BYTE reserved2[5];		/* set this to zero */
	BYTE control;			/* set this to zero */
} FS4000_SET_LAMP_CDB;

typedef struct {
	BYTE opcode;		/* set this to 0xea */
	BYTE reserved[7];	/* set this to zero */
	BYTE data_length;	/* set this to 0x0a */
	BYTE control;		/* set this to zero */
} FS4000_GET_LAMP_CDB;

/* knowing how long the lamps have been on is useful for calibration.
   When FilmGet takes forever to calibrate, although it does some useful scans,
   it mainly just sits for some number of seconds to allow the lamp to warm up. */
typedef struct {
	BYTE is_visible_lamp_on;		/* == 1 if the visible lamp is on */
	UINT4 visible_lamp_duration;	/* number of seconds that the visible lamp has been on */
	BYTE is_infrared_lamp_on;		/* == 1 if the infrared lamp is on */
	UINT4 infrared_lamp_duration;	/* number of seconds that the infrared lamp has been on */
} FS4000_GET_LAMP_DATA_IN;

/* So far, this is a mysterious command. what does "frame" refer to? Setting it to zero
   works for scanning images. When FilmGet calibrates, it sets frame to 12 then scans
   a 4000x60 window at resolution of 4000x160. */
typedef struct {
	BYTE opcode;		/* set this to 0xe8 */
	BYTE unknown1;		/* valid values: [0-4] and [8-12].
										0 seems to be normal scan and normal direction,
										1 = reverse scan direction (will need to deinterleave in reverse as well - code not written).
										2 + 3 repeat the top and bottom lines respectively. (?? why?)
										4 gives noise with no actual scanning and the 40 pixel sidebar has noise too.
										Values 8-12 seem to repeat 0-4. Canon's software uses 0, 1 (for the IR pass for FARE), and 12. */
	BYTE reserved[7];	/* set this to zero */
	BYTE control;		/* set this to zero */
} FS4000_SET_FRAME_CDB;

/* FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB and FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB
   I haven't used these at all */
typedef struct {
	BYTE opcode;			/* set this to 0xf0 */
	BYTE flags;				/* for FS4000, set to 0. bits 5-7 = LUN, the rest are reserved. */
	BYTE reserved[4];		/* set this to zero */
	BYTE data_length_msb;	/* set this to zero */
	UINT2 data_length;		/* set this to 0x48 */
	BYTE control;			/* set this to zero */
} FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB;

typedef struct {
	BYTE opcode;			/* set this to 0xf1 */
	BYTE flags;				/* for FS4000, set to 0x01. bits 5-7 = LUN, bits 1-4 are reserved, bit 0 = "single" - if 1 then return only the descriptor asked for. */
	BYTE reserved[3];		/* set this to zero */
	BYTE window_id;			/* id of window to retrieve descriptor for, typically zero */
	BYTE data_length_msb;	/* set this to zero */
	UINT2 data_length;		/* set this to 0x48 */
	BYTE control;			/* set this to zero */
} FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB;

typedef struct {
	BYTE opcode;		/* set this to 0xf3 */
	BYTE reserved[7];	/* set this zero */
	BYTE data_length;	/* set this to 0x04 */
	BYTE control;		/* set this to zero */
} FS4000_CONTROL_LED_CDB;

typedef struct {
	BYTE blink_status_led;	/* valid values: 0 = no blink, 2 = slow, 4 = medium, 3 = fast */
	BYTE reserved[3];		/* set this to zero */
} FS4000_CONTROL_LED_DATA_OUT;

/*
	[SENSE DATA WHEN FILM HOLDER IS LOADED]
	Negholder in:     f0  0 49  0  0  0  0  6  0  0  0  0 80 1c  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
    Negholder out:    f0  0 49  0  0  0  0  6  0  0  0  0 80 1d  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
	Slide holder in:  f0  0 49  0  0  0  0  6  0  0  0  0 80 1c  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
    Slide holder out: f0  0 49  0  0  0  0  6  0  0  0  0 80 1d  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
	APS holder in:    f0  0 44  0  0  0  0  6  0  0  0  0 80 12  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
    APS holder out:   f0  0 49  0  0  0  0  6  0  0  0  0 80 1d  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
*/
typedef struct {
	BYTE error_code; /* MSB = 1 if the information field adheres to ISO, the rest are the error code */
	BYTE segment_number;
	BYTE flags_and_sense_key;	/* bit 7 = Filemark (set to 0), bit 6 = EOM (FS4000 = 1?), bit 5 = ILI (set to 0), bit 4 = reserved, bits 0-3 = sense key */
	UINT4 information;
	BYTE additional_sense_length;
	UINT4 command_specific_information;
	BYTE additional_sense_code;
	BYTE additional_sense_code_qualifier;
	BYTE unused[18];
} SCSI_SENSE;

#pragma pack(pop)

const char *convert_opcode_to_symbolic_name(BYTE opcode);
void print_byte_array(BYTE *bytes, unsigned int num_bytes);

void print_fs4000_test_unit_ready(FS4000_TEST_UNIT_READY_CDB *cdb);
int fs4000_test_unit_ready(void);
void print_fs4000_inquiry(FS4000_INQUIRY_CDB *cdb, FS4000_INQUIRY_DATA_IN *data);
int fs4000_inquiry(FS4000_INQUIRY_DATA_IN *data);
void swap_endian_FS4000_EXTENDED_INQUIRY_DATA_IN(FS4000_EXTENDED_INQUIRY_DATA_IN *data);
void print_fs4000_extended_inquiry(FS4000_INQUIRY_CDB *cdb, FS4000_EXTENDED_INQUIRY_DATA_IN *data);
int fs4000_extended_inquiry(FS4000_EXTENDED_INQUIRY_DATA_IN *data);
void swap_endian_FS4000_MODE_SELECT_DATA_OUT(FS4000_MODE_SELECT_DATA_OUT *data);
void print_fs4000_mode_select(FS4000_MODE_SELECT_CDB *cdb, FS4000_MODE_SELECT_DATA_OUT *data);
int fs4000_mode_select();
void print_fs4000_reserve_unit(FS4000_RESERVE_UNIT_CDB *cdb);
int fs4000_reserve_unit();
void print_fs4000_release_unit(FS4000_RELEASE_UNIT_CDB *cdb);
int fs4000_release_unit();
void print_fs4000_scan(FS4000_SCAN_CDB *cdb, FS4000_SCAN_DATA_OUT *data);
int fs4000_scan();
void swap_endian_FS4000_GETSET_WINDOW_DATA(FS4000_GET_WINDOW_DATA_IN *data);
void swap_endian_FS4000_SET_WINDOW_CDB(FS4000_SET_WINDOW_CDB *cdb);
void print_fs4000_set_window(FS4000_SET_WINDOW_CDB *cdb, FS4000_SET_WINDOW_DATA_OUT *data);
int fs4000_set_window(UINT2 x_res,
					  UINT2 y_res,
					  UINT4 x_upper_left,
					  UINT4 y_upper_left,
					  UINT4 width,
					  UINT4 height,
					  BYTE bits_per_pixel,
					  BYTE image_composition);
void swap_endian_FS4000_GET_WINDOW_CDB(FS4000_GET_WINDOW_CDB *cdb);
void print_fs4000_get_window(FS4000_GET_WINDOW_CDB *cdb, FS4000_GET_WINDOW_DATA_IN *data);
int fs4000_get_window(UINT2 *x_res, UINT2 *y_res, UINT4 *x_upper_left, UINT4 *y_upper_left, UINT4 *width, UINT4 *height, BYTE *bits_per_pixel);
void swap_endian_FS4000_READ_CDB(FS4000_READ_CDB *cdb);
void print_fs4000_read(FS4000_READ_CDB *cdb);
int fs4000_read(int buffer_size, PIXEL *buffer);
void swap_endian_FS4000_GET_DATA_STATUS_CDB(FS4000_GET_DATA_STATUS_CDB *cdb);
void swap_endian_FS4000_GET_DATA_STATUS_DATA_IN(FS4000_GET_DATA_STATUS_DATA_IN *data);
void print_fs4000_get_data_status(FS4000_GET_DATA_STATUS_CDB *cdb, FS4000_GET_DATA_STATUS_DATA_IN *data);
int fs4000_get_data_status(UINT4 *num_blocks, UINT4 *num_bytes_per_block);
void swap_endian_FS4000_GETSET_SCAN_MODE_DATA(FS4000_GET_SCAN_MODE_DATA_IN_38 *data);
void print_fs4000_get_scan_mode(FS4000_GET_SCAN_MODE_CDB *cdb, FS4000_GET_SCAN_MODE_DATA_IN_38 *data_38, FS4000_GET_SCAN_MODE_DATA_IN_12 *data_12);
int fs4000_get_scan_mode(int rare, FS4000_GET_SCAN_MODE_DATA_IN_38 *data_38, FS4000_GET_SCAN_MODE_DATA_IN_12 *data_12);
void swap_endian_FS4000_DEFINE_SCAN_MODE_CDB(FS4000_DEFINE_SCAN_MODE_CDB *cdb);
void print_fs4000_define_scan_mode(FS4000_DEFINE_SCAN_MODE_CDB *cdb, FS4000_DEFINE_SCAN_MODE_DATA_OUT *data);
int fs4000_define_scan_mode(FS4000_DEFINE_SCAN_MODE_DATA_OUT *data);
void print_fs4000_scan_for_thumbnail(FS4000_SCAN_FOR_THUMBNAIL_CDB *cdb);
int fs4000_scan_for_thumbnail();
void swap_endian_FS4000_EXECUTE_AFAE_DATA_OUT(FS4000_EXECUTE_AFAE_DATA_OUT *data);
void print_fs4000_execute_afae(FS4000_EXECUTE_AFAE_CDB *cdb, FS4000_EXECUTE_AFAE_DATA_OUT *data);
int fs4000_execute_afae(BYTE unknown1a, BYTE unknown1b, BYTE unknown1c, BYTE focus_position, UINT2 unknown2, UINT2 unknown3);
void swap_endian_FS4000_GET_FILM_STATUS_DATA_IN(FS4000_GET_FILM_STATUS_DATA_IN_25 *data);
void print_fs4000_get_film_status(FS4000_GET_FILM_STATUS_CDB *cdb, FS4000_GET_FILM_STATUS_DATA_IN_25 *data25, FS4000_GET_FILM_STATUS_DATA_IN_28 *data28);
int fs4000_get_film_status(int shorter, BYTE *film_holder, BYTE *num_frames, UINT2 *film_position, BYTE *focus_position);
void print_fs4000_cancel(FS4000_CANCEL_CDB *cdb);
int fs4000_cancel();
void swap_endian_FS4000_MOVE_POSITION_CDB(FS4000_MOVE_POSITION_CDB *cdb);
void print_fs4000_move_position(FS4000_MOVE_POSITION_CDB *cdb);
int fs4000_move_position(BYTE unknown1, BYTE unknown2, UINT2 position);
void print_fs4000_set_lamp(FS4000_SET_LAMP_CDB *cdb);
int fs4000_set_lamp(BYTE visible, BYTE infrared);
void swap_endian_FS4000_GET_LAMP_DATA_IN(FS4000_GET_LAMP_DATA_IN *data);
void print_fs4000_get_lamp(FS4000_GET_LAMP_CDB *cdb, FS4000_GET_LAMP_DATA_IN *data);
int fs4000_get_lamp(BYTE *is_visible_lamp_on, UINT4 *visible_lamp_duration, BYTE *is_infrared_lamp_on, UINT4 *infrared_lamp_duration);
void print_fs4000_set_frame(FS4000_SET_FRAME_CDB *cdb);
int fs4000_set_frame(BYTE frame);
void swap_endian_FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB(FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB *data);
void print_fs4000_set_window_for_thumbnail(FS4000_SET_WINDOW_FOR_THUMBNAIL_CDB *cdb,
                                           FS4000_SET_WINDOW_DATA_OUT *data);
int fs4000_set_window_for_thumbnail(UINT2 x_res,
                                    UINT2 y_res,
                                    UINT4 x_upper_left,
                                    UINT4 y_upper_left,
                                    UINT4 width,
                                    UINT4 height,
                                    BYTE bits_per_pixel);
void swap_endian_FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB(FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB *data);
void print_fs4000_get_window_for_thumbnail(FS4000_GET_WINDOW_FOR_THUMBNAIL_CDB *cdb,
                                           FS4000_GET_WINDOW_DATA_IN *data);
int fs4000_get_window_for_thumbnail(UINT2 *x_res,
                                    UINT2 *y_res,
                                    UINT4 *x_upper_left,
                                    UINT4 *y_upper_left,
                                    UINT4 *width,
                                    UINT4 *height,
                                    BYTE *bits_per_pixel);
void print_fs4000_control_led(FS4000_CONTROL_LED_CDB *cdb, FS4000_CONTROL_LED_DATA_OUT *data);
int fs4000_control_led(int speed);

void swap_endian_SCSI_SENSE(SCSI_SENSE *data);
void print_SCSI_SENSE(SCSI_SENSE *data);

/* utility functions for handling image data retuend from the FS4000. */

/* setup_rgb_deinterleave: allocates memory used for de-interleaving.
   pixels_per_line depends on how wide your scanning window is.
   interleave_amount depends on the scanning resolution.
   returns 0 on success, 1 for failure. */
int setup_rgb_deinterleave(unsigned int pixels_per_line, unsigned int interleave_amount);

/* free_rgb_deinterleave: frees memory allocated in setup_rgb_deinterleave. */
void free_rgb_deinterleave();

/* rgb_deinterleave: deinterleaves the input pixel data.
   returns 0 if success but data not ready.
   returns 1 if success but outbuf contains a deinterleaved line.
   The reason you can have success but data not ready is that deinterleaving
   requires a minimum number of lines before it can assemble a deinterleaved one. */
int rgb_deinterleave(PIXEL *inbuf,
					 unsigned short int inbuf_numbytes,
					 PIXEL *outbuf);

/* rgb_mirror_line: this simply mirrors the input scanline. It is basically a
   classic "string-mirror" routine except that each "character" (i.e. pixel) is
   three bytes long instead of just one. */
int rgb_mirror_line(PIXEL *inbuf,
					unsigned int num_pixels);

/* --------------------------------------------------------------------------------------
   ORDER OF OPERATIONS (1)

   Knowing the above commands in not enough. One must know the order which to apply them.
   What follows is a transcript of FilmGet scanning a slide full-frame. I've formatted
   the output to see the loops more easily and I've condensed a lot of the info so that
   you can see it all compactly. Note the debug output from FilmGet re auto-exposure.
   -------------------------------------------------------------------------------------- 

    CANCEL 
    RESERVE UNIT 
    CONTROL LED blink rate 2
    GET LAMP visible lamp is on for 209 seconds
    GET SCAN MODE (short) 

    INQUIRY 
    TEST UNIT READY 
    INQUIRY (extended)
    GET FILM STATUS (short) 
    MOVE POSITION 0 0 0
    SET WINDOW 
    GET WINDOW 
    GET FILM STATUS (short) 
    MOVE POSITION 1 0 0
    GET FILM STATUS (rare) 
    MOVE POSITION 0 0 0

    +pBanffAE->SetScan_POSI[]
	    DEFINE SCAN MODE 
	    GET SCAN MODE 
	    Repeat 3x:
		    GET SCAN MODE 
		    GET FILM STATUS 
		    SET FRAME 12 
		    DEFINE SCAN MODE
			    1st time:	(61, 40, 36) (255, 255, 255) (  0,   0,   0) (131)
			    2nd time:	(61, 40, 36) ( 85,  85,  85) (  0,   0,   0) (131)
			    3rd time:	(61, 40, 36) (278,   6,  10) (750, 750, 451) (131)
		    SET WINDOW	(4000x4000) (0, 0, 1, 1) 14-bit RGB
		    SCAN 
		    GET DATA STATUS total_bytes_to_read: 246, bytes_per_scanline: 246, num_scanlines: 1
		    READ data_length: 246 
		    SET FRAME 0
		    DEFINE SCAN MODE 
    -pBanffAE->SetScan_POSI[]

    +pBanffSH->GetBlackShadingVisible[]
        GET SCAN MODE 
        Repeat 2x:
	        GET SCAN MODE 
	        GET FILM STATUS 
	        SET FRAME 12 
	        DEFINE SCAN MODE 
			        1st time:	(61, 40, 36) (255, 255, 255) (  0,   0,   0) (131)
			        2nd time:	(61, 40, 36) ( 85,  85,  85) (  0,   0,   0) (131)
	        SET WINDOW	(4000x4000) (0, 0, 1, 1) 14-bit
	        SCAN 
	        GET DATA STATUS total_bytes_to_read: 246, bytes_per_block: 246, num_blocks: 1
	        READ data_length: 246 
	        SET FRAME 0
	        DEFINE SCAN MODE 

        DEFINE SCAN MODE	(61, 40, 36) (278,   5,  10) (750, 750, 451) (131)
        SET FRAME 12 
        GET WINDOW 
        SET WINDOW	(500 x 4000) (0, 0, 4000, 60) 14-bit  <-- I "lied" a little up above. For any horizontal resolution, you can have a vertical resolution of 4000dpi
        TEST UNIT READY 
        SCAN 
        GET DATA STATUS total_bytes_to_read: 194400, bytes_per_block: 3240, num_blocks: 60
        Repeat 60x:
	        READ data_length: 3240 

        SET WINDOW 
    -pBanffSH->GetBlackShadingVisible[]

    GET FILM STATUS (short) 
    MOVE POSITION 1 4 292
    GET FILM STATUS (short) 
    MOVE POSITION 0 0 0
    EXECUTE AF/AE unknown1: 1 0 0 focus_pos: 0 unknown2 500 unknown3 3500 
    GET FILM STATUS (short) 
    MOVE POSITION 1 4 554
    SET FRAME 0 
    SET WINDOW 
    GET WINDOW 
    SCAN 
    GET DATA STATUS total_bytes_to_read: 2391120, bytes_per_block: 3240, num_blocks: 738
    Repeat 738x:
	    READ data_length: 61560

    BEGIN FARE --------------------------------------------------------------------------------
        if FilmGet is going to do dust-removal with FARE, it inserts the following commands but
        note that the most recent scan done above is done with SET FRAME 8, not SET FRAME 0.

        INQUIRY 
        TEST UNIT READY 
        INQUIRY (extended)

        SET LAMP infrared lamp on
        GET FILM STATUS (short) 
        EXECUTE AF/AE data: unknown1: 2 0 0 focus_pos: 103 unknown2 0 unknown3 0 
        +pBanffAE->SetWShadingI[]
            DEFINE SCAN MODE    (62, 62, 62) (255, 255, 255) (1000, 1000, 1000) (131)
            GET SCAN MODE
            Repeat 2x
                GET SCAN MODE
                GET FILM STATUS (long)  focus position: 0x67 (103)
                SET FRAME 12
                DEFINE SCAN MODE
			        1st time:	(62, 62, 62) (255, 255, 255) (  0,   0,   0) (131)
			        2nd time:	(62, 62, 62) ( 85,  85,  85) (  0,   0,   0) (131)
		        SET WINDOW	(4000x4000) (0, 0, 1, 1) 14-bit RGB
                SCAN
                GET DATA STATUS total_bytes_to_read: 246, bytes_per_scanline: 246, num_scanlines: 1
                READ data_length: 246 
                SET FRAME 8
                DEFINE SCAN MODE
            DEFINE SCAN MODE    (62, 62, 62) (280, 277, 278) (1000, 1000, 1000) (131)
        -pBanffAE->SetWShadingI[]

        SET FRAME 1
        SET WINDOW	(2000x2000) (0, 0, 4000, 5888) 14-bit gray-scale
        GET WINDOW 
        SCAN 
        GET DATA STATUS     total_bytes_to_read: 794880, bytes_per_scanline: 1080, num_scanlines: 736
        Repeat 12x
            READ data_length: 63720 
        READ data_length: 30240 
        SET LAMP visible lamp on
        GET FILM STATUS (short) 
        EXECUTE AF/AE data: unknown1: 2 0 0 focus_pos: 134 unknown2 0 unknown3 0 
        TEST UNIT READY 
        GET FILM STATUS (short) 
        MOVE POSITION 00 00 pos: 0
    END FARE -------------------------------------------------------------------------------------

    CANCEL 
    CANCEL 
    CONTROL LED 0
    RELEASE UNIT 

   --------------------------------------------------------------------------------------
   ORDER OF OPERATIONS (2)

   The following seems to be the minimum to make things work. This looks like do_scan()
   in fs4000test.c but I've removed the optional things.
   --------------------------------------------------------------------------------------

	RESERVE UNIT            <-- gotta do this for SCSI
	CANCEL                  <-- might be optional but best practice to reset things just in case

    SET LAMP 1 0            <-- turn on the visible-light lamp
	MOVE POSITION 1 0 0     <-- bring the holder all the way into the unit so the scanner starts in a known position. Otherwise, it could move somewhere that isn't where it thinks.
	MOVE POSITION 1 4 292   <-- move to the position for AF/AE for frame 1.
	MOVE POSITION 0 0 0     <-- not sure what this does but it seems to be necessary for the EXECUTE AE/AF to work 100% of the time.
    EXECUTE AFAE 1 0 0 0 500 3500   <-- wish I knew how this really worked.
	MOVE POSITION 1 4 554   <-- move to the position to start scanning for frame 1
	DEFINE SCAN MODE        <-- set the scanning RGB, speed, etc. parameters.
	SET FRAME 0             <-- mysterious command for normal scans
	SET WINDOW (dpi, dpi) (0, 0, 4000, 5904, 14); <-- full frame scan
	SCAN                    <-- scan. This is asynchronous!
	GET DATA STATUS         <-- the scanner tells us how much data is coming back
    repeat X times:
        READ

    SET LAMP 0 0            <-- turn the lamp off. In a real scanning app, I'd leave this on until app exit so the lamp stayed warmed up.
	RELEASE UNIT            <-- let it go for SCSI

   -------------------------------------------------------------------------------------- */

#ifdef _cplusplus
}
#endif

#endif /* _FS4000_SCSI_H_ */
