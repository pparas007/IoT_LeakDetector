#include <zephyr.h>
#include <board.h>
#include <device.h>
#include <gpio.h>
#include "valve.h" 
#include "lights.h"

#define RELAY_PORT 	"GPIO_0"
#define RELAY_PIN		31

struct device *relay_dev;

void closeValve()
{
    putLights(LED_VLV, true);
    gpio_pin_write(relay_dev, RELAY_PIN, 0);
}
void openValve()
{      
    putLights(LED_VLV, false);
    gpio_pin_write(relay_dev, RELAY_PIN, 1);
}

/*bool getValve()
{
    u32_t value;

    gpio_pin_read(led_dev, led_arr[ledno], &value);

    return value ? false : true;
}*/

void init_valve()
{
    relay_dev = device_get_binding(RELAY_PORT);
    gpio_pin_configure(relay_dev, RELAY_PIN, GPIO_DIR_OUT);
}
