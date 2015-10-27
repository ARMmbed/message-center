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

#include "message-center/MessageCenter.h"


#include <queue>
#include <map>


#define MAX_TRANSPORT_CONNECTIONS 3

static MessageCenterTransport* transportMap[MAX_TRANSPORT_CONNECTIONS] = {0};

static std::queue<MessageCenter::transaction_t> sendQueue;
static std::map<uint16_t, FunctionPointer1<void, SharedPointer<Block> > > callbackMap;

static void processQueue();
static void sendDone();
static void receivedBlock(uint16_t port, SharedPointer<Block> block);


namespace MessageCenter
{


    void addTransportTask(uint8_t host, MessageCenterTransport* transport)
    {
        if ((host < MAX_TRANSPORT_CONNECTIONS) && (transport))
        {
            transportMap[host] = transport;

            transport->onReceiveTask(receivedBlock);
        }
    }

    void sendTask(uint8_t host, uint16_t port, BlockStatic* block, FunctionPointer0<void> callback)
    {
        if (host == LocalHost)
        {
            FunctionPointer2<void, uint16_t, SharedPointer<Block> > fp(receivedBlock);
            minar::Scheduler::postCallback(fp.bind(port, block));
            minar::Scheduler::postCallback(callback);
        }
        else if (host < MAX_TRANSPORT_CONNECTIONS)
        {
            transaction_t action;

            action.host = host;
            action.port = port;
            action.block = block;
            action.callback = callback;

            sendQueue.push(action);

            minar::Scheduler::postCallback(processQueue);
        }
    }

    void addListenerTask(uint8_t host, uint16_t port, FunctionPointer1<void, SharedPointer<Block> > callback)
    {
        (void) host;

        callbackMap[port] = callback;
    }

    void removeListenerTask(uint8_t host, uint16_t port, FunctionPointer1<void, SharedPointer<Block> > callback)
    {
        (void) host;

        callbackMap.erase(port);
    }
}

static void processQueue()
{
    if (sendQueue.size() > 0)
    {
        MessageCenter::transaction_t action = sendQueue.front();

        MessageCenterTransport* transport = transportMap[action.host];

        if (transport)
        {
            transport->sendTask(action.port, action.block, sendDone);
        }
    }
}

static void sendDone()
{
    MessageCenter::transaction_t action = sendQueue.front();

    minar::Scheduler::postCallback(action.callback);

    sendQueue.pop();

    minar::Scheduler::postCallback(processQueue);
}

static void receivedBlock(uint16_t port, SharedPointer<Block> block)
{
    if (callbackMap.count(port))
    {
        FunctionPointer1<void, SharedPointer<Block> > fp = callbackMap[port];

        minar::Scheduler::postCallback(fp.bind(block));
    }
    else
    {
        printf("MessageCenter: port not in use: %u\r\n", port);
    }

    minar::Scheduler::postCallback(processQueue);
}



