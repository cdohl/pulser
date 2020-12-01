module pulse_counter (input clk,
	input [31:0] delay,
	input [31:0] width,
	input trigger_in,
	output running,
	output pulse_out);

        
        parameter IDLE = 2'b00, TRIGGERED = 2'b01, OUTPUT = 2'b10;
        reg [31:0] count = 0;
        reg [1:0] state = IDLE;
        reg p_out = 0;
        reg r_out = 0;

        assign pulse_out = p_out; 
        assign running = r_out; 

        always @(posedge clk)
        begin
                count <= count +1;
                case(state)
                        IDLE:
                        begin
                                if (trigger_in)
                                begin
                                        state <= TRIGGERED;
                                        count <= 0;
					r_out <= 1;
                                end
                        end
                        TRIGGERED:
                        begin
                                if (count == delay)
                                begin
                                        state <= OUTPUT;
                                        count <= 0;
                                        p_out <= 1;
                                end
                        end
                        OUTPUT:
                        begin
                                if (count == width)
                                begin
                                        state <= IDLE;
                                        p_out <= 0;
                                        r_out <= 0;
                                end
                        end
                endcase
        end


endmodule


