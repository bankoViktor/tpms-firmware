/**
 ********************************************************************************
 * @file    task_params.h
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    09.01.2025
 * @brief   Header file of the RTOS task parameters.
 ********************************************************************************
 */

#ifndef TASK_PARAMS__H
#define TASK_PARAMS__H

#include <freertos/FreeRTOS.h>


 /// @brief Task intit structure.
typedef struct task_init_params_t {
    const char *name;               /// @brief Task name.
    TaskFunction_t func;            /// @brief Task function.
    const uint32_t stack_depth;     /// @brief Task stack depth.
    void *const parameter;          /// @brief Task start parameter.
    UBaseType_t priority;           /// @brief Task priority.
    TaskHandle_t *const handle;     /// @brief Task handle.
} task_init_params_t;

#endif // TASK_PARAMS__H
