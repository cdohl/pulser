# Digital delay generator

Using Blackice II FPGA board (Lattice Ice40 HX4K)

* Control through SPI from STM32 and serial interface from the outside world

* Arduino sketch accepts serial port commands such as
  ```set width <WIDTH> <PULSER NUMBER>```
  
  ```set delay <DELAY> <PULSER NUMBER>```
  
  ```set enable <PULSER BITS>```
  
  ```req enable```
  
  ```trig```

* Currently 8 output channels and 8 pulsers, pulsers can be multiplexed with on any output channel

* The pulsers are generated (```N``` in the ```pulser.v```) their number is only limited by chip size) 

* Number of outputs ```N_out``` can be adjusted, only limited by number of FPGA pins 
