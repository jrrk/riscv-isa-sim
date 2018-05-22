// #define CURSES

#ifdef CURSES
#include <ncurses.h>
#endif
#include "devices.h"

static char screen[4096];

static void put(int ch)
{
  static FILE *logf;
  if (!logf)
    {
      logf = fopen("vga.log", "w");
      if (!logf)
        abort();
    }
  if (ch < 0)
    fflush(logf);
  else
    {
      putchar(ch);
      putc(ch, logf);
    }
}

vga_device_t::vga_device_t()
{
#ifdef CURSES
  initscr();
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
#ifdef CURSES
      if (ch >= ' ')
        {
          addch(ch);
        }
#else      
      if (addr++ % 128 == 0)
        put('\n');
#endif
      put(ch);
    }
#ifdef CURSES
  refresh();
#else      
  fflush(stdout);
#endif
  return true;
}
