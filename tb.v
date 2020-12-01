`timescale 1ns/100ps

module tb;

	initial begin
		$dumpfile("adder.vcd");
		$dumpvars(0, UUT);
	end

	reg [3:0] dips;
	wire [3:0] leds;

	localparam period = 20;

	adder UUT (.dips(dips), .leds(leds));

	initial begin
			dips = 4'b1111;
			#period; // wait for a period


                        dips = 4'b1100;
                        #period; // wait for a period
			
	end
endmodule
