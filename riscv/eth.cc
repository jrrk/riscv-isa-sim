#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>

#include "devices.h"
#include "eth.h"

static uint32_t machi, maclo, txlen, txfcs, mdio, rxfcs, rsr, rxlen, reset;

eth_device_t::eth_device_t()
{
  machi = 0x2301;
  maclo = 0x00890702;
}

bool eth_device_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
  int rslt, cnt;
  if (addr + len > 0x8000)
    return false;

  switch(addr)
    {
    case MACLO_OFFSET: printf("eth_load(MACLO_OFFSET);\n"); memcpy(bytes, &maclo, len); break;
    case MACHI_OFFSET: printf("eth_load(MACHI_OFFSET);\n"); memcpy(bytes, &maclo, len); break;
    case TPLR_OFFSET: printf("eth_load(TPLR_OFFSET);\n"); memcpy(bytes, &txlen, len); break;
    case TFCS_OFFSET: printf("eth_load(TFCS_OFFSET);\n"); memcpy(bytes, &txfcs, len); break;
    case MDIOCTRL_OFFSET: printf("eth_load(MDIOCTRL_OFFSET);\n"); memcpy(bytes, &mdio, len); break;
    case RFCS_OFFSET: printf("eth_load(RFCS_OFFSET);\n"); memcpy(bytes, &rxfcs, len); break;
    case RSR_OFFSET: printf("eth_load(RSR_OFFSET);\n"); memcpy(bytes, &rsr, len); break;
    case RPLR_OFFSET: printf("eth_load(RPLR_OFFSET);\n"); memcpy(bytes, &rxlen, len); break;
      
    }
  return true;
}

bool eth_device_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
  if (addr + len > 0x8000)
    return false;

  switch(addr)
    {
    case MACLO_OFFSET: printf("eth_store(MACLO_OFFSET);\n"); memcpy(&maclo, bytes, len); break;
    case MACHI_OFFSET: printf("eth_store(MACHI_OFFSET);\n"); memcpy(&machi, bytes, len); break;
    case TPLR_OFFSET: printf("eth_store(TPLR_OFFSET);\n"); memcpy(&txlen, bytes, len); break;
    case TFCS_OFFSET: printf("eth_store(TFCS_OFFSET);\n"); /* cancel */ break;
    case MDIOCTRL_OFFSET: printf("eth_store(MDIOCTRL_OFFSET);\n"); memcpy(&mdio, bytes, len); break;
    case RFCS_OFFSET: printf("eth_store(RFCS_OFFSET);\n"); /* unused */ break;
    case RSR_OFFSET: printf("eth_store(RSR_OFFSET);\n"); memcpy(&reset, bytes, len); break;
    case RPLR_OFFSET: printf("eth_store(RPLR_OFFSET);\n"); /* unused */ break;
      
    }

  return true;
}
