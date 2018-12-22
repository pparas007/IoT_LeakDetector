/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #define LOG_MODULE_NAME net_tb_device
 #define NET_LOG_LEVEL LOG_LEVEL_DBG

#include <zephyr.h>

#include <net/net_context.h>

#include <misc/printk.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#if defined(CONFIG_NET_L2_BT)
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#endif

#include "tb_pubsub.h"
#include "lights.h"
#include "sensors.h"
#include "config.h"

#define ATTR_UPDATE_INTERVAL 15000

#define RC_STR(rc)  ((rc) == 0 ? "OK" : "ERROR")
#define PRINT_RESULT(func, rc)  \
  printf("[%s:%d] %s: %d <%s>\n", __func__, __LINE__, \
         (func), rc, RC_STR(rc))

static void update_attributes()
{
  static char payload[128];

  //TODO: Need to refactor to centralize
  /* Formulate the JSON payload for the attribute update */
  //Send basic info
  snprintf(payload, sizeof(payload), "{\"firmware_version\":\"%s\", \"serial_number\":\"%s\", \"uptime\":\"%d\"}",
    "1.0.0",
    "TEAM-IOT-4",
    (uint32_t)k_uptime_get_32() / 1000);
  tb_publish_attributes(payload);

  //TODO: one method to send attributes
}

#if defined(CONFIG_NET_L2_BT)
static bool bt_connected;

static
void bt_connect_cb(struct bt_conn *conn, u8_t err)
{
  bt_connected = true;
}

static
void bt_disconnect_cb(struct bt_conn *conn, u8_t reason)
{
  bt_connected = false;
  printf("bt disconnected (reason %u)\n", reason);
}

static
struct bt_conn_cb bt_conn_cb = {
  .connected = bt_connect_cb,
  .disconnected = bt_disconnect_cb,
};
#endif

static int network_setup(void)
{

#if defined(CONFIG_NET_L2_BT)
  const char *progress_mark = "/-\\|";
  int i = 0;
  int rc;

  rc = bt_enable(NULL);
  if (rc && rc != -EALREADY) {
    printf("bluetooth init failed\n");
    return rc;
  }

  bt_conn_cb_register(&bt_conn_cb);

  printf("\nwaiting for bt connection: ");
  while (bt_connected == false) {
    k_sleep(250);
    printf("%c\b", progress_mark[i]);
    i = (i + 1) % (sizeof(progress_mark) - 1);
  }
  printf("\n");
#endif

  return 0;
}

void main(void)
{
  int rc;

  rc = network_setup();
  PRINT_RESULT("network_setup", rc);
  if (rc < 0) {
    return;
  }
  
  lights_init();
  tb_pubsub_start();
  
  k_sleep(ATTR_UPDATE_INTERVAL);
  update_attributes();
  sensors_start();
  
  /*
  while (true) {
    k_sleep(ATTR_UPDATE_INTERVAL);
    update_attributes();
  }
  */
}
