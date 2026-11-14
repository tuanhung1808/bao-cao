// top.v - Tang Nano 9K + Gowin_PicoRV32
//   TFT ST7789 qua WB SPI Master + WB GPIO (DC, RES)
//   DS3231 + BH1750 qua WB I2C Master (chung 1 bus)
//   4 nut bam ngoai qua WB GPIO (bit 2..5), muc thap = nhan
// Khong dung UART. IP: WB UART TAT, WB GPIO Data Width = 6.
module soc_top (
    input  wire sys_clk,     // thach anh 27 MHz, chan 52
    input  wire sys_rst_n,   // nut S1 onboard, chan 4 (nhan = muc thap)

    // ---- I2C chung cho DS3231 va BH1750 ----
    inout  wire I2C_SCL,
    inout  wire I2C_SDA,

    // ---- Man hinh TFT ST7789 ----
    output wire tft_sclk,    // chan 37
    output wire tft_mosi,    // chan 36
    output wire tft_cs,      // chan 38 (do ssn cua SPI IP dieu khien)
    inout  wire tft_dc,      // chan 39 = gpio_io[0]
    inout  wire tft_res,     // chan 40 = gpio_io[1]

    // ---- 4 nut ngoai (noi xuong GND, pull-up trong FPGA) ----
    inout  wire btn_menu,    // chan 70 = gpio_io[2]  (MENU / OK / RESET)
    inout  wire btn_up,      // chan 71 = gpio_io[3]  (UP)
    inout  wire btn_down,    // chan 72 = gpio_io[4]  (DOWN)
    inout  wire btn_back     // chan 73 = gpio_io[5]  (BACK)
);

    Gowin_PicoRV32_Top mcu_inst (
        .clk_in             (sys_clk),
        .resetn_in          (sys_rst_n),

        // I2C
        .wbi2c_scl          (I2C_SCL),
        .wbi2c_sda          (I2C_SDA),

        // SPI -> TFT (khong co MISO)
        .wbspi_master_miso  (1'b0),
        .wbspi_master_mosi  (tft_mosi),
        .wbspi_master_ssn   (tft_cs),
        .wbspi_master_sclk  (tft_sclk),

        // GPIO 6 bit: [0]=DC, [1]=RES, [2]=MENU, [3]=UP, [4]=DOWN, [5]=BACK
        .gpio_io            ({btn_back, btn_down, btn_up, btn_menu, tft_res, tft_dc}),

        // JTAG khong dung
        .jtag_TDI           (1'b0),
        .jtag_TDO           (),
        .jtag_TCK           (1'b0),
        .jtag_TMS           (1'b0)
    );

endmodule