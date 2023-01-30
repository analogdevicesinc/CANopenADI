/*
 * Application interface for CANopenNode.
 *
 * @file        CO_application.c
 * @author      --
 * @copyright   2021 --
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


#include "CO_application.h"
#include "OD.h"

/******************************************************************************/
CO_ReturnError_t app_programStart(uint16_t *bitRate,
                                  uint8_t *nodeId,
                                  uint32_t *errInfo)
{
    /* Set initial CAN bitRate and CANopen nodeId. May be configured by LSS. */
    if (*bitRate == 0) *bitRate = 125;
    if (*nodeId == 0) *nodeId = 0x0A;

    return CO_ERROR_NO;
}


/******************************************************************************/
void app_communicationReset(CO_t *co)
{
    if (!co->nodeIdUnconfigured) {

    }
}


/******************************************************************************/
void app_programEnd()
{

}


/******************************************************************************/
void app_programAsync(CO_t *co, uint32_t timer1usDiff)
{
    /* Here can be slower code, all must be non-blocking. Mind race conditions
     * between this functions and following three functions, which all run from
     * realtime timer interrupt */

}


/******************************************************************************/
void app_programRt(CO_t *co, uint32_t timer1usDiff)
{

}


/******************************************************************************/
void app_peripheralRead(CO_t *co, uint32_t timer1usDiff)
{

}


/******************************************************************************/
void app_peripheralWrite(CO_t *co, uint32_t timer1usDiff)
{

}
