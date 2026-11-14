/*
DS3231 - dong ho thoi gian thuc, dia chi I2C co dinh 0x68.

Khac voi BH1750:
  - Khong co "lenh khoi tao" (Power On/Reset...) - DS3231 tu chay lien tuc.
  - Truoc moi lan doc, phai ghi 1 byte CON TRO THANH GHI = 0x00 (tro toi
    thanh ghi Giay) - VA PHAI GHI LAI MOI CHU KY doc (khac BH1750 chi ghi
    lenh dung 1 lan luc dau), vi con tro co the da bi dich chuyen boi lan
    doc truoc.
  - Doc lien tiep 7 byte: Giay, Phut, Gio, Thu, Ngay, Thang, Nam - deu o
    dang BCD (tru byte Thu la so nguyen 1-7, khong phai BCD).
  - Gia dinh thanh ghi Gio dang o CHE DO 24H (bit chon 12h/24h = 0). Neu
    module RTC cua ban dang o che do 12h, can sua lai phan giai ma gio.

Kien truc FSM giu NGUYEN cach dat ten trang thai giong BH1750.v de de doc/
de doi chieu voi waveform da quen: IDLE - Address - Ack_0 - W_data - Ack_1
- R_data - NAck_1 - NAck_2 - stop.
*/

module DS3231#(
    parameter Freq_MegaHZ       = 27,     // tan so sys_clk thuc te (MHz) - PHAI khop clock cap cho module
    parameter READ_PERIOD_TICKS = 100000   // so tick clk_iic (~100kHz) giua 2 lan doc lien tiep (~180ms)
)(
    sys_clk, _rst, str, SCL, SDA,
    sec, mins, hour, dow, date, month, year,
    data_valid, busy
);
input  sys_clk, _rst, str;
inout  SCL, SDA;
output [5:0] sec;    // 0-59  (da chuyen sang thap phan)
output [5:0] mins;   // 0-59
output [4:0] hour;   // 0-23  (gia dinh che do 24h)
output [2:0] dow;    // 1-7   (Thu, khong phai BCD)
output [4:0] date;   // 1-31
output [3:0] month;  // 1-12
output [7:0] year;   // 0-99  (cong them 2000 o phia phan mem/hien thi)
output       data_valid; // xung 1 chu ky clk_iic khi vua co 1 bo gia tri thoi gian moi, hop le
output       busy;

wire RW, start;
reg [8:0]cnt = 'd0;
reg [3:0]state = 'd0;
//---------------------------------------------------//
parameter IDLE    = 4'h0;
parameter Address = 4'h1;
parameter Ack_0   = 4'h2;
parameter W_data  = 4'h3;
parameter Ack_1   = 4'h4;
parameter NAck_1  = 4'h5;
parameter R_data  = 4'h6;
parameter NAck_2  = 4'h7;
parameter stop    = 4'h8;
parameter write   = 1'b1;
parameter read    = 1'b0;
localparam [6:0] I2C_ADDR = 7'b1101000; // 0x68, co dinh
//---------------------------------------------------//
reg scl_iic = 1'b1, sda_iic = 1'b1;
reg scl_gate = 1'b1, sda_gate = 1'b1;
reg clk_iic = 1'b0;
assign SCL = (!scl_gate)?1'bz:scl_iic;
assign SDA = (!sda_gate)?1'bz:sda_iic;
assign busy = (state != IDLE);
//---------------------------------------------------//
	always@(posedge sys_clk, negedge  _rst)begin
		if(!_rst)begin
			cnt     <= 'd0;
			clk_iic <= 1'b0;
		end
		else begin
			if(str)begin
				if(cnt==Freq_MegaHZ*5)begin
					clk_iic <= ~clk_iic;
					cnt     <= 'd0;
				end
				else
					cnt <= cnt + 1'b1;
			end
			else begin
				cnt     <= 'd0;
				clk_iic <= 1'b0;
			end
		end
	end
//---------------------------------------------------//
// Trinh tu dieu khien o muc cao: MOI CHU KY deu ghi lai con tro thanh ghi
// (0x00) roi doc 7 byte - khac BH1750 (chi ghi lenh khoi tao 1 lan).
//---------------------------------------------------//
localparam PH_WRITE_PTR = 2'd0;
localparam PH_READ      = 2'd1;
localparam PH_WAIT      = 2'd2;

reg [1:0]  phase      = PH_WRITE_PTR;
reg [17:0] read_timer = 'd0;
reg        kick       = 1'b0;
reg        start_req  = 1'b0;
reg        boot_done  = 1'b0;

