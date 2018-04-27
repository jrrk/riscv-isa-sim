#ifdef __cplusplus
extern "C" {
#endif
void verilator_main(int argc, char **argv, char **env);
void verilator_loop(unsigned core_lsu_addr, unsigned core_lsu_wdata, unsigned core_sd_we, unsigned sd_detect, 
		    unsigned *sd_cmd_resp_sel, unsigned *sd_reset, unsigned *transf_cnt, unsigned *sd_irq,
                    unsigned sd_buf[]);
void verilator_stop(void);
int verilator_finish(void);
void verilator_eval(void);
void verilator_printf(const char *fmt, ...);
void verilator_clock(void);
extern int sd_irq;
#ifdef __cplusplus
};
#endif

enum {align_reg,clk_din_reg,arg_reg,cmd_reg,
      setting_reg,start_reg,reset_reg,blkcnt_reg,
      blksiz_reg,timeout_reg,clk_pll_reg,irq_en_reg,
      unused1,unused2,unused3,led_reg};

enum {resp0,resp1,resp2,resp3,
      wait_resp,status_resp,packet_resp0,packet_resp1,
      data_wait_resp,trans_cnt_resp,obsolete1,obsolet2,
      detect_resp,xfr_addr_resp,irq_stat_resp,pll_resp,
      align_resp,clk_din_resp,arg_resp,cmd_i_resp,
      setting_resp,start_resp,reset_resp,blkcnt_resp,
      blksize_resp,timeout_resp,clk_pll_resp,irq_en_resp};

enum {SD_APP_OP_COND=41, data_buffer_offset=0x2000};

enum {SD_CARD_RESP_END=1,SD_CARD_RW_END=2, SD_CARD_CARD_REMOVED_0=4, SD_CARD_CARD_INSERTED_0=8};
