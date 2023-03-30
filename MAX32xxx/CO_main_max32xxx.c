/*
 * CANopen main program file.
 *
 * @file        CO_main_max32xxx.c
 * @author      Analog Devices, Inc.    2023
 * @author      Janez Paternoster       2021
 * @copyright   2021 Janez Paternoster
 *
 * This file is part of CANopenNode, an opensource CANopen Stack.
 * Project home page is <https://github.com/CANopenNode/CANopenNode>.
 * For more information on CANopen see <http://www.can-cia.org/>.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdio.h>
#include <string.h>

#include "mxc_device.h"
#include "mxc_sys.h"
#include "mxc_delay.h"
#include "can.h"
#include "led.h"
#include "nvic_table.h"

#include "CANopen.h"
#include "OD.h"
#include "CO_application.h"
#include "CO_storageBlank.h"


#define log_printf(macropar_message, ...) \
        printf(macropar_message, ##__VA_ARGS__)


/* default values for CO_CANopenInit() */
#define NMT_CONTROL \
            CO_NMT_STARTUP_TO_OPERATIONAL \
          | CO_NMT_ERR_ON_ERR_REG \
          | CO_ERR_REG_GENERIC_ERR \
          | CO_ERR_REG_COMMUNICATION
#define FIRST_HB_TIME 500
#define SDO_SRV_TIMEOUT_TIME 1000
#define SDO_CLI_TIMEOUT_TIME 500
#define SDO_CLI_BLOCK false
#define OD_STATUS_BITS NULL


/* Global variables and objects */
CO_t *CO = NULL; /* CANopen object */
uint8_t LED_red, LED_green;
volatile uint32_t ticksMs = 0;

/* 1ms interrupt handler */
void tmrTask_thread(void);

/* CAN interrupt handler */
void CO_CAN1InterruptHandler(void);