wire trans_done = (state==stop) && (clk_cnt=='d0);

always@(posedge clk_iic, negedge _rst)begin
	if(!_rst)begin
		phase      <= PH_WRITE_PTR;
		read_timer <= 'd0;
		kick       <= 1'b0;
		boot_done  <= 1'b0;
	end
	else begin
		kick <= 1'b0;
		if(!boot_done)begin
			boot_done <= 1'b1;
			kick      <= 1'b1; // xung dau tien ngay sau reset, bat dau ghi con tro
		end
		else begin
			case(phase)
				PH_WRITE_PTR: if(trans_done)begin phase <= PH_READ; kick <= 1'b1; end
				PH_READ     : if(trans_done)begin phase <= PH_WAIT; read_timer <= 'd0; end
				PH_WAIT     : begin
					if(read_timer==READ_PERIOD_TICKS)begin
						read_timer <= 'd0;
						phase      <= PH_WRITE_PTR;
						kick       <= 1'b1;
					end
					else
						read_timer <= read_timer + 1'b1;
				end
				default: phase <= PH_WRITE_PTR;
			endcase
		end
	end
end

always@(posedge clk_iic, negedge _rst)begin
	if(!_rst) start_req <= 1'b0;
	else begin
		if(kick)                 start_req <= 1'b1;
		else if(state==Address)  start_req <= 1'b0;
	end
end

assign start = start_req;
assign RW    = (phase == PH_READ) ? 1'b1 : 1'b0;

wire [7:0] Txdata = 8'h00; // con tro thanh ghi: luon tro toi thanh ghi Giay (offset 0x00)
//---------------------------------------------------//
reg [7:0] Rxdata [0:6];  // 7 byte tho nhan duoc: 0=Giay,1=Phut,2=Gio,3=Thu,4=Ngay,5=Thang,6=Nam
reg [2:0] RxByte = 3'd0; // dang nhan byte thu may (0..6)
reg [2:0] RxBit  = 3'd7; // dang nhan bit thu may trong byte hien tai (7..0)
reg [2:0] TxCnt  = 3'd7;
reg [7:0] IRreg  = 8'h0;
reg ack_flag     = 1'b0;
reg [3:0]clk_cnt = 'hB;

// ---- chuyen BCD -> thap phan ----
function [7:0] bcd2dec;
	input [7:0] b;
	begin
		bcd2dec = b[7:4]*4'd10 + b[3:0];
	end
endfunction

reg [5:0] sec_r, mins_r;
reg [4:0] hour_r, date_r;
reg [2:0] dow_r;
reg [3:0] month_r;
reg [7:0] year_r;
reg       data_valid_r;

assign sec        = sec_r;
assign mins       = mins_r;
assign hour       = hour_r;
assign dow         = dow_r;
assign date        = date_r;
assign month       = month_r;
assign year         = year_r;
assign data_valid = data_valid_r;

	always@(posedge clk_iic, negedge  _rst)begin
		if(!_rst)begin
			sda_iic    <= 1'b1;
			scl_iic    <= 1'b1;
			RxByte     <= 3'd0;
			RxBit      <= 3'd7;
			TxCnt      <= 3'd7;
			IRreg      <= 8'd0;
			state      <= IDLE;
			sec_r      <= 6'd0;  mins_r  <= 6'd0;  hour_r <= 5'd0;
			dow_r      <= 3'd0;  date_r  <= 5'd0;  month_r<= 4'd0; year_r <= 8'd0;
			data_valid_r <= 1'b0;
		end
		else begin
			data_valid_r <= 1'b0; // xung 1 chu ky
			case(state)
				IDLE:begin
					TxCnt    <= 3'd7;
					RxByte   <= 3'd0;
					RxBit    <= 3'd7;
					ack_flag <= 1'b0;
					sda_gate <= write;
					scl_gate <= write;
					if(start)begin
						clk_cnt <= clk_cnt - 1'b1;
						if(clk_cnt=='d0)begin
							clk_cnt <= 'hB;
							IRreg   <= {I2C_ADDR, RW};
							state   <= Address;
						end
						else if(clk_cnt=='d1)
							scl_iic <= 1'b0;
						else if(clk_cnt=='d3)
							sda_iic <= 1'b0;
					end
					else begin
						sda_iic  <= 1'b1;
						scl_iic  <= 1'b1;
						clk_cnt  <= 4'hB;
						state    <= IDLE;
					end
				end
				Address:begin
					clk_cnt <= clk_cnt - 1'b1;
					if(clk_cnt=='d0)begin
						clk_cnt  <= 'hB;
						TxCnt    <= TxCnt - 1'b1;
						if(TxCnt=='d0)begin
							TxCnt    <= 3'd7;
							sda_gate <= read;
							state    <= Ack_0;
						end
						else
							state <= Address;
					end
					else if(clk_cnt=='d2)
						sda_iic <= 1'b0;
					else if(clk_cnt=='d3)
						scl_iic <= 1'b0;
					else if(clk_cnt=='d8)
						scl_iic <= 1'b1;
					else if(clk_cnt=='d9)
						sda_iic <= IRreg[TxCnt];
				end
				Ack_0:begin
					clk_cnt <= clk_cnt - 1'b1;
					if(clk_cnt=='d0)begin
						if(ack_flag)begin
							clk_cnt  <= 'hB;
							ack_flag <= ~ack_flag;
							if(!IRreg[0])begin
								sda_gate <= write;
								state    <= W_data;
							end
							else begin
								RxByte   <= 3'd0;
								RxBit    <= 3'd7;
								sda_gate <= read;
								state    <= R_data;
							end
						end
						else begin
							clk_cnt <= 'd2;
							state   <= Ack_0;
						end
					end
					else if(clk_cnt=='d3)
						scl_iic <= 1'b0;
					else if(clk_cnt=='d8)
						scl_iic <= 1'b1;
					else if(clk_cnt<'d8)begin
						if(!SDA)
							ack_flag <= 1'b1;
						else
							ack_flag <= ack_flag;
					end
				end
				W_data:begin
					clk_cnt <= clk_cnt - 1'b1;
					if(clk_cnt=='d0)begin
						clk_cnt  <= 'd11;
						TxCnt    <= TxCnt - 1'b1;
						if(TxCnt=='d0)begin
							TxCnt    <= 3'd7;
							sda_gate <= read;
							state    <= Ack_1;
						end
						else
							state <= W_data;
					end
					else if(clk_cnt=='d1)
						sda_iic <= 1'b0;
					else if(clk_cnt=='d3)
						scl_iic <= 1'b0;
					else if(clk_cnt=='d8)
						scl_iic <= 1'b1;
					else if(clk_cnt=='d9)
						sda_iic <= Txdata[TxCnt];
				end
				Ack_1:begin
					clk_cnt <= clk_cnt - 1'b1;
					if(clk_cnt=='d0)begin
						if(ack_flag)begin
							clk_cnt  <= 'hB;
							sda_gate <= write;
							ack_flag <= ~ack_flag;
							state <= stop;
						end
						else begin
							clk_cnt <= 'd2;
							state <= Ack_1;
						end
					end
					else if(clk_cnt=='d3)
						scl_iic <= 1'b0;
					else if(clk_cnt=='d8)
						scl_iic <= 1'b1;
					else if(clk_cnt<'d8)begin
						if(!SDA)
							ack_flag <= 1'b1;
						else
							ack_flag <= ack_flag;
					end
				end
				R_data:begin
					clk_cnt  <= clk_cnt - 1'b1;
					if(clk_cnt=='d0)begin
						clk_cnt  <= 'hB;
						if(RxBit=='d0)begin
							// vua nhan xong bit cuoi (LSB) cua byte hien tai
							if(RxByte=='d6)begin
								// day la byte thu 7 (Nam) - byte CUOI CUNG
								RxByte       <= 3'd0;
								sda_gate     <= write;
								state        <= NAck_2; // phat NACK, bao dung doc
								// chot toan bo 7 byte, giai ma BCD -> thap phan
								sec_r        <= bcd2dec({1'b0, Rxdata[0][6:0]});
								mins_r       <= bcd2dec({1'b0, Rxdata[1][6:0]});
								hour_r       <= bcd2dec({2'b00, Rxdata[2][5:0]}); // gia dinh che do 24h
								dow_r        <= Rxdata[3][2:0]; // khong phai BCD
								date_r       <= bcd2dec({2'b00, Rxdata[4][5:0]});
								month_r      <= bcd2dec({3'b000, Rxdata[5][4:0]});
								year_r       <= bcd2dec(Rxdata[6]);
								data_valid_r <= 1'b1;
							end
							else begin
								// con byte tiep theo - phat ACK de doc tiep
								RxByte   <= RxByte + 1'b1;
								RxBit    <= 3'd7;
								sda_iic  <= 1'b0;
								sda_gate <= write;
								state    <= NAck_1;
							end
						end
						else begin
							RxBit <= RxBit - 1'b1;
							state <= R_data;
						end
					end
					else if(clk_cnt=='d3)
						scl_iic <= 1'b0;
					else if(clk_cnt=='d8)
						scl_iic <= 1'b1;
					else if(clk_cnt=='d6)
						Rxdata[RxByte][RxBit] <= SDA;
				end
				NAck_1:begin
					clk_cnt <= clk_cnt - 1'b1;
					sda_iic <= 1'b0;
					if(clk_cnt=='d0)begin
						clk_cnt  <= 'hB;
						sda_gate <= read;
						state    <= R_data;
					end
					else if(clk_cnt=='d3)
						scl_iic <= 1'b0;
					else if(clk_cnt=='d8)
						scl_iic <= 1'b1;
				end
				NAck_2:begin
					clk_cnt <= clk_cnt - 1'b1;
					if(clk_cnt>1)
						sda_iic <= 1'b1;
					else
						sda_iic <= 1'b0;

					if(clk_cnt=='d0)begin
						clk_cnt  <= 'hB;
						sda_gate <= write;
						state    <= stop;
					end
					else if(clk_cnt=='d3)
						scl_iic <= 1'b0;
					else if(clk_cnt=='d8)
						scl_iic <= 1'b1;
				end
				stop:begin
					clk_cnt <= clk_cnt - 1'b1;
					if(clk_cnt=='d0)begin
						clk_cnt <= 'hB;
						state   <= IDLE;
					end
					else if(clk_cnt=='d6)
						sda_iic <= 1'b1;
					else if(clk_cnt=='d8)
						scl_iic <= 1'b1;
				end
			endcase
		end
	end
//---------------------------------------------------//
endmodule
