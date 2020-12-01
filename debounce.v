module debounce(input clk, input button, output reg state);

        wire idle;
        reg sync;
        reg [16:0] count;

        assign idle = (state == sync);

        always @(posedge clk) begin
                sync <= ~button;
                count <= (idle) ? 0 : count + 17'b1;
                if(&count)
                        state <= ~state;
        end

endmodule