/* main ***********************************************************************/
int main (void){
    CO_ReturnError_t err;
    CO_NMT_reset_cmd_t reset = CO_RESET_NOT;
    uint32_t heapMemoryUsed;
    uint32_t errInfo = 0;
    void *CANptr = NULL; /* CAN module address */
    uint8_t pendingNodeId = 0; /* read from dip switches or nonvolatile memory, configurable by LSS slave */
    uint8_t activeNodeId = 0; /* Copied from CO_pendingNodeId in the communication reset section */
    uint16_t pendingBitRate = 125;  /* read from dip switches or nonvolatile memory, configurable by LSS slave */

#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
    CO_storage_t storage;
    CO_storage_entry_t storageEntries[] = {
        {
            .addr = &OD_PERSIST_COMM,
            .len = sizeof(OD_PERSIST_COMM),
            .subIndexOD = 2,
            .attr = CO_storage_cmd | CO_storage_restore,
            .addrNV = NULL
        }
    };
    uint8_t storageEntriesCount = sizeof(storageEntries) / sizeof(storageEntries[0]);
    uint32_t storageInitError = 0;
#endif

    /* Configure microcontroller. */


    /* Allocate memory */
    CO_config_t *config_ptr = NULL;
#ifdef CO_MULTIPLE_OD
    /* example usage of CO_MULTIPLE_OD (but still single OD here) */
    CO_config_t co_config = {0};
    OD_INIT_CONFIG(co_config); /* helper macro from OD.h */
    co_config.CNT_LEDS = 1;
    co_config.CNT_LSS_SLV = 1;
    config_ptr = &co_config;
#endif /* CO_MULTIPLE_OD */
    CO = CO_new(config_ptr, &heapMemoryUsed);
    if (CO == NULL) {
        log_printf("Error: Can't allocate memory\n");
        return 0;
    }
    else {
        log_printf("Allocated %u bytes for CANopen objects\n", heapMemoryUsed);
    }


#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
    err = CO_storageBlank_init(&storage,
                               CO->CANmodule,
                               OD_ENTRY_H1010_storeParameters,
                               OD_ENTRY_H1011_restoreDefaultParameters,
                               storageEntries,
                               storageEntriesCount,
                               &storageInitError);

    if (err != CO_ERROR_NO && err != CO_ERROR_DATA_CORRUPT) {
        log_printf("Error: Storage %d\n", storageInitError);
        return 0;
    }
#endif

    err = app_programStart(&pendingBitRate, &pendingNodeId, &errInfo);
    if (err != CO_ERROR_NO) {
        log_printf("Error: app_programStart: %d\n", err);
        return 0;
    }


    while(reset != CO_RESET_APP){
/* CANopen communication reset - initialize CANopen objects *******************/
        log_printf("CANopenNode - Reset communication...\n");

        /* Wait rt_thread. */
        CO->CANmodule->CANnormal = false;

        /* Enter CAN configuration. */
#if TARGET_NUM == 32662
        CANptr = MXC_CAN0;
#elif TARGET_NUM == 32690
        CANptr = MXC_CAN0;
#else
#error "Unsupported target"
#endif
        CO_CANsetConfigurationMode((void *)&CANptr);
        CO->CANmodule->CANptr = CANptr;
        CO_CANmodule_disable(CO->CANmodule);

        /* initialize CANopen */
        err = CO_CANinit(CO, CANptr, pendingBitRate);
        if (err != CO_ERROR_NO) {
            log_printf("Error: CAN initialization failed: %d\n", err);
            return 0;
        }

        /* configure CAN interrupt registers */
        MXC_CAN_EnableInt(MXC_CAN_GET_IDX(CO->CANmodule->CANptr),
                MXC_F_CAN_INTEN_DOR | MXC_F_CAN_INTEN_BERR
              | MXC_F_CAN_INTEN_TX | MXC_F_CAN_INTEN_RX
              | MXC_F_CAN_INTEN_ERPSV | MXC_F_CAN_INTEN_ERWARN
              | MXC_F_CAN_INTEN_AL, 0);
#if TARGET_NUM == 32662
        NVIC_EnableIRQ(CAN_IRQn);
        MXC_NVIC_SetVector(CAN_IRQn, CO_CAN1InterruptHandler);
#elif TARGET_NUM == 32690
        NVIC_EnableIRQ(CAN0_IRQn);
        MXC_NVIC_SetVector(CAN0_IRQn, CO_CAN1InterruptHandler);
#else
#error "Unsupported target"
#endif

        CO_LSS_address_t lssAddress = {.identity = {
            .vendorID = OD_PERSIST_COMM.x1018_identity.vendor_ID,
            .productCode = OD_PERSIST_COMM.x1018_identity.productCode,
            .revisionNumber = OD_PERSIST_COMM.x1018_identity.revisionNumber,
            .serialNumber = OD_PERSIST_COMM.x1018_identity.serialNumber
        }};
        err = CO_LSSinit(CO, &lssAddress, &pendingNodeId, &pendingBitRate);
        if(err != CO_ERROR_NO) {
            log_printf("Error: LSS slave initialization failed: %d\n", err);
            return 0;
        }

        activeNodeId = pendingNodeId;
        uint32_t errInfo = 0;

        err = CO_CANopenInit(CO,                /* CANopen object */
                             NULL,              /* alternate NMT */
                             NULL,              /* alternate em */
                             OD,                /* Object dictionary */
                             OD_STATUS_BITS,    /* Optional OD_statusBits */
                             NMT_CONTROL,       /* CO_NMT_control_t */
                             FIRST_HB_TIME,     /* firstHBTime_ms */
                             SDO_SRV_TIMEOUT_TIME, /* SDOserverTimeoutTime_ms */
                             SDO_CLI_TIMEOUT_TIME, /* SDOclientTimeoutTime_ms */
                             SDO_CLI_BLOCK,     /* SDOclientBlockTransfer */
                             activeNodeId,
                             &errInfo);
        if(err != CO_ERROR_NO && err != CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
            if (err == CO_ERROR_OD_PARAMETERS) {
                log_printf("Error: Object Dictionary entry 0x%X\n", errInfo);
            }
            else {
                log_printf("Error: CANopen initialization failed: %d\n", err);
            }
            return 0;
        }

        err = CO_CANopenInitPDO(CO, CO->em, OD, activeNodeId, &errInfo);
        if(err != CO_ERROR_NO) {
            if (err == CO_ERROR_OD_PARAMETERS) {
                log_printf("Error: Object Dictionary entry 0x%X\n", errInfo);
            }
            else {
                log_printf("Error: PDO initialization failed: %d\n", err);
            }
            return 0;
        }

        /* Configure Timer interrupt function for execution every 1 millisecond */
        /* CPU's system tick timer is used to generate interrupt every 1 millisecond. */
        if (SysTick_Config(SystemCoreClock / 1000)) {
            log_printf("Error: Can't setup system tick\n");
            return 0;
        }
        MXC_NVIC_SetVector(SysTick_IRQn, tmrTask_thread);

        /* Configure CANopen callbacks, etc */
        if(!CO->nodeIdUnconfigured) {

#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
            if(storageInitError != 0) {
                CO_errorReport(CO->em, CO_EM_NON_VOLATILE_MEMORY,
                               CO_EMC_HARDWARE, storageInitError);
            }
#endif
        }
        else {
            log_printf("CANopenNode - Node-id not initialized\n");
        }


        /* start CAN */
        CO_CANsetNormalMode(CO->CANmodule);

        reset = CO_RESET_NOT;

        log_printf("CANopenNode - Running...\n");
        fflush(stdout);

        uint32_t lastCall = 0;
        while(reset == CO_RESET_NOT){
            /* loop for normal program execution ******************************************/
            /* get time difference since last function call */
            if ((ticksMs - lastCall) > 0) {
                uint32_t timeDifference_us = (ticksMs - lastCall) * 1000;
                lastCall = ticksMs;
                /* CANopen process */
                reset = CO_process(CO, false, timeDifference_us, NULL);

                /* Execute external application code */
                app_programAsync(CO, timeDifference_us);

                LED_red = CO_LED_RED(CO->LEDs, CO_LED_CANopen);
                LED_green = CO_LED_GREEN(CO->LEDs, CO_LED_CANopen);
                if (num_leds)
                    LED_green ? LED_On(0) : LED_Off(0);
                if (num_leds > 1)
                    LED_red ? LED_On(1) : LED_Off(1);
            }

            /* Process automatic storage */
        }
    }


    /* program exit ***************************************************************/
    /* stop threads */
    app_programEnd();

    /* disable timer interrupts before releasing resources */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    MXC_Delay(10000);

    /* delete objects from memory */
    CO_CANsetConfigurationMode((void *)&CANptr);
    CO_delete(CO);
    log_printf("CANopenNode finished\n");

    /* reset */
    log_printf("Resetting...\n");
    MXC_Delay(10000);
    MXC_SYS_Reset_Periph(MXC_SYS_RESET0_SYS);
    while(1);

    return 0;
}


