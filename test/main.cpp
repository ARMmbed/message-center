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

// control debug output
#if 1
#define DEBUGOUT(...) { printf(__VA_ARGS__); }
#else
#define DEBUGOUT(...) /* nothing */
#endif // DEBUGOUT

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
#endif


#if defined(TARGET_LIKE_NRF51_GENERIC)
#include "message-center-transport/MessageCenterSPISlave.h"

const PinName MC_BUTTON1 = BUTTON1;
const PinName MC_BUTTON2 = BUTTON2;

static spi_slave_config_t spi_slave_config = {
    .pin_miso         = SPIS_MISO,
    .pin_mosi         = SPIS_MOSI,
    .pin_sck          = SPIS_SCK,
};
static MessageCenterSPISlave transport(spi_slave_config, SPIS_CSN, SPIS_IRQ);
#else
#include "message-center-transport/MessageCenterSPIMaster.h"

static SPI spi(MC_SPI_MOSI, MC_SPI_MISO, MC_SPI_SCLK);
static MessageCenterSPIMaster transport(spi, MC_SPI_SSEL, MC_SPI_IRQ);
#endif

static uint8_t buffer[100];
static BlockStatic block(buffer, sizeof(buffer));


// enable buttons to initiate transfer
static InterruptIn button1(MC_BUTTON1);
static InterruptIn button2(MC_BUTTON2);

// liveness led
static DigitalOut led1(LED1);
static Ticker ticker;




void receivedANCS(BlockStatic block)
{
    DEBUGOUT("main: ANCS: %p\r\n", block.getData());
    for (std::size_t idx = 0; idx < block.getLength(); idx++)
    {
        DEBUGOUT("%02X", block.at(idx));
    }
    DEBUGOUT("\r\n");
}

void receivedEquip(BlockStatic block)
{
    DEBUGOUT("main: Equip: %p\r\n", block.getData());
    for (std::size_t idx = 0; idx < block.getLength(); idx++)
    {
        DEBUGOUT("%02X", block.at(idx));
    }
    DEBUGOUT("\r\n");
}

/*****************************************************************************/
/* Buttons                                                                   */
/*****************************************************************************/

void sendDone()
{
    DEBUGOUT("send done\r\n");
}

void button1Task()
{
    DEBUGOUT("button 1\r\n");

    for (std::size_t idx = 0; idx < block.getMaxLength(); idx++)
    {
        block.at(idx) = idx;
    }

    MessageCenter::sendTask(MessageCenter::RemoteHost, MessageCenter::ANCSPort, block, sendDone);
}

void button1ISR()
{
    minar::Scheduler::postCallback(button1Task);
}


void button2Task()
{
    DEBUGOUT("button 2\r\n");

    for (std::size_t idx = 0; idx < block.getMaxLength(); idx++)
    {
        block.at(idx) = idx;
    }

    MessageCenter::sendTask(MessageCenter::RemoteHost, MessageCenter::EquipPort, block, sendDone);
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
    button1.mode(PullUp);
    button1.fall(button1ISR);
    button2.mode(PullUp);
    button2.fall(button2ISR);

    ticker.attach(ledISR, 1.0);

    MessageCenter::addTransportTask(MessageCenter::RemoteHost, &transport);

    MessageCenter::addListenerTask(MessageCenter::LocalHost, MessageCenter::ANCSPort, receivedANCS);
    MessageCenter::addListenerTask(MessageCenter::LocalHost, MessageCenter::EquipPort, receivedEquip);

    DEBUGOUT("Message Center: %s %s\r\n", __DATE__, __TIME__);
}
