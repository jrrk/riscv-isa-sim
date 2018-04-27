#include <unistd.h>
#include <fcntl.h>
#include "Vusimv_top.h"
#include "verilated.h"
#include "sim_main.h"

static Vusimv_top top;

void verilator_stop(void)
{

}

int verilator_finish(void)
{
  return Verilated::gotFinish();
}

void verilator_eval(void)
{
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
		    unsigned *sd_cmd_resp_sel, unsigned *sd_reset, unsigned *sd_transf_cnt, unsigned *sd_irq,
                    unsigned sd_buf[])
{
 top.core_lsu_addr = core_lsu_addr;
 top.core_lsu_wdata = core_lsu_wdata;
 top.core_sd_we = core_sd_we;
 top.sd_detect = sd_detect;
 if (top.tx_rd)
   top.data_out_tx = sd_buf[top.sd_transf_cnt];
 verilator_clock();
 verilator_eval();
 *sd_cmd_resp_sel = top.sd_cmd_resp_sel;
 *sd_reset = top.sd_reset;
 *sd_transf_cnt = top.sd_transf_cnt;
 *sd_irq = top.sd_irq;
 if (top.rx_wr)
   {
   sd_buf[top.sd_transf_cnt] = top.data_in_rx;
   }
}
