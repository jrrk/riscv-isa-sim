#include <unistd.h>
#include <fcntl.h>
#include "Vusimv_top.h"
#include "verilated.h"
#include "sim_main.h"

static Vusimv_top top;
static int tb;

void verilator_printf(const char *fmt, ...)
{
  uint i;
  va_list args;
  char printbuffer[256];
  va_start(args, fmt);
  i = vsnprintf(printbuffer, sizeof(printbuffer), fmt, args);
  va_end(args);
  write(tb, printbuffer, i);
}

void verilator_stop(void)
{
  close(tb);
}

int verilator_finish(void)
{
  return Verilated::gotFinish();
}

static unsigned rstn, sd_clk, core_lsu_addr, core_lsu_wdata, core_sd_we, sd_detect, data_out_tx;

void verilator_eval(void)
{
  if (sd_clk != top.sd_clk)
    {
      sd_clk = top.sd_clk;
      verilator_printf("#1000 sd_clk='H%X;\n", sd_clk);
    }
  if (rstn != top.rstn)
    {
      rstn = top.rstn;
      verilator_printf("rstn='H%X;\n", rstn);
    }
  if (core_lsu_addr != core_lsu_addr)
    {
      core_lsu_addr = top.core_lsu_addr;
      verilator_printf("core_lsu_addr='H%X;\n", core_lsu_addr);
    }
  if (core_lsu_wdata != top.core_lsu_wdata)
    {
      core_lsu_wdata = top.core_lsu_wdata;
      verilator_printf("core_lsu_wdata='H%X;\n", core_lsu_wdata);
    }
  if (core_sd_we != top.core_sd_we)
    {
      core_sd_we = top.core_sd_we;
      verilator_printf("core_sd_we='H%X;\n", core_sd_we);
    }
  if (sd_detect != top.sd_detect)
    {
      sd_detect = top.sd_detect;
      verilator_printf("sd_detect='H%X;\n", sd_detect);
    }
  if (data_out_tx != top.data_out_tx)
    {
      data_out_tx = top.data_out_tx;
      verilator_printf("data_out_tx='H%X;\n", data_out_tx);
    }
  top.eval();
}

void verilator_clock(void)
{
                 verilator_eval();
                 top.sd_clk = 1;
                 verilator_eval();
                 top.sd_clk = 0;
}

void verilator_main(int argc, char **argv, char **env) {
  int cnt;
                 Verilated::commandArgs(argc, argv);
		 tb = open("usimv_top_tb.v",O_CREAT|O_WRONLY|O_TRUNC, 0700);
		 rstn = -1;
		 sd_clk = -1;
		 core_lsu_addr = -1;
		 core_lsu_wdata = -1;
		 core_sd_we = -1;
		 sd_detect = -1;
		 data_out_tx = -1;
                 top.rstn = 0;
                 top.sd_clk = 0;
		 top.core_lsu_addr = 0;
		 top.core_lsu_wdata = 0;
		 top.core_sd_we = 0;
		 top.sd_detect = 0;
		 top.data_out_tx = 0;
		 for (cnt = 6; cnt--; ) verilator_clock();
                 top.rstn = 1;
}

void verilator_loop(unsigned core_lsu_addr, unsigned core_lsu_wdata, unsigned core_sd_we, unsigned sd_detect, 
		    unsigned *sd_cmd_resp_sel, unsigned *sd_reset, unsigned *sd_transf_cnt, unsigned sd_buf[])
{
 top.core_lsu_addr = core_lsu_addr;
 top.core_lsu_wdata = core_lsu_wdata;
 top.core_sd_we = core_sd_we;
 top.sd_detect = sd_detect;
 top.data_out_tx = data_out_tx;
 if (top.tx_rd)
   top.data_out_tx = sd_buf[top.sd_transf_cnt];
 verilator_clock();
 verilator_eval();
 *sd_cmd_resp_sel = top.sd_cmd_resp_sel;
 *sd_reset = top.sd_reset;
 *sd_transf_cnt = top.sd_transf_cnt;
 if (top.rx_wr)
   {
   sd_buf[top.sd_transf_cnt] = top.data_in_rx;
   }
}
