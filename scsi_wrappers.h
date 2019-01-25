#ifndef _SCSI_WRAPPERS_H_
#define _SCSI_WRAPPERS_H_

#include "wnaspi32.h"

extern  BYTE    byScsiHaId, byScsiTarget;                               // SJS
extern  DWORD   (__cdecl *SendASPI32CommandFunc) (LPSRB);               // SJS

int scsiaspi_init(void);
void scsi_deinit(void);
const char *scsi_decode_asc(unsigned char asc,
                            unsigned char ascq);
void scsi_decode_sense(unsigned char sense,
                       unsigned char asc,
                       unsigned char ascq);
int scsi_do_command(void *cdb,
                    unsigned int cdb_length,
                    int data_dir,
                    void *data_buf,
                    unsigned int data_buf_len);
void scsi_bus_scan(void);

#endif /* _SCSI_WRAPPERS_H_ */
