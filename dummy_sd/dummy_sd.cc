/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <getopt.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/stat.h>
#include <signal.h>
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
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <openssl/md5.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>

#include "devices.h"
#include "sim_main.h"

static unsigned core_lsu_addr, core_lsu_wdata, core_sd_we, sd_detect;
static unsigned sd_cmd_resp_sel, sd_reset, sd_transf_cnt, sd_buf[8192];
static unsigned rx_fifo_head, tx_fifo_head;

sd_device_t::sd_device_t()
{
  int argc = 1;
  const char *argv[] = {"a.out", NULL};
  const char *envp[] = {"LANG=en_GB.UTF-8", NULL};
  verilator_main(argc, (char **)argv, (char **)envp);
}

bool sd_device_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
  uint16_t machi0;
  uint32_t machi, rxfcs, rsr, rxlen;
  int rslt, cnt;
  if (addr + len > 0x10000)
    return false;

  if (addr < 0x8000)
    {
      core_sd_we = 0;
      core_lsu_wdata = 0;
      core_lsu_addr = addr;
      verilator_loop(core_lsu_addr, core_lsu_wdata, core_sd_we, sd_detect, 
                     &sd_cmd_resp_sel, &sd_reset, &sd_transf_cnt, sd_buf);
      memcpy(bytes, &sd_cmd_resp_sel, len);
    }
  else
    {

    }
  return true;
}

bool sd_device_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
  uint32_t machi;
  if (addr + len > 0x10000)
    return false;

  
  if (addr < 0x8000)
    {
    core_sd_we = 1;
    core_lsu_addr = addr;
    memcpy(&core_lsu_wdata, bytes, len);
    verilator_loop(core_lsu_addr, core_lsu_wdata, core_sd_we, sd_detect, 
		    &sd_cmd_resp_sel, &sd_reset, &sd_transf_cnt, sd_buf);
    core_sd_we = 0;
    }
  else
    {
      memcpy(sd_buf+addr-0x8000, bytes, len);
    }

  return true;
}
