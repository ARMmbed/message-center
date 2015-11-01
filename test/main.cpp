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
#include "message-center-transport/MessageCenterSPIMaster.h"


#if defined(TARGET_LIKE_EFM32GG_STK)
const PinName MC_SPI_MOSI = PD0;
const PinName MC_SPI_MISO = PD1;
const PinName MC_SPI_SCLK = PD2;
const PinName MC_SPI_SSEL = PD3;
const PinName MC_SPI_IRQ  = PD4;
const PinName MC_BUTTON1 = BTN0;
const PinName MC_BUTTON2 = BTN1;
#elif defined(TARGET_LIKE_K64F)
const PinName MC_SPI_MOSI = YOTTA_CFG_HARDWARE_TEST_PINS_SPI_MOSI;
const PinName MC_SPI_MISO = YOTTA_CFG_HARDWARE_TEST_PINS_SPI_MISO;
const PinName MC_SPI_SCLK = YOTTA_CFG_HARDWARE_TEST_PINS_SPI_SCLK;
const PinName MC_SPI_SSEL = YOTTA_CFG_HARDWARE_TEST_PINS_SPI_SSEL;
const PinName MC_SPI_IRQ  = PTC4;
const PinName MC_BUTTON1 = SW2;
const PinName MC_BUTTON2 = SW3;
#elif defined(TARGET_LIKE_COFFEE_ODIN)
const PinName MC_SPI_MOSI = YOTTA_CFG_HARDWARE_PINS_SPI_MOSI;
const PinName MC_SPI_MISO = YOTTA_CFG_HARDWARE_PINS_SPI_MISO;
const PinName MC_SPI_SCLK = YOTTA_CFG_HARDWARE_PINS_SPI_SCLK;
const PinName MC_SPI_SSEL = YOTTA_CFG_HARDWARE_PINS_SPI_SSEL;
const PinName MC_SPI_IRQ  = YOTTA_CFG_HARDWARE_PINS_SPI_IRQ;
const PinName MC_BUTTON1 = YOTTA_CFG_HARDWARE_PINS_BUTTON1;
const PinName MC_BUTTON2 = YOTTA_CFG_HARDWARE_PINS_BUTTON2;
#else
const PinName MC_SPI_MOSI = YOTTA_CFG_HARDWARE_TEST_PINS_SPI_MOSI;
const PinName MC_SPI_MISO = YOTTA_CFG_HARDWARE_TEST_PINS_SPI_MISO;
const PinName MC_SPI_SCLK = YOTTA_CFG_HARDWARE_TEST_PINS_SPI_SCLK;
const PinName MC_SPI_SSEL = YOTTA_CFG_HARDWARE_TEST_PINS_SPI_SSEL;
const PinName MC_SPI_IRQ  = YOTTA_CFG_HARDWARE_TEST_PINS_SPI_IRQ;
#endif

static SPI spi(MC_SPI_MOSI, MC_SPI_MISO, MC_SPI_SCLK);
static MessageCenterSPIMaster transport(spi, MC_SPI_SSEL, MC_SPI_IRQ);

static uint8_t buffer[100];
static BlockStatic block(buffer, sizeof(buffer));


// enable buttons to initiate transfer
static InterruptIn button1(MC_BUTTON1);
static InterruptIn button2(MC_BUTTON2);

// liveness led
static DigitalOut led1(LED1);
static Ticker ticker;




void receivedBlock(BlockStatic block)
{
    printf("main: 0x0100: %p\r\n", block.getData());
    for (std::size_t idx = 0; idx < block.getLength(); idx++)
    {
        printf("%02X", block.at(idx));
    }
    printf("\r\n");
}


/*****************************************************************************/
/* Buttons                                                                   */
/*****************************************************************************/

void sendDone()
{
    printf("send done\r\n");
}

void button1Task()
{
    printf("button 1\r\n");

    for (std::size_t idx = 0; idx < block.getMaxLength(); idx++)
    {
        block.at(idx) = idx;
    }

    MessageCenter::sendTask(MessageCenter::RemoteHost, 0x0100, block, sendDone);
}

void button1ISR()
{
    minar::Scheduler::postCallback(button1Task);
}


void button2Task()
{
    printf("button 2\r\n");

    for (std::size_t idx = 0; idx < block.getMaxLength(); idx++)
    {
        block.at(idx) = idx;
    }

    MessageCenter::sendTask(MessageCenter::LocalHost, 0x0100, block, sendDone);
}

void button2ISR()
{
    minar::Scheduler::postCallback(button2Task);
}

/*****************************************************************************/
/* App start                                                                 */
/*****************************************************************************/

void ledISR()
{
    led1 = !led1;
}

void app_start(int, char *[])
{
    button1.fall(button1ISR);
    button2.fall(button2ISR);

    ticker.attach(ledISR, 1.0);

    MessageCenter::addTransportTask(MessageCenter::RemoteHost, &transport);

    MessageCenter::addListenerTask(MessageCenter::LocalHost, 0x0100, receivedBlock);

    printf("Message Center: %s %s\r\n", __DATE__, __TIME__);
}
