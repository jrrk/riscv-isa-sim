#ifdef CURSES
#include <ncurses.h>
#endif
#include "devices.h"

static char screen[4096];
static FILE *logf;

vga_device_t::vga_device_t()
{
#ifdef CURSES
  initscr();
  logf = fopen("vga.log", "w");
#else
  logf = stdout;
#endif  
}

bool vga_device_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
  if (addr + len > sizeof(screen))
    return false;
  memcpy(bytes, screen+addr, len);
  return true;
}

bool vga_device_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
  if (addr + len > sizeof(screen))
    return false;
  memcpy(screen+addr, bytes, len);
#ifdef CURSES
  move(addr/128, addr%128);
#endif
  while (len--)
    {
      int ch = *bytes++ & 0x7F;
      if (ch >= ' ')
        {
#ifdef CURSES
          addch(ch);
#endif
        }
      fputc(ch, logf);
    }
#ifdef CURSES
  refresh();
#endif
  fflush(logf);
  return true;
}
