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

enum {DEBUG=1};

static unsigned core_lsu_addr, core_lsu_wdata, core_sd_we, sd_detect;
static unsigned sd_cmd_resp_sel, sd_reset, sd_transf_cnt, sd_irq, sd_buf[8192];
static unsigned long sd_poll_cnt;
static int sd_flag;
static FILE *logf;

int sd_poll(void)
{
  if (sd_flag)
    {      
      static int old_irq;
      verilator_loop(core_lsu_addr, core_lsu_wdata, core_sd_we, sd_detect, 
                     &sd_cmd_resp_sel, &sd_reset, &sd_transf_cnt, &sd_irq, sd_buf);
      if (old_irq != sd_irq)
        fprintf(logf, "sd_irq=%d\n", sd_irq);
      old_irq = sd_irq;
      if (sd_poll_cnt++ % 1000 == 0)
          fflush(logf);
      return sd_irq;
    }
  return 0;
}

sd_device_t::sd_device_t()
{
  int argc = 1;
  const char *argv[] = {"a.out", NULL};
  const char *envp[] = {"LANG=en_GB.UTF-8", NULL};
  logf = fopen("sd_device.log", "w");
  verilator_main(argc, (char **)argv, (char **)envp);
  sd_detect = 0;
  sd_flag = 1;
}

