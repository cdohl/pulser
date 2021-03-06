/* Digital Delay Generator

Pulser: the hardware that generates the pulse with delay and width 
Output: wire, defined in pulser.pcf as ppo[N_out-1:0]

Multiplexing: Each of the <N_out> outputs can be driven by any of the <N> pulsers

Clk is 100MHz or 10ns time resolution

Claus-Dieter Ohl (2020)
	//output [7:0] ppo,

*/

module pulser(input clk, 
	input rst, 
	input B1, input B2, input TRIG_IN,
	output [7:0] ppo,
	output [3:0] leds,
	output SPI_MISO,
	input SPI_MOSI, SPI_SCK, SPI_SS);

	parameter N  = 8;          //Number of pulsers
	parameter N_out = 8;       //Number of outputs
	parameter Version = 0;
        parameter Version_low = 6; // increases with changes in the cmd set or ports

	reg [31:0] DELAY [0:N-1];
	reg [31:0] WIDTH [0:N-1];
	reg [N-1:0] ENABLE = N-1'd0; 
	reg [7:0] CMD [0:5];     // 6 bytes for commands
	reg [7:0] RET [0:6];     // 7 bytes for reply incl. one safety	
	reg [N-1:0] mux [0:N_out-1];  // multiplexer for N_out channels 
	reg [31:0] temp = 32'd0;
	reg [2:0] CMD_counter = 3'b000;  //from 0 to 5 
	reg soft_trigger = 1'b0;
	reg CMD_received = 1'b0;

	integer j;
	initial begin
		for (j=0; j<N; j = j+1) begin
			DELAY[j] = 32'd1000;  //start with 10us delay und width
			WIDTH[j] = 32'd1000;
		end
		for (j=0; j<6; j = j+1) begin
			CMD[j] = 8'b0;
			RET[j] = 8'b0;
		end
		for (j=0; j<N_out; j++) begin
			mux[j] = 0;
			mux[j][j] = 1'b1;
		end

		RET[6] = 8'b0;
	end

	wire [7:0] spi_txdata, spi_rxdata;
	wire spi_rxready;

	wire pulser_busy;
	wire [N-1:0] pulse_out;
	wire [N-1:0] pulse_busy;
	wire trigger;

	wire b1on;
	reg b1on_delay;
	wire b2on;	
	reg b2on_delay;

	reg TRIG_IN_delay;

	genvar i;
	generate
		for (i=0; i<N; i = i +1) begin
			//instantiate N pulsers
			pulse_counter pulser(.clk(clk), .delay(DELAY[i]), .width(WIDTH[i]),
					     .trigger_in(trigger), .running(pulse_busy[i]),
					     .pulse_out(pulse_out[i]));
		// set the multiplexed wire outputs  
		end
		for (i=0; i<N_out; i = i + 1) begin
			assign ppo[i] = |(pulse_out & ENABLE & mux[i]);
		end
	endgenerate

        //debounced buttons
	debounce dbon1(.clk(clk), .button(B1),.state(b1on));
	debounce dbon2(.clk(clk), .button(B2),.state(b2on));


	//spislave to receive commands from STM32
	spislave SPI (.clk(clk), .txdata(spi_txdata), .rxdata(spi_rxdata), .rxready(spi_rxready),
	              .mosi(SPI_MOSI), .miso(SPI_MISO), .sck(SPI_SCK), .ss(SPI_SS));
		
	assign leds[2] = pulser_busy; //busy lamp
	assign leds[3] = CMD_received;
	
	assign pulser_busy = |(pulse_busy & ENABLE); //true if any enabled pulser is running
	//detect risiging edge for trigger and make sure it is not running
	// currently both buttons and an external trigger input with rising edge on TRIG_IN
	assign trigger = ((b1on & ~b1on_delay) || soft_trigger || (b2on & ~b2on_delay) || (TRIG_IN & ~TRIG_IN_delay))  && ~pulser_busy; 

	//1 clk cycle delayed to generate rising edge trigger
	always @(posedge clk) begin
		b1on_delay <= b1on;
		b2on_delay <= b2on;
		TRIG_IN_delay <= TRIG_IN;
	end

	assign spi_txdata = RET[CMD_counter];	

	always @(posedge clk) begin

		soft_trigger <= 1'b0;

		//Save commands into buffer
		if (spi_rxready) begin
			if (CMD_counter == 0)
				CMD_received <= 0;
			CMD[CMD_counter] <= spi_rxdata; 
			CMD_counter <= CMD_counter + 1;
		end else if (SPI_SS)
			CMD_counter <= 0;
		if (CMD_counter == 6) begin
			CMD_received <= 1;
			CMD_counter <= 0;
		end

		//Process commands
                if (CMD_received) begin
                        case(CMD[0][7:4]) // CMD[0][7:4] are the 4 command bits for maximum 

                                4'b0001: //set pulse width
				begin
					WIDTH[CMD[1]]  <= {CMD[2], CMD[3], CMD[4], CMD[5]};// Bytes 2-5, 32 bits
				end

				4'b1001: //req pulse width
				begin
					RET[0] <= 8'b00010000; //request is width
					RET[1] <= CMD[1];      //of channel
					RET[2] <= WIDTH[CMD[1]][31:24];
					RET[3] <= WIDTH[CMD[1]][23:16];
					RET[4] <= WIDTH[CMD[1]][15:8];
					RET[5] <= WIDTH[CMD[1]][7:0];
				end

                                4'b0010: //set pulse delay
				begin
					DELAY[CMD[1]]  <= {CMD[2], CMD[3], CMD[4], CMD[5]};// Bytes 2-5, 32 bits
				end 

				4'b1010: //req pulse delay
                                begin
                                        RET[0] <= 8'b10100000; //request is delay
                                        RET[1] <= CMD[1];      //of channel
                                        RET[2] <= DELAY[CMD[1]][31:24];
                                        RET[3] <= DELAY[CMD[1]][23:16];
                                        RET[4] <= DELAY[CMD[1]][15:8];
                                        RET[5] <= DELAY[CMD[1]][7:0];
                                end

				4'b1011: //req id
                                begin
                                        RET[0] <= 8'b10110000; //request is id
                                        RET[1] <= 1'b0;        //
                                        RET[2] <= N;           //Number of pulsers   
                                        RET[3] <= N_out;       //Number of outputs
                                        RET[4] <= Version;     //Version.Version_low
                                        RET[5] <= Version_low; // 
                                end

                                4'b0100: //enable channels
                                begin
					ENABLE <= {CMD[2], CMD[3], CMD[4], CMD[5]};
                                end

                                4'b1100: //req on-off channels
                                begin
                                        RET[0] <= 8'b11000000; //request is on/off
                                        RET[1] <= 8'd0;        //
					temp = {32-N'b0,ENABLE};
                                        RET[2] <= temp[31:24];
                                        RET[3] <= temp[23:16];
                                        RET[4] <= temp[15:8];
                                        RET[5] <= temp[7:0];
                                end

				4'b0101: //set mux
                                begin
					if (CMD[1]<N_out) begin //check if pulser addressed is within bounds
						temp <= {CMD[2], CMD[3], CMD[4], CMD[5]};
						mux[CMD[1]] <= temp[N-1:0]; //only set the bits of the pulsers that are available
					end
                                end

				4'b1101: //req mux
                                begin
                                        RET[0] <= 8'b11010000; //request is mux
                                        RET[1] <= CMD[1];      //of pulser
					if (CMD[1]<N_out) begin 
						temp <= {32-N_out'b0, mux[CMD[1]]};
		                                RET[2] <= temp[31:24];	
			                        RET[3] <= temp[23:16];
				                RET[4] <= temp[15:8];
					        RET[5] <= temp[7:0];
					end
                                end


				4'b1110: //req busy
                                begin
                                        RET[0] <= 8'b11100000; //request is busy
                                        RET[1] <= pulser_busy;    
                                        RET[2] <= pulser_busy;
                                        RET[3] <= pulser_busy;
                                        RET[4] <= pulser_busy;
                                        RET[5] <= pulser_busy;
                                end

				4'b1111: //trigger
				begin
					soft_trigger <= 1;
				end 
	
                                default:	CMD_received <= 0;
                        endcase
                        CMD_received <= 0;
                end
	end

endmodule
