/* mbed
 * Copyright (c) 2006-2015 ARM Limited
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


#ifndef __MESSAGE_CENTER_H__
#define __MESSAGE_CENTER_H__

#include "message-center-transport/MessageCenterTransport.h"

#include "core-util/FunctionPointer.h"

#include <stdint.h>


using namespace mbed::util;


namespace MessageCenter
{
    typedef struct {
        uint8_t host;
        uint16_t port;
        BlockStatic block;
        FunctionPointer0<void> callback;
    } transaction_t;

    typedef enum {
        RemoteHost = 0x00,
        LocalHost  = 0xFF
    } host_t;

    typedef enum {
        ANCSPort  = 0x0110,
        EquipPort = 0x0120,
        ProxyPort = 0x0130
    } port_t;

    /*
        Add new transport link between local MCU and remote MCU.
        uint8_t host                      - Host ID, only local and 1 remote supported.
        MessageCenterTransport* transport - Object implementing the transport protocol.
    */
    void addTransportTask(uint8_t host, MessageCenterTransport* transport);

    /*
        Send block of data to specified MCU and port.
        uint8_t host                      - Host ID, only local and 1 remote supported.
        uint16_t port                     - Port to send to.
        BlockStatic& block                - Block to send.
        FunctionPointer0<void> callback   - Callback function when send is done.
    */
    void sendTask(uint8_t host, uint16_t port, BlockStatic& block, FunctionPointer0<void> callback);

    /*
        Add function to be called when blocks are send to the given port on the given host.
        uint8_t host                                 - Host ID, only local host supported.
        uint16_t port                                - Port to listen to.
        FunctionPointer1<void, BlockStatic> callback - Callback for handling block.
    */
    void addListenerTask(uint8_t host, uint16_t port, FunctionPointer1<void, BlockStatic> callback);

    /*
        Remove previously registrered callback function.
        uint8_t host                                 - Host ID, only local host supported.
        uint16_t port                                - Port registered to.
        FunctionPointer1<void, BlockStatic> callback - Callback to be removed.
    */
    void removeListenerTask(uint8_t host, uint16_t port, FunctionPointer1<void, BlockStatic> callback);
}


#endif // __MESSAGE_CENTER_H__