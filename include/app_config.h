/**
 ********************************************************************************
 * @file    app_config.h
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    09.01.2025
 * @brief   Header file of the RTOS application configuration.
 ********************************************************************************
 */

#ifndef APP_CONFIG__H
#define APP_CONFIG__H

 // Task stack depthes
#define TASK_UHF_RECEIVER_STACK_DEPTH 4096
#define TASK_WIFI_STACK_DEPTH 2048
#define TASK_CAN_STACK_DEPTH 2048
#define TASK_BLINKING_LED_STACK_DEPTH 1024

// Task priorities
#define TASK_UHF_RECEIVER_PRIORITY 1
#define TASK_WIFI_PRIORITY 3
#define TASK_CAN_PRIORITY 2
#define TASK_BLINKING_LED_PRIORITY 4


#endif // APP_CONFIG__H
