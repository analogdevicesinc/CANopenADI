/*
 * CAN module object for generic microcontroller.
 *
 * This file is a template for other microcontrollers.
 *
 * @file        CO_driver.c
 * @ingroup     CO_driver
 * @author      Janez Paternoster
 * @copyright   2004 - 2020 Janez Paternoster
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

#include "mxc_device.h"
#include "can.h"

#include "301/CO_driver.h"

#define MAP_B   1

/* Global variables and objects */
mxc_can_req_t rxReq;
mxc_can_msg_info_t rxInfo;
uint8_t rxData[64];
CO_CANmodule_t *CANthis;

/* CAN driver callback functions */
void canUnitEvent_cb(uint32_t can_idx, uint32_t event);
void canObjEvent_cb(uint32_t can_idx, uint32_t event);

/******************************************************************************/
void CO_CANsetConfigurationMode(void *CANptr){
    /* Put CAN module in configuration mode */
    int err = MXC_CAN_SetMode(MXC_CAN_GET_IDX(CANptr),
            MXC_CAN_MODE_INITIALIZATION);
    if (err != E_NO_ERROR) {
        printf("%s: Error: MXC_CAN_SetMode() failed: %d\n", __func__, err);
    }
}


/******************************************************************************/
void CO_CANsetNormalMode(CO_CANmodule_t *CANmodule){
    /* Put CAN module in normal mode */
    if (MXC_CAN_SetMode(MXC_CAN_GET_IDX(CANmodule->CANptr),
            MXC_CAN_MODE_NORMAL) != E_NO_ERROR) {
        printf("%s: Error: MXC_CAN_SetMode() failed\n", __func__);
    } else {
        CANmodule->CANnormal = true;
    }
}


