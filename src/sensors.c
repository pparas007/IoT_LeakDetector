#include <zephyr.h>
#include <board.h>
#include <device.h>
#include <misc/printk.h>
#include <kernel.h>
#include "config.h"
#include "tb_pubsub.h"
#include "force_sensor.h"
#include "water_flow_sensor.h"
#include "valve.h"

void my_timer_handler(struct k_timer *dummy)
{
  k_work_submit(&tic_force_sensor);
  k_work_submit(&tic_water_sensor);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);

void sensors_start()
{
  printk("External sensors startging\n");
  init_force_sensor();
  init_water_flow_sensor();
  init_valve();
  /* start periodic timer that expires once every X period seconds */
  //https://docs.zephyrproject.org/1.9.0/kernel/timing/timers.html
  
  //k_timer_start(name, duration, period)
  k_timer_start(&my_timer, K_SECONDS(1), K_SECONDS(3));
  //k_timer_stop(&my_timer);
}