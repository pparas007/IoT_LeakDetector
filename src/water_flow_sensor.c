#include <zephyr.h>
#include <board.h>
#include <device.h>
#include <kernel.h>
#include <misc/printk.h>
#include <gpio.h>
#include "config.h"
#include "tb_pubsub.h"
#include "lights.h"

#define WFS_PORT  "GPIO_0"
#define WFS_PIN   11

// Thresholds:
int FLUSHING_THRESHOLD = 5;
int SLOW_HIGH_THRESHOLD = 30;

#define NO_FLOW  "NO_FLOW"
#define FLUSHING_SLOW  "SLOW"
#define FLUSHING_FAST  "FAST"

#define STATES_ARRAY_SIZE  3

u8_t current_state; // numerical current state.
u8_t previous_reported_state = 4; // Previous state of the water_flow_sensor.
char *current_state_label; // Current state of the water_flow_sensor.
u32_t current_flow; // Exact flow number.

/**
  * Array used to record states during 3 consecutive detecting interval.
  * NO_FLOW is represented by 0
  * FLUSHING_SLOW is represented by 1
  * FLUSHING_FAST is be represented by 2
  */
u8_t previous_states[STATES_ARRAY_SIZE];

struct device *wfs_dev;

// Expiry function of timer.
void my_wfs_expiry_fn(struct k_work *work)
{
  u32_t cur_val = 1;
  u32_t last_val = 0;
  u32_t cnt_flow = 0;

  u32_t start_time;
  u32_t stop_time;
  u32_t cycles_spent;
  u32_t nanoseconds_spent;
  // Set the length of the detecting interval to 1.5S.
  u32_t time_interval = 1500000000;// (nanoseconds) = 1.5s.

  // Capture initial time stamp.
  start_time = k_cycle_get_32();

  printk("Detecting water flow...\n");
  while (1) {
    gpio_pin_read(wfs_dev, WFS_PIN, &cur_val);
    // Each square wave increases cur_flow by 2.
    if (cur_val != last_val) {
      cnt_flow++;
      printk("Detected\n");
    }
    last_val = cur_val;

    // Capture final time stamp.
    stop_time = k_cycle_get_32();

    // Compute how long the work took (assumes no counter rollover).
    cycles_spent = stop_time - start_time;
    nanoseconds_spent = SYS_CLOCK_HW_CYCLES_TO_NS(cycles_spent);
    //printk("Current spend time: %d.\n", nanoseconds_spent);
    // Break when spent over 2S.
    if(nanoseconds_spent >= time_interval) break;
  }
  /**
    * Water-flow Formula: 1L = 5880 square waves.
    * So 1000ml ~= 6000 square waves = 12000 cnt_flow.
    * Then 1/12 ml pre cnt_flow.
    * Flow rate: fr ~= 1/12ml/cnt_flow/1.5s = cnt_flow/18 ml/s.
    */
  current_flow = cnt_flow / 18;
  //printf("Current water flow: %d ml/s.\n", cnt_flow);
  printf("Current water flow: %d ml/s.\n", current_flow);
  update_states_array();
}

// Update the states array.
void update_states_array() {
  if (current_flow >= SLOW_HIGH_THRESHOLD) {
    current_state = 2;
    current_state_label = FLUSHING_FAST;
  } else if(current_flow >= FLUSHING_THRESHOLD){
    current_state = 1;
    current_state_label = FLUSHING_SLOW;
  } else {
    current_state = 0;
    current_state_label = NO_FLOW;
  }

  for (int i = STATES_ARRAY_SIZE - 1; i >= 0; i--) {
    previous_states[i] = previous_states[i - 1];
  }
  previous_states[0] = current_state;
  printf("previous water flow samples!\n");
  printf("[%d], [%d], [%d]\n", previous_states[0], previous_states[1], previous_states[2]);

  bool repetition = true;
  // Checking the last samples
  for (int i = 1; i < STATES_ARRAY_SIZE; i++) {
    repetition = repetition && (previous_states[i - 1] == previous_states[i]);
  }
  printf("repetition [%d] \n", repetition);

  //If NO_FLOW, turn off light
  if (current_state == 0){
    putLights(LED_WTR, false);
  }//OTHERWISE, turn on light
  else {
    putLights(LED_WTR, true);
  }
  
  // Report if there is a stable change and it is different wiht the one reported before.
  if (repetition && current_state != previous_reported_state) {
    printf("State changes!\n");
    // Update the previous state and pub the current state to Thingsboard.
    previous_reported_state = current_state;
    pub_water_data();
  }
}

// Pub the state or specific number of the water_flow_sensor.
void pub_water_data(){
  char payload[32];
  
  printf("Water flow sensor sensing event!\n");
  //snprintf(payload, sizeof(payload), "{\"flow_number\":\"%d\"}", current_flow);
  //snprintf(payload, sizeof(payload), "{\"flow_state\":\"%d\"}", current_state);
  snprintf(payload, sizeof(payload), "{\"wtr\":\"%s\", \"wtrLvl\":%d}", current_state_label, current_state);
  tb_publish_telemetry(payload);
}

K_WORK_DEFINE(tic_water_sensor, my_wfs_expiry_fn);

/*
void my_timer_handler(struct k_timer *dummy)
{
  k_work_submit(&tic_water_sensor);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);
*/

void init_water_flow_sensor()
{
  // Configure water flow sensor GPIO.
  wfs_dev = device_get_binding(WFS_PORT);
  gpio_pin_configure(wfs_dev, WFS_PIN, GPIO_DIR_IN | GPIO_PUD_PULL_UP);

  printk("Water Flow Sensor is Configured\n");
  // Submit detecting work every 3 seconds after a second.
  //Moving timer to sensors
  //k_timer_start(&my_timer, K_SECONDS(1), K_SECONDS(3));
}