bool sd_device_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
  int cnt;
  if (addr + len > 0x10000)
    return false;

  if (addr < 0x8000)
    {
      static uint32_t last[32];
      core_sd_we = 0;
      core_lsu_wdata = 0;
      core_lsu_addr = addr;
      sd_poll();
      sd_poll();
      memcpy(bytes, &sd_cmd_resp_sel, len);
      if (DEBUG && (last[core_lsu_addr >> 2] != sd_cmd_resp_sel)) switch(core_lsu_addr >> 2)
        {
        case  0: fprintf(logf, "load(sd_cmd_response[38:7]) => 0x%X\n", sd_cmd_resp_sel); break;
        case  1: fprintf(logf, "load(sd_cmd_response[70:39]) => 0x%X\n", sd_cmd_resp_sel); break;
        case  2: fprintf(logf, "load(sd_cmd_response[102:71]) => 0x%X\n", sd_cmd_resp_sel); break;
        case  3: fprintf(logf, "load({1'b0,sd_cmd_response[133:103]}) => 0x%X\n", sd_cmd_resp_sel); break;
        case  4: fprintf(logf, "load(sd_cmd_wait) => 0x%X\n", sd_cmd_resp_sel); break;
        case  5: fprintf(logf, "load({sd_status[31:4],4'b0}) => 0x%X\n", sd_cmd_resp_sel); break;
        case  6: fprintf(logf, "load(sd_cmd_packet[31:0]) => 0x%X\n", sd_cmd_resp_sel); break;
        case  7: fprintf(logf, "load({16'b0,sd_cmd_packet[47:32]}) => 0x%X\n", sd_cmd_resp_sel); break;
        case  8: fprintf(logf, "load(sd_data_wait) => 0x%X\n", sd_cmd_resp_sel); break;
        case  9: fprintf(logf, "load({16'b0,sd_transf_cnt}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 10: fprintf(logf, "load(32'b0) => 0x%X\n", sd_cmd_resp_sel); break;
        case 11: fprintf(logf, "load(32'b0) => 0x%X\n", sd_cmd_resp_sel); break;
        case 12: fprintf(logf, "load({31'b0,sd_detect}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 13: fprintf(logf, "load({23'b0,sd_xfr_addr}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 14: fprintf(logf, "load({28'b0,sd_irq_stat}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 15: fprintf(logf, "load({14'b0,sd_clk_locked,sd_clk_drdy,sd_clk_dout}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 16: fprintf(logf, "load({30'b0,sd_align}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 17: fprintf(logf, "load({16'b0,sd_clk_din}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 18: fprintf(logf, "load(sd_cmd_arg) => 0x%X\n", sd_cmd_resp_sel); break;
        case 19: fprintf(logf, "load({26'b0,sd_cmd_i}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 20: fprintf(logf, "load({26'b0,sd_data_start,sd_cmd_setting[2:0]}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 21: fprintf(logf, "load({31'b0,sd_cmd_start}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 22: fprintf(logf, "load({28'b0,sd_reset,sd_clk_rst,sd_data_rst,sd_cmd_rst}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 23: fprintf(logf, "load({16'b0,sd_blkcnt}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 24: fprintf(logf, "load({20'b0,sd_blksize}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 25: fprintf(logf, "load(sd_cmd_timeout) => 0x%X\n", sd_cmd_resp_sel); break;
        case 26: fprintf(logf, "load({23'b0,sd_clk_dwe,sd_clk_den,sd_clk_daddr}) => 0x%X\n", sd_cmd_resp_sel); break;
        case 27: fprintf(logf, "load({28'b0,sd_irq_en}) => 0x%X\n", sd_cmd_resp_sel); break;
        default: fprintf(logf, "load(0x%X) => 0x%X\n", core_lsu_addr, sd_cmd_resp_sel);
        }
      last[core_lsu_addr >> 2] = sd_cmd_resp_sel;
    }
  else
    {
      uint32_t tmp = addr-0x8000 >> 2;
      fprintf(logf, "dataload(0x%X) => 0x%X\n", tmp, sd_buf[tmp]);
      memcpy(bytes, sd_buf+tmp, len);
    }
  return true;
}

bool sd_device_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
  int cnt;
  if (addr + len > 0x10000)
    return false;

  if (addr < 0x8000)
    {
      static uint32_t last[32];
      core_sd_we = 1;
      core_lsu_addr = addr;
      memcpy(&core_lsu_wdata, bytes, len);
      sd_poll();
      core_sd_we = 0;
      sd_poll();
      if (DEBUG && (last[core_lsu_addr >> 2] != core_lsu_wdata)) switch(core_lsu_addr >> 2)
        {
        case  0: fprintf(logf, "store(sd_align, 0x%X);\n", core_lsu_wdata); break;
        case  1: fprintf(logf, "store(sd_clk_din, 0x%X);\n", core_lsu_wdata); break;
        case  2: fprintf(logf, "store(sd_cmd_arg, 0x%X);\n", core_lsu_wdata); break;
        case  3: fprintf(logf, "store(sd_cmd_i, 0x%X);\n", core_lsu_wdata); break;
        case  4: fprintf(logf, "store({sd_data_start,sd_cmd_setting}, 0x%X);\n", core_lsu_wdata); break;
        case  5: fprintf(logf, "store(sd_cmd_start, 0x%X);\n", core_lsu_wdata); break;
        case  6: fprintf(logf, "store({sd_reset,sd_clk_rst,sd_data_rst,sd_cmd_rst}, 0x%X);\n", core_lsu_wdata); break;
        case  7: fprintf(logf, "store(sd_blkcnt, 0x%X);\n", core_lsu_wdata); break;
        case  8: fprintf(logf, "store(sd_blksize, 0x%X);\n", core_lsu_wdata); break;
        case  9: fprintf(logf, "store(sd_cmd_timeout, 0x%X);\n", core_lsu_wdata); break;
        case 10: fprintf(logf, "store({sd_clk_dwe,sd_clk_den,sd_clk_daddr}, 0x%X);\n", core_lsu_wdata); break;
        case 11: fprintf(logf, "store(sd_irq_en, 0x%X);\n", core_lsu_wdata); break;
        default: fprintf(logf, "store(0x%X, 0x%X);\n", core_lsu_addr, core_lsu_wdata);
        }
      last[core_lsu_addr >> 2] = core_lsu_wdata;
    }
  else
    {
      uint32_t tmp = addr-0x8000 >> 2;
      memcpy(sd_buf+tmp, bytes, len);
      fprintf(logf, "datastore(0x%X) => 0x%X\n", tmp, sd_buf[tmp]);
    }

  return true;
}
