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

#include "core-util/SharedPointer.h"
#include "core-util/FunctionPointer.h"

#include <stdint.h>


using namespace mbed::util;


namespace MessageCenter
{
    typedef struct {
        uint8_t host;
        uint16_t port;
        BlockStatic* block;
        FunctionPointer0<void> callback;
    } transaction_t;

    typedef enum {
        LocalHost = 0,
        RemoteHost = 1
    } host_t;

    void addTransportTask(uint8_t host, MessageCenterTransport* transport);

    void sendTask(uint8_t host, uint16_t port, BlockStatic* block, FunctionPointer0<void> callback);

    void addListenerTask(uint8_t host, uint16_t port, FunctionPointer1<void, SharedPointer<Block> > callback);
    void removeListenerTask(uint8_t host, uint16_t port, FunctionPointer1<void, SharedPointer<Block> > callback);
}


#endif // __MESSAGE_CENTER_H__