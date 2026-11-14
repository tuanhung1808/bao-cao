// ============================================================================
// sensors_wb.v
// Wrapper Wishbone cho BH1750 + DS3231, cam thang vao "OPEN WB INTERFACE"
// cua Gowin_PicoRV32 (dung dung quy uoc tin hieu trong wbreg.v mau cua Gowin).
//
// Ban do thanh ghi (offset tinh tu OPEN_WB_BASE = 0x20000000):
//   0x00  BH1750_STATUS  (doc)      bit0 = 1 khi co lux MOI chua doc
//   0x04  BH1750_DATA    (doc)      [15:0] = so dem tho (lux = DATA/1.2)
//                                    doc thanh ghi nay se tu xoa STATUS.bit0
//   0x10  DS3231_STATUS  (doc)      bit0 = 1 khi co gio MOI chua doc
//   0x14  DS3231_TIME    (doc)      [7:0]=giay [15:8]=phut [23:16]=gio
//   0x18  DS3231_DATE    (doc)      [7:0]=nam [15:8]=thang [23:16]=ngay [26:24]=thu
//                                    doc thanh ghi nay se tu xoa STATUS.bit0
// ============================================================================
module sensors_wb #(
    parameter ARST_LVL           = 1'b0,
    parameter FREQ_MHZ           = 27,
    parameter BH1750_ADDR_HIGH   = 0,
    parameter READ_PERIOD_TICKS  = 100000   // ~1 giay
)(
    // tin hieu Wishbone - dung dung ten nhu wbreg.v mau cua Gowin
    wb_clk_i,
    wb_rst_i,
    arst_i,
    wb_adr_i,
    wb_dat_i,
    wb_dat_o,
    wb_we_i,
    wb_stb_i,
    wb_cyc_i,
    wb_ack_o,

    // chan vat ly I2C - 2 bus rieng
    SCL_bh,  SDA_bh,
    SCL_rtc, SDA_rtc
);

    input         wb_clk_i;
    input         wb_rst_i;
    input         arst_i;
    input  [4:0]  wb_adr_i;
    input  [31:0] wb_dat_i;
    output [31:0] wb_dat_o;
    input         wb_we_i;
    input         wb_stb_i;
    input         wb_cyc_i;
    output        wb_ack_o;

    inout SCL_bh, SDA_bh, SCL_rtc, SDA_rtc;

    reg [31:0] wb_dat_o;
    reg        wb_ack_o;

    // reset noi bo: giong wbreg.v (XOR voi ARST_LVL), cong them wb_rst_i cho chac
    wire rst_hi  = (arst_i ^ ARST_LVL) | wb_rst_i;  // 1 = dang reset
    wire resetn  = ~rst_hi;                          // driver cua minh dung active-low

    // access_now = dung luc Wishbone dang phuc vu yeu cau nay (cung bieu thuc
    // quyet dinh wb_ack_o o chu ky ke tiep) - dung de biet CHINH XAC luc nao
    // 1 lenh doc/ghi thuc su "trung dich" thanh ghi cua minh.
    wire access_now = wb_cyc_i & wb_stb_i & ~wb_ack_o;

    always @(posedge wb_clk_i)
        wb_ack_o <= access_now;

    // ======================================================================
    // BH1750
    // ======================================================================
    wire [15:0] lux_data;
    wire        lux_valid, lux_busy;

    BH1750 #(
        .Freq_MegaHZ       (FREQ_MHZ),
        .ADDR_SEL_HIGH     (BH1750_ADDR_HIGH),
        .READ_PERIOD_TICKS (READ_PERIOD_TICKS)
    ) u_bh1750 (
        .sys_clk(wb_clk_i), ._rst(resetn), .str(1'b1),
        .SCL(SCL_bh), .SDA(SDA_bh),
        .data(lux_data), .data_valid(lux_valid), .busy(lux_busy)
    );

    reg [2:0] bh_vsync;
    always @(posedge wb_clk_i or negedge resetn)
        if (!resetn) bh_vsync <= 3'b000;
        else         bh_vsync <= {bh_vsync[1:0], lux_valid};
    wire bh_v_rise = bh_vsync[1] & ~bh_vsync[2];

    reg [15:0] lux_reg;
    reg        lux_new;
    wire rd_bh_data = access_now & ~wb_we_i & (wb_adr_i == 5'h04);

    always @(posedge wb_clk_i or negedge resetn) begin
        if (!resetn) begin
            lux_reg <= 16'd0;
            lux_new <= 1'b0;
        end else begin
            if (bh_v_rise)       begin lux_reg <= lux_data; lux_new <= 1'b1; end
            else if (rd_bh_data) lux_new <= 1'b0;
        end
    end

    // ======================================================================
    // DS3231
    // ======================================================================
    wire [5:0] sec, mins;
    wire [4:0] hour, date;
    wire [2:0] dow;
    wire [3:0] month;
    wire [7:0] year;
    wire       rtc_valid, rtc_busy;

    DS3231 #(
        .Freq_MegaHZ       (FREQ_MHZ),
        .READ_PERIOD_TICKS (READ_PERIOD_TICKS)
    ) u_ds3231 (
        .sys_clk(wb_clk_i), ._rst(resetn), .str(1'b1),
        .SCL(SCL_rtc), .SDA(SDA_rtc),
        .sec(sec), .mins(mins), .hour(hour), .dow(dow),
        .date(date), .month(month), .year(year),
        .data_valid(rtc_valid), .busy(rtc_busy)
    );

    reg [2:0] rtc_vsync;
    always @(posedge wb_clk_i or negedge resetn)
        if (!resetn) rtc_vsync <= 3'b000;
        else         rtc_vsync <= {rtc_vsync[1:0], rtc_valid};
    wire rtc_v_rise = rtc_vsync[1] & ~rtc_vsync[2];

    reg [5:0] sec_r, min_r;
    reg [4:0] hour_r, date_r;
    reg [2:0] dow_r;
    reg [3:0] month_r;
    reg [7:0] year_r;
    reg       rtc_new;
    wire rd_rtc_date = access_now & ~wb_we_i & (wb_adr_i == 5'h18);

    always @(posedge wb_clk_i or negedge resetn) begin
        if (!resetn) begin
            sec_r<=0; min_r<=0; hour_r<=0; date_r<=0; dow_r<=0; month_r<=0; year_r<=0;
            rtc_new <= 1'b0;
        end else begin
            if (rtc_v_rise) begin
                sec_r<=sec; min_r<=mins; hour_r<=hour;
                date_r<=date; dow_r<=dow; month_r<=month; year_r<=year;
                rtc_new <= 1'b1;
            end else if (rd_rtc_date) begin
                rtc_new <= 1'b0;
            end
        end
    end

    // ======================================================================
    // giai ma dia chi -> wb_dat_o (giong het kieu wbreg.v mau)
    // ======================================================================
    always @(posedge wb_clk_i) begin
        case (wb_adr_i) // synopsys parallel_case
            5'h00:   wb_dat_o <= {31'd0, lux_new};
            5'h04:   wb_dat_o <= {16'd0, lux_reg};
            5'h10:   wb_dat_o <= {31'd0, rtc_new};
            5'h14:   wb_dat_o <= {8'd0, 3'd0, hour_r, 2'd0, min_r, 2'd0, sec_r};
            5'h18:   wb_dat_o <= {5'd0, dow_r, 3'd0, date_r, 4'd0, month_r, year_r};
            default: wb_dat_o <= 32'd0;
        endcase
    end

endmodule
