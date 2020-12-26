module pulse_counter (input clk,
	input [31:0] delay,
	input [31:0] width,
	input trigger_in,
	output running,
	output pulse_out);

        parameter IDLE = 2'b00, TRIGGERED = 2'b01, OUTPUT = 2'b10;

        reg [31:0] count = 0;
        reg [1:0] state = IDLE;
        reg running;
        reg pulse_out;

        always @(posedge clk)
        begin
                case(state)
                        IDLE:
                        begin
				if (trigger_in)
                                begin
                                        state <= TRIGGERED;
                                        count <= 0;
					running <= 1;
                                end else
				begin
					count <= count + 1;
					running <= 0;
				end
                        end
                        TRIGGERED:
                        begin
                                if (count == delay)
                                begin
                                        state <= OUTPUT;
                                        count <= 0;
                                        pulse_out <= 1;
                                end else
					count <= count + 1;
                        end
                        OUTPUT:
                        begin
                                if (count == width)
                                begin
                                        state <= IDLE;
                                        pulse_out <= 0;
                                        running <= 0;
					count <= 0;
                                end else
					count <= count + 1;
				
                        end
                endcase
        end

endmodule


