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

eth_device_t::eth_device_t()
{

}

bool eth_device_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
  int rslt, cnt;
  if (addr + len > 0x8000)
    return false;
  memset(bytes, 0, len);
  return true;
}

bool eth_device_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
  if (addr + len > 0x8000)
    return false;

  return true;
}
