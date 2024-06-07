#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "driver/gpio.h"
#include "esp_log.h"

//
//    FILE: INA226_demo.ino
//  AUTHOR: Rob Tillaart
// PURPOSE: demo
//     URL: https://github.com/RobTillaart/INA226


#include "INA226.h"

#define CTS_GPIO	GPIO_NUM_15 // s3
#define RTS_GPIO	GPIO_NUM_16 // s3
INA226 INA(0x40);

static const char *TAG = "main";

void setup()
{
//  Serial0.begin(115200);
//  Serial0.println(__FILE__);
//  Serial0.print("INA226_LIB_VERSION: ");
//  Serial0.println(INA226_LIB_VERSION);

  Wire.begin();
  if (!INA.begin() )
  {
	ESP_LOGE(TAG, "could not connect. Fix and Reboot\n\n");
   // Serial.println("could not connect. Fix and Reboot");
  }
  INA.setMaxCurrentShunt(0.82, 0.1,true);
}


void loop()
{
  //Serial.println("\nBUS\tSHUNT\tCURRENT\tPOWER");
	ESP_LOGE(TAG,"\nBUS\tSHUNT\tCURRENT\tPOWER");
  for (int i = 0; i < 20; i++)
  {
	  printf("bus voltage: %3.2f \t", INA.getBusVoltage());
	  printf("shunt voltage: %3.2f\t", INA.getShuntVoltage_mV());
	  printf("current: %3.0f\t", INA.getCurrent_mA());
	  printf("power: %3.0f\n\r", INA.getPower_mW());
    delay(1000);
  }
}

//  -- END OF FILE --

extern "C" void app_main(void)
{
	setup();
	 printf("Hello from app_main!\n");

	while (true) {
      //  printf("Hello from app_main!\n");
    	loop();
        sleep(1);
    }
}
