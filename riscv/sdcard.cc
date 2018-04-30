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
#include "sdcard.h"

int sd_irq;

enum {DEBUG=1};

static uint32_t core_lsu_addr, core_lsu_wdata, core_sd_we, sd_detect, sd_irq_en, sd_irq_stat, mask;
static uint32_t sd_cmd_resp_sel, sd_reset, sd_transf_cnt, sd_buf[8192];
static int sd_flag;
static uint32_t stlast[32];
static uint32_t ldlast[32];
static int mapfd;
static uint8_t *cardmem;

static void die(const char *msg)
{
  perror(msg);
  exit(strlen(msg));
}
  
sd_device_t::sd_device_t()
{
  enum {len=(1UL<<32)-(1<<21)};
  int rslt;
  mapfd = open("cardmem.bin", O_CREAT|O_RDWR, 0666);
  if (mapfd < 0) die("cardmem.bin");
  rslt = ftruncate64(mapfd, len);
  if (rslt < 0) die("ftruncate");
  cardmem = (uint8_t*)mmap64(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, mapfd, 0);
  if (cardmem == MAP_FAILED) die("mmap");
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
      core_sd_we = 0;
      core_lsu_wdata = 0;
      core_lsu_addr = addr;
      sd_cmd_resp_sel = ldlast[core_lsu_addr >> 2];
      memcpy(bytes, &sd_cmd_resp_sel, len);
#ifdef LOGF     
      if (DEBUG || (ldlast[core_lsu_addr >> 2] != sd_cmd_resp_sel)) switch(core_lsu_addr >> 2)
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
        case 14: if (0) fprintf(logf, "load({28'b0,sd_irq_stat}) => 0x%X\n", sd_cmd_resp_sel); break;
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
      fflush(logf);
#endif
    }
  else
    {
      uint32_t tmp = (addr-0x8000) >> 2;
      uint32_t tmp2 = ntohl(sd_buf[tmp]);
#ifdef LOGF
      fprintf(logf, "dataload(0x%X) => 0x%X\n", tmp, tmp2);
#endif
      memcpy(bytes, &tmp2, len);
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
      core_sd_we = 1;
      core_lsu_addr = addr >> 2;
      memcpy(&core_lsu_wdata, bytes, len);
      core_sd_we = 0;
#ifdef LOGF   
      if (DEBUG || (stlast[core_lsu_addr] != core_lsu_wdata)) switch(core_lsu_addr)
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
        case 11: if (0) fprintf(logf, "store(sd_irq_en, 0x%X);\n", core_lsu_wdata); break;
        default: fprintf(logf, "store(0x%X, 0x%X);\n", core_lsu_addr, core_lsu_wdata);
        }
#endif
      stlast[core_lsu_addr] = core_lsu_wdata;
      if (core_lsu_addr==start_reg)
	{
	  if (stlast[start_reg] & 1)
	    {
	      switch(stlast[cmd_reg]&255)
		{
		case 0x0:
		  ldlast[wait_resp] = 0x0;
		  mask = 0;
		  break;
		case 0x2:
		  ldlast[wait_resp] = 0x8;
		  ldlast[resp3] = 0x3F00534D;
		  ldlast[resp2] = 0x534D4920;
		  ldlast[resp1] = 0x20100251;
		  ldlast[resp0] = 0x66450082;
		  break;
		case 0x3:
		  ldlast[wait_resp] = 0x8;
		  ldlast[resp0] = 0xBEEF0700;
		  break;
		case 0x7:
		  ldlast[wait_resp] = 0x8;
		  ldlast[resp0] = 0x508;
		  break;
		case 0x9:
		  ldlast[wait_resp] = 0x8;
#if 1	      
		  ldlast[resp3] = 0x9000E00;
		  ldlast[resp2] = 0x325B5EFF;
		  ldlast[resp1] = 0xFF76B27F;
		  ldlast[resp0] = 0x800A4040;
#else
		  ldlast[resp3] = 0x09000E00;
		  ldlast[resp2] = 0x325B5905;
		  ldlast[resp1] = 0x0076B27F;
		  ldlast[resp0] = 0x800A4040;	      
#endif	      
		  break;
		case 0xD:
		  ldlast[wait_resp] = 0x8;
		  ldlast[resp0] = 0x00000900;
		  ldlast[status_resp] |= 1<<10;
		  break;
		case 0x11:
		  ldlast[wait_resp] = 0x8;
		  ldlast[resp0] = 0x0;
		  ldlast[status_resp] |= 1<<10;
		  memcpy(sd_buf, cardmem+stlast[arg_reg], 512);
		  break;
		case 0x18:
		  ldlast[wait_resp] = 0x8;
		  ldlast[resp0] = 0x00000900;
		  memcpy(cardmem+stlast[arg_reg], sd_buf, 512);
		  break;
		case 0x29:
		  ldlast[wait_resp] = 0x8;
		  ldlast[resp0] = 0xC0FF8000;
		  break;
		case 0x33:
		  ldlast[wait_resp] = 0x8;
		  ldlast[resp0] = 0x0;
		  ldlast[status_resp] |= 1<<10;
		  break;
		case 0x37:
		  ldlast[wait_resp] = 0x8;
		  ldlast[resp0] = 0x120 | mask;
		  mask = 0x800;
		  break;
		case 0x5:
		case 0x34:
		default:
		  ldlast[wait_resp] = stlast[timeout_reg]+1;
		  ldlast[resp0] = 0;
		  break;
		}
	      ldlast[status_resp] |= 1<<8;
	    }
	  else
	    {
	      ldlast[status_resp] = 0;
	    }
        }
      for (int i = 0; i < 16; i++) ldlast[i+16] = stlast[i];
      sd_irq_en = stlast[irq_en_reg];
      sd_irq_stat = 0x8 | (ldlast[status_resp] & (1<<10) ? 2:0) | (ldlast[status_resp] & (1<<8) ? 1:0);
      ldlast[irq_stat_resp] = sd_irq_stat;
      sd_irq = sd_irq_en & sd_irq_stat ? 1 : 0;
    }
  else
    {
      uint32_t tmp = (addr-0x8000) >> 2;
      uint32_t tmp2;
      memcpy(&tmp2, bytes, len);
      sd_buf[tmp] = ntohl(tmp2);
#ifdef LOGF
      fprintf(logf, "datastore(0x%X) => 0x%X\n", tmp, sd_buf[tmp]);
#endif
    }

  return true;
}
 
