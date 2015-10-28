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


#define MAX_TRANSPORT_CONNECTIONS 1

static MessageCenterTransport* transportMap[MAX_TRANSPORT_CONNECTIONS] = {0};

static std::queue<MessageCenter::transaction_t> sendQueue;
static std::map<uint16_t, FunctionPointer1<void, BlockStatic> > callbackMap;

static void processQueueTask();
static void sendDoneTask();
static void receivedBlockTask(uint16_t port, SharedPointer<BlockStatic> block);
static void dispatchBlockCommand(uint16_t port, BlockStatic block);

namespace MessageCenter
{
    /*
        Add new transport link between local MCU and remote MCU.
        uint8_t host                      - Host ID, only local and 1 remote supported.
        MessageCenterTransport* transport - Object implementing the transport protocol.
    */
    void addTransportTask(uint8_t host, MessageCenterTransport* transport)
    {
        // check if there is still space to store transport
        if ((host < MAX_TRANSPORT_CONNECTIONS) && (transport))
        {
            transportMap[host] = transport;

            // register block handler
            transport->onReceiveTask(receivedBlockTask);
        }
    }

    /*
        Send block of data to specified MCU and port.
        uint8_t host                      - Host ID, only local and 1 remote supported.
        uint16_t port                     - Port to send to.
        BlockStatic& block                - Block to send.
        FunctionPointer0<void> callback   - Callback function when send is done.
    */
    void sendTask(uint8_t host, uint16_t port, BlockStatic& block, FunctionPointer0<void> callback)
    {
        // if host is local, bypass queue and send directly through dispatch
        if (host == LocalHost)
        {
            FunctionPointer2<void, uint16_t, BlockStatic> fp(dispatchBlockCommand);

            minar::Scheduler::postCallback(fp.bind(port, block));
            minar::Scheduler::postCallback(callback);
        }
        // if host is within bounds, push transaction to queue
        else if (host < MAX_TRANSPORT_CONNECTIONS)
        {
            transaction_t action;

            action.host = host;
            action.port = port;
            action.block = block;
            action.callback = callback;

            sendQueue.push(action);

            minar::Scheduler::postCallback(processQueueTask);
        }
    }

    /*
        Add function to be called when blocks are send to the given port on the given host.
        uint8_t host                                 - Host ID, only local host supported.
        uint16_t port                                - Port to listen to.
        FunctionPointer1<void, BlockStatic> callback - Callback for handling block.
    */
    void addListenerTask(uint8_t host, uint16_t port, FunctionPointer1<void, BlockStatic> callback)
    {
        (void) host;

        callbackMap[port] = callback;
    }

    /*
        Remove previously registrered callback function.
        uint8_t host                                 - Host ID, only local host supported.
        uint16_t port                                - Port registered to.
        FunctionPointer1<void, BlockStatic> callback - Callback to be removed.
    */
    void removeListenerTask(uint8_t host, uint16_t port, FunctionPointer1<void, BlockStatic> callback)
    {
        (void) host;
        (void) callback;

        callbackMap.erase(port);
    }
}

/*
    Process one transcation at a time from the queue.
*/
static void processQueueTask()
{
    // only process if queue is not empty
    if (sendQueue.size() > 0)
    {
        MessageCenter::transaction_t action = sendQueue.front();

        // transport map is off-by-one
        MessageCenterTransport* transport = transportMap[action.host];

        // check for NULL pointer before dereferencing, and sending
        if (transport)
        {
            transport->sendTask(action.port, action.block, sendDoneTask);
        }
    }
}

/*
    Cleanup after block is send. Remove transaction from queue.
*/
static void sendDoneTask()
{
    MessageCenter::transaction_t action = sendQueue.front();

    // schedule sendDone callback
    minar::Scheduler::postCallback(action.callback);

    sendQueue.pop();

    // schedule next transcation in queueu to be processed
    minar::Scheduler::postCallback(processQueueTask);
}

/*
    Block received from remote host.
*/
static void receivedBlockTask(uint16_t port, SharedPointer<BlockStatic> block)
{
    // get pointer, dereference, and call dispatchBlockCommand synchronously
    // so that the shared pointer stays in scope
    dispatchBlockCommand(port, *(block.get()));

    // process queue for any outstanding transactions
    minar::Scheduler::postCallback(processQueueTask);
}

/*
    Dispatch block to the proper port.
*/
static void dispatchBlockCommand(uint16_t port, BlockStatic block)
{
    if (callbackMap.count(port))
    {
        callbackMap[port].call(block);
    }
    else
    {
        printf("MessageCenter: port not in use: %u\r\n", port);
    }
}



