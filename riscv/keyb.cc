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

int keyb_irq;
static char outc, fifo[256];
static int head, tail;
static struct termios termios;

keyb_device_t::keyb_device_t()
{
  head = 0;
  tail = 0;
  tcgetattr(0, &termios);
  cfmakeraw(&termios);
  termios.c_oflag |= OPOST;
  tcsetattr(0, 0, &termios);
}

void keyb_poll()
{
  if (head == tail)
    {
      int cnt, rslt = ioctl(0, FIONREAD, &cnt);
      assert(rslt >= 0);
      if (cnt > 0)
        {
          char tmp[cnt+1], *ptr = tmp;
          rslt = read(0, tmp, cnt);
          assert(rslt >= 0);
          tmp[rslt] = 0;
          if ((*tmp < ' ') && (*tmp != 0x0d))
            {
            printf("read(0, \"\\x%.2x\", %d);\n", *tmp, rslt);
            if (*tmp == 0x1c) kill(getpid(), SIGQUIT);
            }
          while (rslt-- > 0)
            {
              fifo[head++] = *ptr++;
              head &= 255;
            }
        }
    }
  keyb_irq = (head != tail);
}

bool keyb_device_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
  int rslt, cnt;
  if (addr + len > 4)
    return false;
  bytes[2] = head == tail ? 1 : 0;
  bytes[1] = outc;
  return true;
}

bool keyb_device_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
  if (addr + len > 4)
    return false;
  outc = fifo[tail++];
  tail &= 255;
  return true;
}
