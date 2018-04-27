// 
// (c) Copyright 2008 - 2013 Xilinx, Inc. All rights reserved.
// 
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
// 
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
// 
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
// 
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
// 

// Copyright 2015 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
// See LICENSE for license details.

`default_nettype none

module usimv_top(
 input 		    sd_clk,
 input 		    rstn,
 input [31:0] 	    core_lsu_addr,
 input wire [31:0]  core_lsu_wdata,
 input 		    core_sd_we,
 input wire 	    sd_detect,
/*
 input wire [3:0]   fifo_status,
 input wire [31:0]  rx_fifo_status,
 input wire [31:0]  tx_fifo_status, 
*/
 input wire [31:0]  data_out_tx,
// input              sd_cmd_to_host,
// input [3:0]        sd_dat_to_host,
//---------------Output ports---------------
 output reg [31:0]  sd_cmd_resp_sel,
 output wire [31:0] data_in_rx,
 output 	    tx_rd,
 output 	    rx_wr,
 output wire [15:0] sd_transf_cnt,		 
 output reg         sd_reset,
 output reg 	    sd_irq,
 output reg [7:0]   clock_divider_sd_clk,
 output reg         sd_data_rst,
 output reg         sd_clk_rst,
 output reg	    sd_cmd_oe,
 output reg	    sd_cmd_to_mem,
 output reg	    sd_dat_oe,
 output reg [3:0]   sd_dat_to_mem,
 output wire [31:0] sd_status
);

   reg 		    sd_cmd_to_host_dly;
   reg [3:0] 	    sd_dat_to_host_dly;
		    
   wire		    start_data;
   wire 	    sd_busy;

logic [6:0] sd_clk_daddr;
logic       sd_clk_dclk, sd_clk_den, sd_clk_drdy, sd_clk_dwe, sd_clk_locked;
logic [15:0] sd_clk_din, sd_clk_dout;
logic [3:0] sd_irq_en, sd_irq_stat;
wire [8:0] sd_xfr_addr;
   
   wire [133:0]     sd_cmd_response;
   wire [31:0] 	    sd_cmd_wait, sd_data_wait;
   wire [6:0] 	    sd_cmd_crc_val;
   wire [47:0] 	    sd_cmd_packet;
   wire [15:0] 	    transf_cnt_o;

   wire       sd_data_busy, data_crc_ok;
   wire [3:0] sd_dat_to_host;
   wire       sd_cmd_to_host;
   wire       sd_cmd_finish, sd_data_finish, sd_cmd_crc_ok, sd_cmd_index_ok;

   reg [2:0]  sd_data_start;
   reg [1:0]  sd_align;
   reg [15:0] sd_blkcnt;
   reg [11:0] sd_blksize;
   
   reg [2:0]  sd_cmd_setting;
   reg [5:0]  sd_cmd_i;
   reg [31:0] sd_cmd_arg;
   reg [31:0] sd_cmd_timeout;
   reg                             sd_cmd_start, sd_cmd_rst, dmy;
   wire [5:0]                      sd_cmd_state;
   wire [6:0]                      sd_data_state;

always @(posedge sd_clk or negedge rstn)
   if (!rstn)
     begin
	sd_align <= 0;
	sd_blkcnt <= 0;
        sd_blksize <= 0;
        sd_data_start <= 0;
	sd_clk_din <= 0;
	sd_clk_den <= 0;
	sd_clk_dwe <= 0;
	sd_clk_daddr <= 0;
        sd_cmd_i <= 0;
        sd_cmd_arg <= 0;
        sd_cmd_setting <= 0;
        sd_cmd_start <= 0;
        sd_reset <= 0;
        sd_data_rst <= 0;
        sd_cmd_rst <= 0;
        sd_clk_rst <= 0;
        sd_cmd_timeout <= 0;
        sd_irq_stat <= 0;
        sd_irq_en <= 0;
        sd_irq <= 0;
     end
   else
     begin
        sd_irq_stat <= {~sd_detect,sd_detect,sd_status[10],sd_status[8]};
        sd_irq <= |(sd_irq_en & sd_irq_stat);
        if (core_sd_we)
          case(core_lsu_addr[5:2])
	    0: sd_align <= core_lsu_wdata[1:0];
            1: sd_clk_din <= core_lsu_wdata[15:0];
            2: sd_cmd_arg <= core_lsu_wdata;
            3: sd_cmd_i <= core_lsu_wdata[5:0];
            4: {sd_data_start,sd_cmd_setting} <= core_lsu_wdata[5:0];
            5: sd_cmd_start <= core_lsu_wdata[0];
            6: {sd_reset,sd_clk_rst,sd_data_rst,sd_cmd_rst} <= core_lsu_wdata[3:0];
	    7: sd_blkcnt <= core_lsu_wdata[15:0];
            8: sd_blksize <= core_lsu_wdata[11:0];
            9: sd_cmd_timeout <= core_lsu_wdata;
	   10: {sd_clk_dwe,sd_clk_den,sd_clk_daddr} <= core_lsu_wdata[8:0];
           11: sd_irq_en <= core_lsu_wdata[3:0];
          endcase
     end

always_comb
     case(core_lsu_addr[6:2])
       0: sd_cmd_resp_sel = sd_cmd_response[38:7];
       1: sd_cmd_resp_sel = sd_cmd_response[70:39];
       2: sd_cmd_resp_sel = sd_cmd_response[102:71];
       3: sd_cmd_resp_sel = {1'b0,sd_cmd_response[133:103]};
       4: sd_cmd_resp_sel = sd_cmd_wait;
       5: sd_cmd_resp_sel = {sd_status[31:4],4'b0};
       6: sd_cmd_resp_sel = sd_cmd_packet[31:0];
       7: sd_cmd_resp_sel = {16'b0,sd_cmd_packet[47:32]};       
       8: sd_cmd_resp_sel = sd_data_wait;
       9: sd_cmd_resp_sel = {16'b0,sd_transf_cnt};
      10: sd_cmd_resp_sel = 32'b0;
      11: sd_cmd_resp_sel = 32'b0;
      12: sd_cmd_resp_sel = {31'b0,sd_detect};
      13: sd_cmd_resp_sel = {23'b0,sd_xfr_addr};
      14: sd_cmd_resp_sel = {28'b0,sd_irq_stat};
      15: sd_cmd_resp_sel = {14'b0,sd_clk_locked,sd_clk_drdy,sd_clk_dout};
      16: sd_cmd_resp_sel = {30'b0,sd_align};
      17: sd_cmd_resp_sel = {16'b0,sd_clk_din};
      18: sd_cmd_resp_sel = sd_cmd_arg;
      19: sd_cmd_resp_sel = {26'b0,sd_cmd_i};
      20: sd_cmd_resp_sel = {26'b0,sd_data_start,sd_cmd_setting[2:0]};
      21: sd_cmd_resp_sel = {31'b0,sd_cmd_start};
      22: sd_cmd_resp_sel = {28'b0,sd_reset,sd_clk_rst,sd_data_rst,sd_cmd_rst};
      23: sd_cmd_resp_sel = {32'b1};
      24: sd_cmd_resp_sel = {20'b0,sd_blksize};
      25: sd_cmd_resp_sel = sd_cmd_timeout;
      26: sd_cmd_resp_sel = {23'b0,sd_clk_dwe,sd_clk_den,sd_clk_daddr};
      27: sd_cmd_resp_sel = {28'b0,sd_irq_en};
     default: sd_cmd_resp_sel = 32'HDEADBEEF;
     endcase // case (core_lsu_addr[6:2])

sd_top sdtop(
    .sd_clk     (sd_clk),
    .cmd_rst    (~(sd_cmd_rst&rstn)),
    .data_rst   (~(sd_data_rst&rstn)),
    .setting_i  (sd_cmd_setting),
    .timeout_i  (sd_cmd_timeout),
    .cmd_i      (sd_cmd_i),
    .arg_i      (sd_cmd_arg),
    .start_i    (sd_cmd_start),
    .sd_data_start_i(sd_data_start),
    .sd_align_i(sd_align),
    .sd_blkcnt_i(sd_blkcnt),
    .sd_blksize_i(sd_blksize),
    .sd_data_i(data_out_tx),
    .sd_dat_to_host(sd_dat_to_host),
    .sd_cmd_to_host(sd_cmd_to_host),
    .finish_cmd_o(sd_cmd_finish),
    .finish_data_o(sd_data_finish),
    .response0_o(sd_cmd_response[38:7]),
    .response1_o(sd_cmd_response[70:39]),
    .response2_o(sd_cmd_response[102:71]),
    .response3_o(sd_cmd_response[133:103]),
    .crc_ok_o   (sd_cmd_crc_ok),
    .index_ok_o (sd_cmd_index_ok),
    .transf_cnt_o(sd_transf_cnt),
    .wait_o(sd_cmd_wait),
    .wait_data_o(sd_data_wait),
    .status_o(sd_status[31:4]),
    .packet0_o(sd_cmd_packet[31:0]),
    .packet1_o(sd_cmd_packet[47:32]),
    .crc_val_o(sd_cmd_crc_val),
    .crc_actual_o(sd_cmd_response[6:0]),
    .sd_rd_o(tx_rd),
    .sd_we_o(rx_wr),
    .sd_data_o(data_in_rx),    
    .sd_dat_to_mem(sd_dat_to_mem),
    .sd_cmd_to_mem(sd_cmd_to_mem),
    .sd_dat_oe(sd_dat_oe),
    .sd_cmd_oe(sd_cmd_oe),
    .sd_xfr_addr(sd_xfr_addr)
    );
  
sd_verilator_model sdflash1 (
             .sdClk(~sd_clk),
             .cmd(sd_cmd_to_mem),
             .cmdOut(sd_cmd_to_host),
             .dat(sd_dat_to_mem),
             .datOut(sd_dat_to_host),
	     .oeCmd(),
	     .oeDat()
);
  
endmodule // chip_top
`default_nettype wire
