/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef CONFIG_NET_CONFIG_SETTINGS
#ifdef CONFIG_NET_IPV6
#define ZEPHYR_ADDR		CONFIG_NET_CONFIG_MY_IPV6_ADDR
#define SERVER_ADDR		CONFIG_NET_CONFIG_PEER_IPV6_ADDR
#else
#define ZEPHYR_ADDR		CONFIG_NET_CONFIG_MY_IPV4_ADDR
#define SERVER_ADDR		CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#endif
#else
#ifdef CONFIG_NET_IPV6
#define ZEPHYR_ADDR		"2001:db8::1"
#define SERVER_ADDR		"2001:db8::2"
#else
#define ZEPHYR_ADDR		"192.168.1.101"
#define SERVER_ADDR		"192.168.1.10"
#endif
#endif

#define MQTT_CLIENTID		"zephyr_publisher"

#ifdef CONFIG_MQTT_LIB_TLS
#define SERVER_PORT		8883
#else
#define SERVER_PORT		1883
#endif

#define APP_NET_INIT_TIMEOUT    15000
//from Mengxuan
//#define APP_NET_INIT_TIMEOUT    30000
#define APP_TX_RX_TIMEOUT       1000
//from Mengxuan
//#define APP_TX_RX_TIMEOUT       5000
#define APP_CONN_TRIES          10
#define APP_TX_CONN_TRIES       APP_CONN_TRIES
#define APP_TX_CONN_TIMEOUT     20000

#endif
