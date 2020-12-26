###############################################################################
#                                                                             #
# Copyright 2016 myStorm Copyright and related                                #
# rights are licensed under the Solderpad Hardware License, Version 0.51      #
# (the “License”); you may not use this file except in compliance with        #
# the License. You may obtain a copy of the License at                        #
# http://solderpad.org/licenses/SHL-0.51. Unless required by applicable       #
# law or agreed to in writing, software, hardware and materials               #
# distributed under this License is distributed on an “AS IS” BASIS,          #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or             #
# implied. See the License for the specific language governing                #
# permissions and limitations under the License.                              #
#                                                                             #
###############################################################################

chip.bin: pulser.v debounce.v pulse_counter.v spislave.v pulser.pcf
	yosys -q -p "synth_ice40 -blif chip.blif" pulser.v debounce.v pulse_counter.v spislave.v
	arachne-pnr -d 8k -P tq144:4k -p pulser.pcf chip.blif -o chip.txt
	icepack chip.txt chip.bin

.PHONY: upload
upload:
	stty -F /dev/ttyACM0 raw && cat chip.bin >/dev/ttyACM0

flash: 
	dfu-util -d 0483:df11 -t 4096 --alt 0 --dfuse-address 0x0801F000 -D chip.bin

bitstream: chip.bin
	od -An -t x1 -v $^ | sed 's/[^ ][^ ]/0x&,/g;s/0x00,/0,/g;s/ //g' > arduino/myspi/ice40bitstream.h 

iceboot:
	sudo dfu-util -d 0483:df11 -s 0x08000000 -D ../../../iceboot.raw --alt 0

icetime:
	icetime -P tq144:4k -d hx8k chip.txt


.PHONY: clean
clean:
	$(RM) -f chip.blif chip.txt chip.ex chip.bin

tb: tb.v pulser.v debounce.v
	iverilog -o tb tb.v pulser.v debounce.v pulse_counter.v spislave.v

sim: tb
	./tb


