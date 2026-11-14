//Copyright (C)2014-2025 Gowin Semiconductor Corporation.
//All rights reserved.
//File Title: Template file for instantiation
//Tool Version: V1.9.11.03 Education
//Part Number: GW1NR-LV9QN88PC6/I5
//Device: GW1NR-9
//Device Version: C
//Created Time: Tue Nov 10 12:23:48 2026

//Change the instance name and port connections to the signal names
//--------Copy here to design--------

	Gowin_PicoRV32_Top your_instance_name(
		.gpio_io(gpio_io), //inout [13:0] gpio_io
		.wbspi_master_miso(wbspi_master_miso), //input wbspi_master_miso
		.wbspi_master_mosi(wbspi_master_mosi), //output wbspi_master_mosi
		.wbspi_master_ssn(wbspi_master_ssn), //output [0:0] wbspi_master_ssn
		.wbspi_master_sclk(wbspi_master_sclk), //output wbspi_master_sclk
		.wbi2c_sda(wbi2c_sda), //inout wbi2c_sda
		.wbi2c_scl(wbi2c_scl), //inout wbi2c_scl
		.jtag_TDI(jtag_TDI), //input jtag_TDI
		.jtag_TDO(jtag_TDO), //output jtag_TDO
		.jtag_TCK(jtag_TCK), //input jtag_TCK
		.jtag_TMS(jtag_TMS), //input jtag_TMS
		.clk_in(clk_in), //input clk_in
		.resetn_in(resetn_in) //input resetn_in
	);

//--------Copy end-------------------
