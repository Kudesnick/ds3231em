#include <stdbool.h>

#include "i2c_slave.h"
#include "ds3231.h"

#include "RTE_Components.h"
#include CMSIS_device_header

/*******************************************************************/
I2C1_MODE_t i2c1_mode = I2C1_MODE_WAITING;

I2CSLAVE_ADDR_t curr_addr;
/*******************************************************************/

void I2C1_Slave_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    I2C_InitTypeDef  I2C_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* Configure I2C_EE pins: SCL and SDA */
    // GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
    // GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Configure the I2C event priority */
    NVIC_SetPriority(I2C1_EV_IRQn, 0);
    NVIC_EnableIRQ(I2C1_EV_IRQn);

    //Configure I2C error interrupt to have the higher priority.
    NVIC_SetPriority(I2C1_ER_IRQn, 0);
    NVIC_EnableIRQ(I2C1_ER_IRQn);

    /* I2C configuration */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;
    I2C_InitStructure.I2C_OwnAddress1 = I2CSLAVE_ADDR1 << 1;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = I2C1_CLOCK_FRQ;

    I2C_OwnAddress2Config(I2C1, I2CSLAVE_ADDR2 << 1);
    I2C_DualAddressCmd(I2C1, ENABLE);

    /* I2C Peripheral Enable */
    I2C_Cmd(I2C1, ENABLE);
    /* Apply I2C configuration after enabling it */
    I2C_Init(I2C1, &I2C_InitStructure);

    I2C_ITConfig(I2C1, I2C_IT_EVT, ENABLE); //Part of the STM32 I2C driver
    I2C_ITConfig(I2C1, I2C_IT_BUF, ENABLE);
    I2C_ITConfig(I2C1, I2C_IT_ERR, ENABLE); //Part of the STM32 I2C driver
}
/*******************************************************************/

/*******************************************************************/
static void I2C1_ClearFlag(void)
{
    // ADDR-Flag clear
    while ((I2C1->SR1 & I2C_SR1_ADDR) == I2C_SR1_ADDR)
    {
        I2C1->SR1;
        I2C1->SR2;
    }

    // STOPF Flag clear
    while ((I2C1->SR1 & I2C_SR1_STOPF) == I2C_SR1_STOPF)
    {
        I2C1->SR1;
        I2C1->CR1 |= 0x1;
    }
}
/*******************************************************************/

/*******************************************************************/
void I2C1_EV_IRQHandler(void)
{
    uint32_t event;
    uint8_t wert;

    // Reading last event
    event = I2C_GetLastEvent(I2C1);

    if (event == I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED)
    {
        curr_addr = I2CSLAVE_ADDR1;
        // Master has sent the slave address to send data to the slave
        i2c1_mode = I2C1_MODE_SLAVE_ADR_WR;
    }
    else if (event == I2C_EVENT_SLAVE_RECEIVER_SECONDADDRESS_MATCHED)
    {
        curr_addr = I2CSLAVE_ADDR2;
        // Master has sent the slave address to send data to the slave
        i2c1_mode = I2C1_MODE_SLAVE_ADR_WR;
    }
    else if (event == I2C_EVENT_SLAVE_BYTE_RECEIVED)
    {
        // Master has sent a byte to the slave
        wert = I2C_ReceiveData(I2C1);
        // Check address
        if (i2c1_mode == I2C1_MODE_SLAVE_ADR_WR)
        {
            i2c1_mode = I2C1_MODE_ADR_BYTE;
            // Set current ram address
            if (curr_addr == I2CSLAVE_ADDR1)
            {
                ds_set_addr(wert);
            }
            else if (curr_addr == I2CSLAVE_ADDR2)
            {
                // @todo insert function to second device
            }
        }
        else
        {
            i2c1_mode = I2C1_MODE_DATA_BYTE_WR;
            // Store data in RAM
            if (curr_addr == I2CSLAVE_ADDR1)
            {
                ds_write(wert);
            }
            else if (curr_addr == I2CSLAVE_ADDR2)
            {
                // @todo insert function to second device
            }
        }
    }
    else if (event == I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED)
    {
        curr_addr = I2CSLAVE_ADDR1;
        // Master has sent the slave address to read data from the slave
        i2c1_mode = I2C1_MODE_SLAVE_ADR_RD;
        // Read data from RAM
        wert = ds_read();
        // Send data to the master
        I2C_SendData(I2C1, wert);
    }
    else if (event == I2C_EVENT_SLAVE_TRANSMITTER_SECONDADDRESS_MATCHED)
    {
        curr_addr = I2CSLAVE_ADDR2;
        // Master has sent the slave address to read data from the slave
        i2c1_mode = I2C1_MODE_SLAVE_ADR_RD;
        // Read data from RAM
        // @todo insert function to second device
        wert = 0;
        // Send data to the master
        I2C_SendData(I2C1, wert);    
    }
    else if (event == I2C_EVENT_SLAVE_BYTE_TRANSMITTED)
    {
        // Master wants to read another byte of data from the slave
        i2c1_mode = I2C1_MODE_DATA_BYTE_RD;
        // Read data from RAM
        if (curr_addr == I2CSLAVE_ADDR1)
        {
            wert = ds_read();
        }
        else if (curr_addr == I2CSLAVE_ADDR2)
        {
            // @todo insert function to second device
            wert = 0;
        }
        // Send data to the master
        I2C_SendData(I2C1, wert);
    }
    else if (event | I2C_EVENT_SLAVE_STOP_DETECTED)
    {
        // Master has STOP sent
        I2C1_ClearFlag();
        i2c1_mode = I2C1_MODE_WAITING;
    }
}

/*******************************************************************/
void I2C1_ER_IRQHandler(void)
{
    if (I2C_GetITStatus(I2C1, I2C_IT_AF))
    {
        I2C_ClearITPendingBit(I2C1, I2C_IT_AF);
    }
}
/*******************************************************************/
