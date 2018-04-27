#ifdef __cplusplus
extern "C" {
#endif
void verilator_main(int argc, char **argv, char **env);
void verilator_loop(unsigned core_lsu_addr, unsigned core_lsu_wdata, unsigned core_sd_we, unsigned sd_detect, 
		    unsigned *sd_cmd_resp_sel, unsigned *sd_reset, unsigned *transf_cnt, unsigned sd_buf[]);
void verilator_stop(void);
int verilator_finish(void);
void verilator_eval(void);
void verilator_printf(const char *fmt, ...);
void verilator_clock(void);
#ifdef __cplusplus
};
#endif
