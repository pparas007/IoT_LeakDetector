#include <zephyr.h>
#include <board.h>
#include <device.h>
#include <adc.h>
#include <kernel.h>
#include <hal/nrf_saadc.h>
#include <misc/printk.h>
#include "config.h"
#include "tb_pubsub.h"
#include "lights.h"

//http://infocenter.nordicsemi.com/pdf/nRF52840_PS_v1.0.pdf
//pg 349
#define ADC_DEVICE_NAME   CONFIG_ADC_0_NAME
#define ADC_RESOLUTION    10
#define ADC_GAIN    ADC_GAIN_1_6 //1/6
#define ADC_REFERENCE   ADC_REF_INTERNAL //0.6
#define ADC_ACQUISITION_TIME  ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_1ST_CHANNEL_ID  0
#define ADC_1ST_CHANNEL_INPUT NRF_SAADC_INPUT_AIN1
#define ADC_2ND_CHANNEL_ID  2
#define ADC_2ND_CHANNEL_INPUT NRF_SAADC_INPUT_AIN2

#define BUFFER_SIZE  6
static s16_t m_sample_buffer[BUFFER_SIZE];

static const struct adc_channel_cfg m_1st_channel_cfg = {
  .gain             = ADC_GAIN,
  .reference        = ADC_REFERENCE,
  .acquisition_time = ADC_ACQUISITION_TIME,
  .channel_id       = ADC_1ST_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
  .input_positive   = ADC_1ST_CHANNEL_INPUT,
#endif
};

#if defined(ADC_2ND_CHANNEL_ID)
static const struct adc_channel_cfg m_2nd_channel_cfg = {
  .gain             = ADC_GAIN,
  .reference        = ADC_REFERENCE,
  .acquisition_time = ADC_ACQUISITION_TIME,
  .channel_id       = ADC_2ND_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
  .input_positive   = ADC_2ND_CHANNEL_INPUT,
#endif
};
#endif /* defined(ADC_2ND_CHANNEL_ID) */

//method to init ADC interface
static struct device *init_adc(void)
{
  int ret;
  struct device *adc_dev = device_get_binding(ADC_DEVICE_NAME);

  if (!adc_dev) {
    printk("Cannot get ADC device");
    return;
  }

  ret = adc_channel_setup(adc_dev, &m_1st_channel_cfg);
  if (ret) {
    printk("Setting up of the first channel failed with code %d", ret);
  }

#if defined(ADC_2ND_CHANNEL_ID)
  ret = adc_channel_setup(adc_dev, &m_2nd_channel_cfg);
  if (ret) {
    printk("Setting up of the second channel failed with code %d", ret);
  }
#endif /* defined(ADC_2ND_CHANNEL_ID) */

  (void)memset(m_sample_buffer, 0, sizeof(m_sample_buffer));

  return adc_dev;
}

//method to sample sensor, which in turns calls init_adc
int sample_sensor (int channel_id)
{
  int ret;

  const struct adc_sequence sequence = {
    .channels    = BIT(channel_id),
    .buffer      = m_sample_buffer,
    .buffer_size = sizeof(m_sample_buffer),
    .resolution  = ADC_RESOLUTION,
  };

  struct device *adc_dev = init_adc();

  if (!adc_dev) {
    printk("Failed to initialise ADC");
    return -1;
  }

  ret = adc_read(adc_dev, &sequence);

  if (ret) {
    printk("Failed to read ADC with code %d", ret);
  }

  return m_sample_buffer[0];
}

u32_t state[4];
int current_value;
u8_t current_level;
// Non use constant
u8_t previous_reported_level = 4;
char *current_level_label;

int SAMPLE_LOW_LEVEL = 10;
int SAMPLE_MID_LEVEL = 250;
int SAMPLE_HIGH_LEVEL = 500;

#define SAMPLE_NONE_LEVEL_LABEL  "NONE"
#define SAMPLE_LOW_LEVEL_LABEL  "LOW"
#define SAMPLE_MID_LEVEL_LABEL  "MID"
#define SAMPLE_HIGH_LEVEL_LABEL  "HIGH"

#define SLIDING_WINDOW_SIZE  3


// Low is represented by 1
// Mid is be represented by 2
// High is be represented by 3
u8_t previous_samples_levels[SLIDING_WINDOW_SIZE];


//TODO: Don't think the next line is necessary
//TODO: Correct, following line is unnecessary if code reorganized
//int btn_alert_handler2(struct k_alert *alert);
//renaming force_pub_alert_handler because it is being repeated across other c files

int force_pub_alert_handler(struct k_alert *alert)
{
	int value;
	//char payload[16];
  char payload[32];

  /* Context: Zephy kernel workqueue thread */

	printf("Force sensor sensing event!\n");

  /* Iterate over each button looking for a change from the previously
    * published state */
  //Skipping that for now, see below for example of comparing states for the button
  //for (u32_t i = 0; i < BTN_COUNT; i++) {
  //gpio_pin_read(btn_dev, btn_arr[i], &value);
  //if (value != state[i]) {
    //TODO: hardcoding values
    u32_t i = 1;
    value = 1;
    /* Formulate JSON in the format expected by thingsboard.io */
    //TODO: Uncomment after initial button telemetry test
    //u32_t i;
		//int value;
	  //snprintf(payload, sizeof(payload), "{\"btn%d\":%s}", i, value ? "false" : "true");
    //snprintf(payload, sizeof(payload), "{\"frc\":\"%s\"}", current_level_label);
    snprintf(payload, sizeof(payload), "{\"frc\":\"%s\", \"frcLvl\":%d}", current_level_label, current_level);
    //snprintf(payload, sizeof(payload), "{\"frcVal\":%d}", current_value);
    //snprintf(payload, sizeof(payload), "{\"firmware_version\":\"%s\", \"serial_number\":\"%s\", \"uptime\":\"%d\"}",
    //"1.2.3",
    //"jdukes-001",
    //(uint32_t)k_uptime_get_32() / 1000);
    previous_reported_level = current_level;
    tb_publish_telemetry(payload);
    //TODO: If it is a different state, then change
    //state[i] = value;
  //}
  //}
	return 0;
}

