batterytester
====================

This tester is made for testing 4 penlite NI-MH or Nicad batteries.
The hardware or the tester is build into an old Conrad charge manager housing. This has 4 baterry positons, a 4x16 line LCD and one switch.
The tester uses a ESP32-S3 module, but an other ESP board is also usable.
The 4 independent positions can charge and decharge the batteries with constant current. 
The hardware is a simple circuit containing an INA226 16-bit digital current and voltage sensor, a charge and decharge FET.
Each FET is used in the linear domain. The gate voltage is generated with a kind of PWM signal from the ESP , an RC filter and x3 opAmp.
A very simple algoritm in a short high-prio task keeps the current stable enough. The type of FET is not critical. I used a 400V 10A type. 
The source resistors makes the circuit short-circuit proof.  
Maximum current: 700mA. 
The circuit is build on a experiment board. The INA226 boards comes from Aliexpress. The have a 100mE shunt.
The LCD has a hd44780 driver. It is a 5V type, but the logic levels of the ESP are just enough to work fine. 

End-of-charging detection: In some literature you can find that a small voltage dip (10-15mV) is present when the battery is nearly full.
To measure this small dip the voltage is measured currentless. Else the not-so very stable contact-resistance causes too high deviations. 
It turns out that on most battries the dip is not present. The only way to properly charge is to first decharge the cell,and then charge timed.
16 hours at 0.1C is mostly used. I choosed to make this 7 hours at 0.2C. Also decharging is done at 0.2C.

Manual:
Insert a battery. Choose with the key the nearest capacity. At first the cell is decharged ("O" on the lcd) ,then charged ("L") and again decarged.
When the cell is decargerd to 0.9V the measurenment is ready "T" whith the measuer mAh. The battery is chagerged again. When full (" Vol").

Also a website is provided to monitor the process.
To connect with your network use ESP touch.

Also the INA's can be calibrated for current on the website . 

The software is made with IDF 5.3.   



    
   