/* timer thread executes in constant intervals ********************************/
void tmrTask_thread(void){
    /* get time difference since last function call */
    uint32_t timeDifference_us = 1000;
    ticksMs++;

    /* Execute external application code */
    app_peripheralRead(CO, timeDifference_us);

    CO_LOCK_OD(CO->CANmodule);
    if (!CO->nodeIdUnconfigured && CO->CANmodule->CANnormal) {
        bool_t syncWas = false;

#if (CO_CONFIG_SYNC) & CO_CONFIG_SYNC_ENABLE
        syncWas = CO_process_SYNC(CO, timeDifference_us, NULL);
#endif
#if (CO_CONFIG_PDO) & CO_CONFIG_RPDO_ENABLE
        CO_process_RPDO(CO, syncWas, timeDifference_us, NULL);
#endif

        /* Execute external application code */
        app_programRt(CO, timeDifference_us);

#if (CO_CONFIG_PDO) & CO_CONFIG_TPDO_ENABLE
        CO_process_TPDO(CO, syncWas, timeDifference_us, NULL);
#endif

            /* Further I/O or nonblocking application code may go here. */
        }
    CO_UNLOCK_OD(CO->CANmodule);

    app_peripheralWrite(CO, timeDifference_us);
}


/* CAN interrupt function executes on received CAN message ********************/
void CO_CAN1InterruptHandler(void){
    /* interrupt flag cleared in MXC_CAN_Handler */
#if TARGET_NUM == 32662
    MXC_CAN_Handler(MXC_CAN_GET_IDX(MXC_CAN0));
#elif TARGET_NUM == 32690
    MXC_CAN_Handler(MXC_CAN_GET_IDX(MXC_CAN0));
#else
#error "Unsupported target"
#endif
}