//K_ALERT_DEFINE(name, handler, max_num_pending_alerts)
K_ALERT_DEFINE(force_pub_alert, force_pub_alert_handler, 10);

void update_sample_history() {
  // Updating sliding window
  for (int i = SLIDING_WINDOW_SIZE - 1; i >= 0; i--) {
    previous_samples_levels[i] = previous_samples_levels[i - 1];
  }
  previous_samples_levels[0] = current_level;
  printf("previous force samples!\n");
  printf("[%d], [%d], [%d]\n", previous_samples_levels[0], previous_samples_levels[1], previous_samples_levels[2]);
}

void check_and_report() {
  update_sample_history();

  bool repetition = true;
  // Checking the last samples
  for (int i = 1; i < SLIDING_WINDOW_SIZE; i++) {
    repetition = repetition && (previous_samples_levels[i - 1] == previous_samples_levels[i]);
  }
  printf("repetition [%d] \n", repetition);

  //If NONE turn off light
  if (current_level == 0){
    putLights(LED_FRC, false);
  }//OTHERWISE, turn on light
  else {
    putLights(LED_FRC, true);
  }

  // Report if there is a stable level and it is different than the one reported before
  if (repetition && current_level != previous_reported_level) {
    //TODO: Review the purpose of the alert
    printf("Signalling alert!\n");
    k_alert_send(&force_pub_alert);
  }
}

//
void tic_force_sensor_handler(struct k_work *work)
{
  int sample;
  int fsrVoltage;
  unsigned long fsrResistance;  // The voltage converted to resistance, can be very big so make "long"
  unsigned long fsrConductance; 
  long fsrForce;       // Finally, the resistance converted to force
  
  printk("\nSampling ... ");

  sample = sample_sensor(ADC_1ST_CHANNEL_ID);
  current_value = sample;
  printk("%d \n", sample);

  // analog voltage reading ranges from about 2 to 783 which maps to 0V to 3.3V (= 3300mV)
  //fsrVoltage = map(fsrReading, 1, 783, 0, 3300);
  //double fsrVoltage = ((sample - 1) / ((double) (783 - 1))) * 3300;
  //fsrVoltage = 3300 / (1023);
  //fsrVoltage = 3300 / (796);
  //fsrVoltage = fsrVoltage * (sample - 1);
  fsrVoltage = 3300 * (sample - 1) / 800;

  //double fsrVoltage = ((sample - 1) / ((double) (783 - 1))) * 3300;
  //printk("%f \n", fsrVoltage);
  printk("Voltage reading in mV = %d \n", fsrVoltage);

  // The voltage = Vcc * R / (R + FSR) where R = 4.7K and Vcc = 3.3V
  // so FSR = ((Vcc - V) * R) / V        yay math!
  fsrResistance = 3300 - fsrVoltage;     // fsrVoltage is in millivolts so 3.3V = 3300mV
  fsrResistance *= 4700;                // 4.7K resistor
  fsrResistance /= fsrVoltage;
  printk("FSR resistance in ohms = %ld \n", fsrResistance);

  fsrConductance = 1000000;           // we measure in micromhos so 
  fsrConductance /= fsrResistance;
  printk("Conductance in microMhos: %ld \n", fsrConductance);

  /*
  // Use the two FSR guide graphs to approximate the force
  if (fsrConductance <= 1000) {
    fsrForce = fsrConductance / 80;
    Serial.print("Force in Newtons: ");
    Serial.println(fsrForce);      
  } else {
    fsrForce = fsrConductance - 1000;
    fsrForce /= 30;
    Serial.print("Force in Newtons: ");
    Serial.println(fsrForce);            
  }*/

  /* Context: worker handler */
  printf("sample: %d\n", sample);

  if (sample >= SAMPLE_HIGH_LEVEL) {
    current_level = 3;
    current_level_label = SAMPLE_HIGH_LEVEL_LABEL;
  }
  else {
    if (sample >= SAMPLE_MID_LEVEL) {
      current_level = 2;
      current_level_label = SAMPLE_MID_LEVEL_LABEL;
    }
    else {
      if (sample >= SAMPLE_LOW_LEVEL) {
        current_level = 1;
        current_level_label = SAMPLE_LOW_LEVEL_LABEL;
      }
      else {
        current_level = 0;
        current_level_label = SAMPLE_NONE_LEVEL_LABEL;
      }
    }
  }

  check_and_report();
}

K_WORK_DEFINE(tic_force_sensor, tic_force_sensor_handler);

/*
void my_timer_handler(struct k_timer *dummy)
{
  k_work_submit(&tic_force_sensor);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);
*/

void init_force_sensor()
{
  printk("Preparing ADC\n");
  /* start periodic timer that expires once every X period seconds */
  //https://docs.zephyrproject.org/1.9.0/kernel/timing/timers.html
  
  //Moving timer to sensors
  //k_timer_start(name, duration, period)
  //k_timer_start(&my_timer, K_SECONDS(1), K_SECONDS(5));
  //k_timer_stop(&my_timer);
}