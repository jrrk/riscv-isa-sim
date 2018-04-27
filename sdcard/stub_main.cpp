#include <stdlib.h>
#include "sim_main.h"

static unsigned core_lsu_addr, core_lsu_wdata, core_sd_we, sd_detect;
static unsigned sd_cmd_resp_sel, sd_reset, sd_transf_cnt, sd_buf[8192];

int main(int argc, char **argv, char **envp)
{
  verilator_main(argc, argv, envp);
  while (!verilator_finish())
    {
      verilator_loop(core_lsu_addr, core_lsu_wdata, core_sd_we, sd_detect, 
		     &sd_cmd_resp_sel, &sd_reset, &sd_transf_cnt, sd_buf);
    }
  verilator_stop();
  exit(0);
}
