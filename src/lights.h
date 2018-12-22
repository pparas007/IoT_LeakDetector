#ifndef __LIGHTS_H__
#define __LIGHTS__

#define LED_MQTT 0
#define LED_FRC 1
#define LED_WTR 2
#define LED_VLV 3

void putLights(u32_t ledno, bool state);
bool getLights(u32_t ledno);
void lights_init();

#endif
