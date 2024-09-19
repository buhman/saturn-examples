#include <stdint.h>

#include "sh2.h"
#include "serial.h"

volatile uint16_t * const DATATRNS = (volatile uint16_t *)0x25890000; // 0
volatile uint16_t * const DATASTAT = (volatile uint16_t *)0x25890004; // 1 (not implemented in mednafen)
volatile uint16_t * const HIRQREQ  = (volatile uint16_t *)0x25890008; // 2
volatile uint16_t * const HIRQMSK  = (volatile uint16_t *)0x2589000c; // 3
volatile uint16_t * const CDATA0   = (volatile uint16_t *)0x25890018; // 6
volatile uint16_t * const CDATA1   = (volatile uint16_t *)0x2589001c; // 7
volatile uint16_t * const CDATA2   = (volatile uint16_t *)0x25890020; // 8
volatile uint16_t * const CDATA3   = (volatile uint16_t *)0x25890024; // 9
volatile uint16_t * const MPEGRGB  = (volatile uint16_t *)0x25890028; // a (not implemented in mednafen)

/* Bit names of interrupt factor registers (HIRQREQ, HIRQMSK) */
#define HIRQ__CMOK (1 << 0)  /* Command can be issued */
#define HIRQ__DRDY (1 << 1)  /* Data transfer ready */
#define HIRQ__CSCT (1 << 2)  /* 1 sector read completed */
#define HIRQ__BFUL (1 << 3)  /* CD buffer full */
#define HIRQ__PEND (1 << 4)  /* End of CD playback */
#define HIRQ__DCHG (1 << 5)  /* Disk replacement occurrence */
#define HIRQ__ESEL (1 << 6)  /* End of selector setting process */
#define HIRQ__EHST (1 << 7)  /* End of host I/O processing */
#define HIRQ__ECPY (1 << 8)  /* End of copy/move processing */
#define HIRQ__EFLS (1 << 9)  /* End of file system processing */
#define HIRQ__SCDQ (1 << 10) /* Subcode Q update completed */
#define HIRQ__MPED (1 << 11) /* End of MPEG-related processing */
#define HIRQ__MPCM (1 << 12) /* End of MPEG operation undefined interval */
#define HIRQ__MPST (1 << 13) /* MPEG interrupt status notification */

struct cd_command {
  uint16_t CDATA[4];
};

struct cd_response {
  uint16_t CDATA[4];
};

int cd_wait_hirq(uint16_t status)
{
  for (int i = 0; i < 0x240000; i++) {
    uint16_t hirq_temp = *HIRQREQ;
    if ((hirq_temp & status) != 0) {
      return 0;
    }
  }
  return -1;
}

int cd_command_response(cd_command * command, cd_response * response)
{
  uint16_t hirq = *HIRQREQ;
  serial_string("cd_command_response hirq: ");
  serial_integer(hirq, 8, '\n');
  if ((hirq & HIRQ__CMOK) == 0) {
    return -1;
  }

  *HIRQREQ &= ~HIRQ__CMOK;

  *CDATA0 = command->CDATA[0];
  *CDATA1 = command->CDATA[1];
  *CDATA2 = command->CDATA[2];
  *CDATA3 = command->CDATA[3];

  if (cd_wait_hirq(HIRQ__CMOK) != 0) {
    return -1;
  }

  response->CDATA[0] = *CDATA0;
  response->CDATA[1] = *CDATA1;
  response->CDATA[2] = *CDATA2;
  response->CDATA[3] = *CDATA3;

  return 0;
}

int cd_data_end()
{
  struct cd_command command = {{
    0x0600,
    0,
    0,
    0
  }};
  struct cd_response response = {0};

  int ret = cd_command_response(&command, &response);

  *HIRQREQ &= ~HIRQ__DRDY;

  return ret;
}

int cd_wait_data_ready()
{
   int ret = cd_wait_hirq(HIRQ__DRDY);
   if (ret != 0)
     cd_data_end();

  *HIRQREQ &= ~HIRQ__DRDY;

   return ret;
}

int cd_get_data(uint16_t * data, int size)
{
  int ret;

  ret = cd_wait_data_ready();
  if (ret != 0)
    return -1;

  while (size > 0) {
    *data = *DATATRNS;
    data += 1;
    size -= 1;
  }

  ret = cd_data_end();
  if (ret != 0)
    return -1;

  return 0;
}

int cd_get_toc(uint32_t * toc)
{
  struct cd_command command = {{
    0x0200,
    0,
    0,
    0
  }};
  struct cd_response response = {0};

  int ret = cd_command_response(&command, &response);
  if (ret != 0)
    return -1;

  int size = ((response.CDATA[0] & 0xff) << 16)
           | ((response.CDATA[1] & 0xffff) << 0);
  ret = cd_get_data((uint16_t *)toc, size);
  if (ret != 0)
    return -1;

  return 0;
}

int cd_init()
{
  struct cd_command command = {{
    0x0400,
    0,
    0,
    0x040f,
  }};
  struct cd_response response = {0};

  int ret = cd_command_response(&command, &response);
  return ret;
}

void test_cd()
{
  int ret;
  ret = cd_init();
  serial_string("cd_init: ");
  serial_integer(ret, 8, '\n');
  if (ret != 0)
    return;

  uint32_t toc[102];
  ret = cd_get_toc(toc);
  serial_string("cd_get_toc: ");
  serial_integer(ret, 8, '\n');
  if (ret != 0)
    return;

  for (int i = 0; i < 102; i++) {
    serial_integer(toc[i], 8, '\n');
  }
}

void main()
{
  // serial mode register
  sh2.reg.SMR = 0
    | SMR__CA__ASYNCHRONOUS_MODE
    | SMR__CHR__EIGHT_BIT_DATA
    | SMR__PE__PARITY_BIT_NOT_ADDED_OR_CHECKED
    | SMR__OE__EVEN_PARITY
    | SMR__STOP__ONE_STOP_BIT
    | SMR__MP__DISABLED
    | SMR__CKS__PHI_4;

  // actual baud rate at phi_4 = 3579545 / (16 * (5 + 1))
  // 37286.927 ~= 38400
  sh2.reg.BRR = 5;

  sh2.reg.SCR = SCR__TE__TRANSMITTER_ENABLE | SCR__RE__RECEIVER_ENABLE;

  sh2.reg.SSR = 0;

  test_cd();

  while (1);
}