/******************************************************************************/
CO_ReturnError_t CO_CANmodule_init(
        CO_CANmodule_t         *CANmodule,
        void                   *CANptr,
        CO_CANrx_t              rxArray[],
        uint16_t                rxSize,
        CO_CANtx_t              txArray[],
        uint16_t                txSize,
        uint16_t                CANbitRate)
{
    uint16_t i;
    uint32_t bitrate = CANbitRate * 1000;

    /* verify arguments */
    if(CANmodule==NULL || rxArray==NULL || txArray==NULL){
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    /* Configure object variables */
    CANmodule->CANptr = CANptr;
    CANmodule->rxArray = rxArray;
    CANmodule->rxSize = rxSize;
    CANmodule->txArray = txArray;
    CANmodule->txSize = txSize;
    CANmodule->CANerrorStatus = 0;
    CANmodule->CANnormal = false;
    /* Number of hardware filters are usually less than rxSize. */
    CANmodule->useCANrxFilters = false;
    CANmodule->bufferInhibitFlag = false;
    CANmodule->firstCANtxMessage = true;
    CANmodule->CANtxCount = 0U;
    CANmodule->errOld = 0U;

    CANthis = CANmodule;

    for(i=0U; i<rxSize; i++){
        rxArray[i].ident = 0U;
        rxArray[i].mask = 0xFFFFU;
        rxArray[i].object = NULL;
        rxArray[i].CANrx_callback = NULL;
    }
    for(i=0U; i<txSize; i++){
        txArray[i].bufferFull = false;
    }


    /* Configure CAN module registers */
#if TARGET_NUM == 32662
    if (MXC_CAN_Init(MXC_CAN_GET_IDX(CANmodule->CANptr), MXC_CAN_OBJ_CFG_TXRX,
            canUnitEvent_cb, canObjEvent_cb, MAP_B) != E_NO_ERROR) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
#elif TARGET_NUM == 32690
    if (MXC_CAN_Init(MXC_CAN_GET_IDX(CANmodule->CANptr),
            MXC_CAN_OBJ_CFG_TX_RX_DATA, canUnitEvent_cb,
            canObjEvent_cb) != E_NO_ERROR) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
#endif

    /* Configure CAN timing */
    if (MXC_CAN_SetBitRate(MXC_CAN_GET_IDX(CANmodule->CANptr),
            MXC_CAN_BITRATE_SEL_NOMINAL, bitrate,
            MXC_CAN_BIT_SEGMENTS(7, 2, 2)) != E_NO_ERROR) {
        printf("%s: Error: MXC_CAN_SetBitrate() failed\n", __func__);
        return CO_ERROR_ILLEGAL_BAUDRATE;
    }

    /* Configure CAN module hardware filters */
    if(CANmodule->useCANrxFilters){
        /* CAN module filters are used, they will be configured with */
        /* CO_CANrxBufferInit() functions, called by separate CANopen */
        /* init functions. */
        /* Configure all masks so, that received message must match filter */
    }
    else{
        /* CAN module filters are not used, all messages with standard 11-bit */
        /* identifier will be received */
        /* Configure mask 0 so, that all messages with standard identifier are accepted */
        MXC_CAN_ObjectSetFilter(MXC_CAN_GET_IDX(CANmodule->CANptr),
                MXC_CAN_FILT_CFG_MASK_ADD | MXC_CAN_FILT_CFG_SINGLE_STD_ID,
                0x7FF, 0);
    }

    /* Store message read request */
    rxReq.data = rxData;
    rxReq.data_sz = sizeof(rxData);
    rxReq.msg_info = &rxInfo;
    if (MXC_CAN_MessageReadAsync(MXC_CAN_GET_IDX(CANmodule->CANptr), &rxReq)
            < E_NO_ERROR) {
        printf("%s: Error: MXC_CAN_MessageReadAsync() failed\n", __func__);
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    return CO_ERROR_NO;
}


/******************************************************************************/
void CO_CANmodule_disable(CO_CANmodule_t *CANmodule) {
    if (CANmodule != NULL) {
        /* turn off the module */
        if (MXC_CAN_PowerControl(MXC_CAN_GET_IDX(CANmodule->CANptr),
                MXC_CAN_PWR_CTRL_OFF) != E_NO_ERROR) {
            printf("%s: Error: MXC_CAN_PowerControl() failed\n", __func__);
        }
        if (MXC_CAN_UnInit(MXC_CAN_GET_IDX(CANmodule->CANptr)) != E_NO_ERROR) {
            printf("%s: Error: MXC_CAN_UnInit() failed\n", __func__);
        }
    }
}


/******************************************************************************/
CO_ReturnError_t CO_CANrxBufferInit(
        CO_CANmodule_t         *CANmodule,
        uint16_t                index,
        uint16_t                ident,
        uint16_t                mask,
        bool_t                  rtr,
        void                   *object,
        void                  (*CANrx_callback)(void *object, void *message))
{
    CO_ReturnError_t ret = CO_ERROR_NO;

    if((CANmodule!=NULL) && (object!=NULL) && (CANrx_callback!=NULL) && (index < CANmodule->rxSize)){
        /* buffer, which will be configured */
        CO_CANrx_t *buffer = &CANmodule->rxArray[index];

        /* Configure object variables */
        buffer->object = object;
        buffer->CANrx_callback = CANrx_callback;

        /* CAN identifier and CAN mask, bit aligned with CAN module. Different on different microcontrollers. */
        buffer->ident = ident & 0x07FFU;
        if(rtr){
            buffer->ident |= 0x0800U;
        }
        buffer->mask = (mask & 0x07FFU) | 0x0800U;

        /* Set CAN hardware module filter and mask. */
        if(CANmodule->useCANrxFilters){
            __NOP();
        }
    }
    else{
        ret = CO_ERROR_ILLEGAL_ARGUMENT;
    }

    return ret;
}


/******************************************************************************/
CO_CANtx_t *CO_CANtxBufferInit(
        CO_CANmodule_t         *CANmodule,
        uint16_t                index,
        uint16_t                ident,
        bool_t                  rtr,
        uint8_t                 noOfBytes,
        bool_t                  syncFlag)
{
    CO_CANtx_t *buffer = NULL;

    if((CANmodule != NULL) && (index < CANmodule->txSize)){
        /* get specific buffer */
        buffer = &CANmodule->txArray[index];

        /* CAN identifier, DLC and rtr, bit aligned with CAN module transmit buffer.
         * Microcontroller specific. */
        buffer->ident = ((uint32_t)ident & 0x07FFU)
                      | ((uint32_t)(((uint32_t)noOfBytes & 0xFU) << 12U))
                      | ((uint32_t)(rtr ? 0x8000U : 0U));

        buffer->bufferFull = false;
        buffer->syncFlag = syncFlag;
        buffer->DLC = noOfBytes;
    }

    return buffer;
}


/******************************************************************************/
static int can_MessageSend(void *canPtr, CO_CANtx_t *buffer)
{
    mxc_can_req_t req;
    mxc_can_msg_info_t info;

    info.brs = 0;
    info.dlc = buffer->DLC;
    info.esi = 0;
    info.fdf = 0;
    info.msg_id = MXC_CAN_STANDARD_ID(buffer->ident);
    info.rsv = 0;
    info.rtr = 0;
    req.data = buffer->data;
    req.data_sz = buffer->DLC;
    req.msg_info = &info;
    return MXC_CAN_MessageSendAsync(MXC_CAN_GET_IDX(canPtr), &req);
}

CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer){
    CO_ReturnError_t err = CO_ERROR_NO;
    uint8_t canStat;

    /* Verify overflow */
    if(buffer->bufferFull){
        if(!CANmodule->firstCANtxMessage){
            /* don't set error, if bootup message is still on buffers */
            CANmodule->CANerrorStatus |= CO_CAN_ERRTX_OVERFLOW;
        }
        err = CO_ERROR_TX_OVERFLOW;
    }

    CO_LOCK_CAN_SEND(CANmodule);
    /* if CAN TX buffer is free, copy message to it */
    canStat = ((mxc_can_regs_t *) CANmodule->CANptr)->stat;
    if((canStat & MXC_F_CAN_STAT_TXBUF) && CANmodule->CANtxCount == 0){
        if (can_MessageSend(CANmodule->CANptr, buffer) != E_NO_ERROR) {
            err = CO_ERROR_TX_BUSY;
        }
        CANmodule->bufferInhibitFlag = buffer->syncFlag;
        /* copy message and txRequest */
    }
    /* if no buffer is free, message will be sent by interrupt */
    else{
        buffer->bufferFull = true;
        CANmodule->CANtxCount++;
    }
    CO_UNLOCK_CAN_SEND(CANmodule);

    return err;
}


/******************************************************************************/
void CO_CANclearPendingSyncPDOs(CO_CANmodule_t *CANmodule){
    uint32_t tpdoDeleted = 0U;

    CO_LOCK_CAN_SEND(CANmodule);
    /* Abort message from CAN module, if there is synchronous TPDO.
     * Take special care with this functionality. */
    if(/*messageIsOnCanBuffer && */CANmodule->bufferInhibitFlag){
        /* clear TXREQ */
        CANmodule->bufferInhibitFlag = false;
        tpdoDeleted = 1U;
    }
    /* delete also pending synchronous TPDOs in TX buffers */
    if(CANmodule->CANtxCount != 0U){
        uint16_t i;
        CO_CANtx_t *buffer = &CANmodule->txArray[0];
        for(i = CANmodule->txSize; i > 0U; i--){
            if(buffer->bufferFull){
                if(buffer->syncFlag){
                    buffer->bufferFull = false;
                    CANmodule->CANtxCount--;
                    tpdoDeleted = 2U;
                }
            }
            buffer++;
        }
    }
    CO_UNLOCK_CAN_SEND(CANmodule);


    if(tpdoDeleted != 0U){
        CANmodule->CANerrorStatus |= CO_CAN_ERRTX_PDO_LATE;
    }
}


/******************************************************************************/
/* Get error counters from the module. If necessary, function may use
    * different way to determine errors. */
void CO_CANmodule_process(CO_CANmodule_t *CANmodule) {
    uint32_t err;
    uint16_t rxErrors=0, txErrors=0, overflow=0;

    overflow = (MXC_CAN0->stat & MXC_F_CAN_STAT_DOR) ? 1 : 0;
    txErrors = MXC_CAN0->txerr;
    rxErrors = MXC_CAN0->rxerr;
    err = ((uint32_t)txErrors << 16) | ((uint32_t)rxErrors << 8) | overflow;

    if (CANmodule->errOld != err) {
        uint16_t status = CANmodule->CANerrorStatus;

        CANmodule->errOld = err;

        if (txErrors >= 256U) {
            /* bus off */
            status |= CO_CAN_ERRTX_BUS_OFF;
        }
        else {
            /* recalculate CANerrorStatus, first clear some flags */
            status &= 0xFFFF ^ (CO_CAN_ERRTX_BUS_OFF |
                                CO_CAN_ERRRX_WARNING | CO_CAN_ERRRX_PASSIVE |
                                CO_CAN_ERRTX_WARNING | CO_CAN_ERRTX_PASSIVE);

            /* rx bus warning or passive */
            if (rxErrors >= 128) {
                status |= CO_CAN_ERRRX_WARNING | CO_CAN_ERRRX_PASSIVE;
            } else if (rxErrors >= 96) {
                status |= CO_CAN_ERRRX_WARNING;
            }

            /* tx bus warning or passive */
            if (txErrors >= 128) {
                status |= CO_CAN_ERRTX_WARNING | CO_CAN_ERRTX_PASSIVE;
            } else if (rxErrors >= 96) {
                status |= CO_CAN_ERRTX_WARNING;
            }

            /* if not tx passive clear also overflow */
            if ((status & CO_CAN_ERRTX_PASSIVE) == 0) {
                status &= 0xFFFF ^ CO_CAN_ERRTX_OVERFLOW;
            }
        }

        if (overflow != 0) {
            /* CAN RX bus overflow */
            status |= CO_CAN_ERRRX_OVERFLOW;
        }

        CANmodule->CANerrorStatus = status;
    }
}


/******************************************************************************/
void CO_CANTXinterrupt(CO_CANmodule_t *CANmodule){
    /* Clear interrupt flag */

    /* First CAN message (bootup) was sent successfully */
    CANmodule->firstCANtxMessage = false;
    /* clear flag from previous message */
    CANmodule->bufferInhibitFlag = false;
    /* Are there any new messages waiting to be send */
    if(CANmodule->CANtxCount > 0U){
        uint16_t i;             /* index of transmitting message */
        /* first buffer */
        CO_CANtx_t *buffer = &CANmodule->txArray[0];
        /* search through whole array of pointers to transmit message buffers. */
        for(i = CANmodule->txSize; i > 0U; i--){
            /* if message buffer is full, send it. */
            if(buffer->bufferFull){
                buffer->bufferFull = false;
                CANmodule->CANtxCount--;

                /* Copy message to CAN buffer */
                CANmodule->bufferInhibitFlag = buffer->syncFlag;
                /* canSend... */
                if (can_MessageSend(CANmodule->CANptr, buffer) < E_NO_ERROR) {
                    printf("Error: can_MessageSend() failed\n");
                }
                break; /* exit for loop */
            }
            buffer++;
        }/* end of for loop */

        /* Clear counter if no more messages */
        if(i == 0U){
            CANmodule->CANtxCount = 0U;
            MXC_CAN_DisableInt(MXC_CAN_GET_IDX(CANmodule->CANptr), MXC_F_CAN_INTEN_TX, 0);
        }
    }
}

void CO_CANRXinterrupt(CO_CANmodule_t *CANmodule){
    CO_CANrxMsg_t rcvMsg;      /* pointer to received message in CAN module */
    uint16_t index;             /* index of received message */
    uint32_t rcvMsgIdent;       /* identifier of the received message */
    CO_CANrx_t *buffer = NULL;  /* receive message buffer from CO_CANmodule_t object. */
    bool_t msgMatched = false;

    rcvMsg.ident = rxReq.msg_info->msg_id;
    rcvMsg.DLC = rxReq.msg_info->dlc;
    memcpy(rcvMsg.data, rxReq.data, rcvMsg.DLC);

    rcvMsgIdent = rcvMsg.ident;
    if(CANmodule->useCANrxFilters){
        /* CAN module filters are used. Message with known 11-bit identifier has */
        /* been received */
        index = 0;  /* get index of the received message here. Or something similar */
        if(index < CANmodule->rxSize){
            buffer = &CANmodule->rxArray[index];
            /* verify also RTR */
            if(((rcvMsgIdent ^ buffer->ident) & buffer->mask) == 0U){
                msgMatched = true;
            }
        }
    }
    else{
        /* CAN module filters are not used, message with any standard 11-bit identifier */
        /* has been received. Search rxArray form CANmodule for the same CAN-ID. */
        buffer = &CANmodule->rxArray[0];
        for(index = CANmodule->rxSize; index > 0U; index--){
            if(((rcvMsgIdent ^ buffer->ident) & buffer->mask) == 0U){
                msgMatched = true;
                break;
            }
            buffer++;
        }
    }

    /* Call specific function, which will process the message */
    if(msgMatched && (buffer != NULL) && (buffer->CANrx_callback != NULL)){
        buffer->CANrx_callback(buffer->object, (void*) &rcvMsg);
    }
}

///< Callback used when a bus event occurs
void canUnitEvent_cb(uint32_t can_idx, uint32_t event)
{
    switch (event) {
    case MXC_CAN_UNIT_EVT_INACTIVE:
        printf("Peripherals entered inactive state\n");
        break;
    case MXC_CAN_UNIT_EVT_ACTIVE:
        printf("Peripherals entered active state\n");
        break;
    case MXC_CAN_UNIT_EVT_WARNING:
        printf("Peripheral received error warning\n");
        break;
    case MXC_CAN_UNIT_EVT_PASSIVE:
        printf("Peripheral entered passive state\n");
        break;
    case MXC_CAN_UNIT_EVT_BUS_OFF:
        printf("Bus turned off\n");
        break;
    default:
        printf("Undefined event\n");
    }
}

///< Callback used when a transmission event occurs
void canObjEvent_cb(uint32_t can_idx, uint32_t event)
{
    switch (event) {
    case MXC_CAN_OBJ_EVT_TX_COMPLETE:
        CO_CANTXinterrupt(CANthis);
        break;
    case MXC_CAN_OBJ_EVT_RX:
        CO_CANRXinterrupt(CANthis);
        break;
    case MXC_CAN_OBJ_EVT_RX_OVERRUN:
        break;
    default:
        printf("Undefined event\n");
    }
}
