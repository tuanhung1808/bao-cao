module top (
    input  wire sys_clk,
    input  wire sys_rst_n,

    // --- I2C CHUNG (DS3231 & BH1750) ---
    inout  wire I2C_SCL,
    inout  wire I2C_SDA,

    // --- UART DEBUG ---
    output wire uart_tx,
    input  wire uart_rx,

    // --- SPI TFT ST7789 ---
    output wire tft_sclk,
    output wire tft_mosi,
    output wire tft_cs,
    inout  wire tft_dc,
    inout  wire tft_res
);

    Gowin_PicoRV32_Top mcu_inst (
        .clk_in             (sys_clk),
        .resetn_in          (sys_rst_n),

        // UART
        .wbuart_tx          (uart_tx),
        .wbuart_rx          (uart_rx),

        // I2C
        .wbi2c_scl          (I2C_SCL),
        .wbi2c_sda          (I2C_SDA),

        // SPI TFT
        .wbspi_master_miso  (1'b0),
        .wbspi_master_mosi  (tft_mosi),
        .wbspi_master_ssn   (tft_cs),
        .wbspi_master_sclk  (tft_sclk),
        .gpio_io            ({tft_res, tft_dc}), // bit1=RES, bit0=DC

        // JTAG (Tắt)
        .jtag_TDI           (1'b0),
        .jtag_TDO           (),
        .jtag_TCK           (1'b0),
        .jtag_TMS           (1'b0)
    );

endmodule