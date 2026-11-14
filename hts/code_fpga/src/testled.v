/*
POWER OFF 0x00
POWER ON  0x01
Reset reg 0x07
Continuously H Mode  0x10  120mS
Continuously H Mode2 0x11  120mS
Continuously L Mode  0x13  16mS
1's H Mode  0x20
1's H Mode2 0x21
1's L Mode  0x23

============================================================================
SUA LOI so voi ban goc (xem giai thich chi tiet trong hoi thoai):
  1) Dia chi I2C: ban goc luon dung ADDR_H (0x5C, chi dung khi chan ADD noi
     VCC). Da doi mac dinh sang ADDR_L (0x23, dung khi ADD noi GND/tha noi -
     truong hop pho bien nhat cua module BH1750 roi). Neu board cua ban noi
     ADD len VCC, dat tham so ADDR_SEL_HIGH = 1 khi instantiate.
  2) Logic chon lenh gui (Txdata) truoc day dung bo dem IR chay vong qua 4
     lenh (Power Down/Power On/Reset/Do-1-lan) troi tren nhieu chu ky doc,
     khien 3/4 lan doc ra du lieu cu/khong hop le. Da thay bang trinh tu:
     gui MOT LAN DUY NHAT luc khoi dong (Power On -> Continuous H-Resolution
     Mode), sau do CHI DOC LAP LAI - dung theo dac tinh "che do do lien tuc"
     cua cam bien (tu cap nhat thanh ghi moi ~120ms).
  3) Ngo ra du lieu: ban goc "assign data = Rxdata" tro thang vao thanh ghi
     dang bi ghi tung bit trong luc nhan -> mach ngoai co the doc phai gia
     tri "nua cu nua moi". Da them thanh ghi data_reg + xung data_valid,
     chi cap nhat data_reg MOT LAN duy nhat khi da nhan du 16-bit.
============================================================================
*/

module BH1750#(
    parameter Freq_MegaHZ      = 27,     // tan so sys_clk thuc te (MHz) - PHAI khop voi clock cap cho module
    parameter ADDR_SEL_HIGH    = 0,      // 0 = dung dia chi 0x23 (ADD noi GND/tha noi, mac dinh)
                                          // 1 = dung dia chi 0x5C (ADD noi VCC)
    parameter READ_PERIOD_TICKS = 100000  // so tick clk_iic (~100kHz) giua 2 lan doc lien tiep 
)(
    sys_clk, _rst, str, SCL, SDA, data, data_valid, busy
);
input  sys_clk, _rst, str;
inout  SCL, SDA;
output [15:0] data;
output        data_valid;  // xung 1 chu ky clk_iic khi vua co 1 gia tri lux moi, hop le
output        busy;

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
parameter ADDR_H  = 7'b1011100; // 0x5C - chan ADD noi VCC
parameter ADDR_L  = 7'b0100011; // 0x23 - chan ADD noi GND / tha noi (mac dinh)
wire [6:0] I2C_ADDR = ADDR_SEL_HIGH ? ADDR_H : ADDR_L;
//---------------------------------------------------//
reg scl_iic = 1'b1, sda_iic = 1'b1;
reg scl_gate = 1'b1, sda_gate = 1'b1;
reg clk_iic = 1'b0;
assign SCL = (!scl_gate)?1'bz:scl_iic;//I/O
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
				if(cnt==Freq_MegaHZ*5)begin// 
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
// Trinh tu dieu khien o muc cao: gui 2 lenh khoi tao DUY NHAT MOT LAN
// (Power On -> Continuous H-Resolution Mode), sau do CHI DOC lap lai theo
// chu ky READ_PERIOD_TICKS - khong gui lai lenh moi lan doc.
//---------------------------------------------------//
localparam PH_INIT_PWRON = 2'd0;
localparam PH_INIT_CONT  = 2'd1;
localparam PH_READ       = 2'd2;

reg [1:0]  phase      = PH_INIT_PWRON;
reg [17:0] read_timer = 'd0;
reg        kick       = 1'b0;   // xung 1 tick clk_iic: "hay bat dau 1 giao dich moi"
reg        start_req  = 1'b0;
reg        boot_done  = 1'b0;   // dam bao co DUNG 1 xung kick ngay sau reset de bat dau giao dich dau tien

