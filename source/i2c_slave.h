// see: https://blog.avislab.com/stm32-i2c-slave_ru/

/*******************************************************************/

typedef enum
{
    I2CSLAVE_ADDR1 = 0x68,   // DS1307/DS32xx
    I2CSLAVE_ADDR2  =0x48,   // ADS1115
} I2CSLAVE_ADDR_t;

#define   I2C1_CLOCK_FRQ    400000 // I2C-Frq in Hz (400 kHz)
#define   I2C1_RAM_SIZE     0x100  // RAM Size in Byte (0...255)

typedef enum
{
    I2C1_MODE_WAITING,      // Waiting for commands
    I2C1_MODE_SLAVE_ADR_WR, // Received slave address (writing)
    I2C1_MODE_ADR_BYTE,     // Received ADR byte
    I2C1_MODE_DATA_BYTE_WR, // Data byte (writing)
    I2C1_MODE_SLAVE_ADR_RD, // Received slave address (to read)
    I2C1_MODE_DATA_BYTE_RD, // Data byte (to read)
} I2C1_MODE_t;

/*******************************************************************/

void I2C1_Slave_init(void);

/*******************************************************************/