wire trans_done = (state==stop) && (clk_cnt=='d0); // giao dich vua ket thuc (STOP vua hoan tat)

always@(posedge clk_iic, negedge _rst)begin
	if(!_rst)begin
		phase      <= PH_INIT_PWRON;
		read_timer <= 'd0;
		kick       <= 1'b0;
		boot_done  <= 1'b0;
	end
	else begin
		kick <= 1'b0;
		if(!boot_done)begin
			boot_done <= 1'b1;
			kick      <= 1'b1; // moi giao dich chi bat dau khi co kick; can 1 xung "moi" ngay sau reset
		end
		else begin
			case(phase)
				PH_INIT_PWRON: if(trans_done)begin phase <= PH_INIT_CONT; kick <= 1'b1; end
				PH_INIT_CONT : if(trans_done)begin phase <= PH_READ;      kick <= 1'b1; end
				PH_READ      : begin
					if(read_timer==READ_PERIOD_TICKS)begin
						read_timer <= 'd0;
						kick       <= 1'b1;
					end
					else
						read_timer <= read_timer + 1'b1;
				end
				default: phase <= PH_INIT_PWRON;
			endcase
		end
	end
end

// start_req duoc GIU MUC CAO tu luc kick den khi FSM thuc su roi khoi IDLE
// (chuyen sang Address) - dam bao du thoi gian de tao dieu kien START,
// khong phu thuoc dem so tick co dinh nhu ban goc.
always@(posedge clk_iic, negedge _rst)begin
	if(!_rst) start_req <= 1'b0;
	else begin
		if(kick)              start_req <= 1'b1;
		else if(state==Address) start_req <= 1'b0;
	end
end


assign start = start_req;
assign RW    = (phase == PH_READ) ? 1'b1 : 1'b0;

wire [7:0] Txdata = (phase==PH_INIT_PWRON) ? 8'h01 :  // Power On
                     (phase==PH_INIT_CONT ) ? 8'h10 :  // Continuously H-Resolution Mode
                     8'h00;                             // khong dung khi doc
//---------------------------------------------------//
reg [15:0]Rxdata  = 'd0;
reg [15:0]data_reg   = 'd0;
reg       data_valid_r = 1'b0;
reg [ 3:0]RxCnt   = 'hF;
reg [ 3:0]clk_cnt = 'hB;
reg [ 2:0]TxCnt   = 'd7;
reg [ 7:0]IRreg   = 'h0;
reg ack_flag      = 'b0;
assign data       = data_reg;
assign data_valid = data_valid_r;
	always@(posedge clk_iic, negedge  _rst)begin
		if(!_rst)begin
			Rxdata     <= 16'h0000;
			data_reg   <= 16'h0000;
			data_valid_r <= 1'b0;
			sda_iic    <= 1'b1;
			scl_iic    <= 1'b1;
			RxCnt      <= 4'hF;
			TxCnt      <= 3'd7;
			IRreg      <= 8'd0;
			state      <= IDLE;
		end
		else begin
			data_valid_r <= 1'b0; // xung 1 chu ky
			case(state)
				IDLE:begin
					TxCnt <= 3'd7;
					RxCnt <= 4'hF;
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
								RxCnt    <= 4'hF;
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
						RxCnt    <= RxCnt - 1'b1;
						if(RxCnt=='h8)begin
							sda_iic  <= 1'b0;
							sda_gate <= write;
							state    <= NAck_1;
						end
						else if(RxCnt=='h0)begin
							RxCnt      <= 4'hF;
							sda_gate   <= write;
							state      <= NAck_2;
							data_reg   <= Rxdata;  // du lieu 16-bit da nhan DAY DU, chot 1 lan
							data_valid_r <= 1'b1;   // bao cho mach ngoai biet co gia tri moi
						end
						else 
							state <= R_data;
					end
					else if(clk_cnt=='d3)
						scl_iic <= 1'b0;
					else if(clk_cnt=='d8)
						scl_iic <= 1'b1;
					else if(clk_cnt=='d6)
						Rxdata[RxCnt] <= SDA;
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
