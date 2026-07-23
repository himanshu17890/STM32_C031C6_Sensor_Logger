
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define DHT_PORT GPIOA
#define DHT_PIN GPIO_PIN_1

#define LED_PORT GPIOA
#define LED_PIN GPIO_PIN_5

#define BUTTON_PORT GPIOB
#define BUTTON_PIN GPIO_PIN_0

#define LCD_ADDRESS (0x27 << 1)

#define BUFFER_SIZE 32

typedef struct
{
    uint8_t temperature;
    uint8_t humidity;
    uint8_t battery;
} SensorData;

typedef struct
{
    uint8_t start;
    uint8_t temperature;
    uint8_t humidity;
    uint8_t battery;
    uint8_t crc;
    uint8_t end;
} DataPacket;

typedef struct
{
    uint8_t buffer[BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
} CircularBuffer;

typedef enum
{
    LED_OFF,
    LED_ON,
    LED_BLINK
} LED_State;

UART_HandleTypeDef huart2;
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim16;

CircularBuffer rxBuffer;

SensorData sensor;
DataPacket packet;

LED_State ledState = LED_OFF;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM16_Init(void);

void UART_Print(char *message);

uint8_t crc8(uint8_t *data, uint8_t len);

void Buffer_Init(CircularBuffer *cb);
uint8_t Buffer_IsEmpty(CircularBuffer *cb);
uint8_t Buffer_IsFull(CircularBuffer *cb);
uint8_t Buffer_Push(CircularBuffer *cb, uint8_t data);
uint8_t Buffer_Pop(CircularBuffer *cb, uint8_t *data);

void Create_Packet(
    SensorData *sensorData,
    DataPacket *dataPacket
);

void UART_Send_Packet(
    DataPacket *dataPacket
);

uint8_t Validate_Packet(
    DataPacket *dataPacket
);

void LED_Controller(void);

void LCD_Init(void);
void LCD_EnablePulse(uint8_t data);
void LCD_SendCommand(uint8_t command);
void LCD_SendData(uint8_t data);
void LCD_SetCursor(uint8_t row, uint8_t column);
void LCD_Print(char *string);
void LCD_Clear(void);

void I2C_Scan(void);

void delay_us(uint16_t us);

void DHT_Pin_Output(void);
void DHT_Pin_Input(void);

uint8_t DHT_WaitForState(
    GPIO_PinState state,
    uint16_t timeout
);

uint8_t DHT22_Read(
    uint8_t *humidity,
    uint8_t *temperature
);

uint8_t crc8(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;

    while (len--)
    {
        crc ^= *data++;

        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x80)
            {
                crc =
                    (crc << 1) ^ 0x07;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

void Buffer_Init(CircularBuffer *cb)
{
    cb->head = 0;
    cb->tail = 0;
}

uint8_t Buffer_IsEmpty(CircularBuffer *cb)
{
    return cb->head == cb->tail;
}

uint8_t Buffer_IsFull(CircularBuffer *cb)
{
    return
        ((cb->head + 1) % BUFFER_SIZE)
        == cb->tail;
}

uint8_t Buffer_Push(
    CircularBuffer *cb,
    uint8_t data
)
{
    if (Buffer_IsFull(cb))
    {
        return 0;
    }

    cb->buffer[cb->head] = data;

    cb->head =
        (cb->head + 1) %
        BUFFER_SIZE;

    return 1;
}

uint8_t Buffer_Pop(
    CircularBuffer *cb,
    uint8_t *data
)
{
    if (Buffer_IsEmpty(cb))
    {
        return 0;
    }

    *data =
        cb->buffer[cb->tail];

    cb->tail =
        (cb->tail + 1) %
        BUFFER_SIZE;

    return 1;
}

void Create_Packet(
    SensorData *sensorData,
    DataPacket *dataPacket
)
{
    uint8_t crcData[3];

    dataPacket->start =
        0xAA;

    dataPacket->temperature =
        sensorData->temperature;

    dataPacket->humidity =
        sensorData->humidity;

    dataPacket->battery =
        sensorData->battery;

    crcData[0] =
        dataPacket->temperature;

    crcData[1] =
        dataPacket->humidity;

    crcData[2] =
        dataPacket->battery;

    dataPacket->crc =
        crc8(
            crcData,
            3
        );

    dataPacket->end =
        0x55;
}

void UART_Send_Packet(
    DataPacket *dataPacket
)
{
    uint8_t packetData[6];

    packetData[0] =
        dataPacket->start;

    packetData[1] =
        dataPacket->temperature;

    packetData[2] =
        dataPacket->humidity;

    packetData[3] =
        dataPacket->battery;

    packetData[4] =
        dataPacket->crc;

    packetData[5] =
        dataPacket->end;

    HAL_UART_Transmit(
        &huart2,
        packetData,
        6,
        HAL_MAX_DELAY
    );

    char message[100];

    snprintf(
        message,
        sizeof(message),
        "\r\nPacket: %02X %02X %02X %02X %02X %02X\r\n",
        packetData[0],
        packetData[1],
        packetData[2],
        packetData[3],
        packetData[4],
        packetData[5]
    );

    UART_Print(message);
}

uint8_t Validate_Packet(
    DataPacket *dataPacket
)
{
    if (
        dataPacket->start !=
        0xAA
    )
    {
        return 0;
    }

    if (
        dataPacket->end !=
        0x55
    )
    {
        return 0;
    }

    uint8_t crcData[3];

    crcData[0] =
        dataPacket->temperature;

    crcData[1] =
        dataPacket->humidity;

    crcData[2] =
        dataPacket->battery;

    uint8_t calculatedCRC =
        crc8(
            crcData,
            3
        );

    if (
        calculatedCRC !=
        dataPacket->crc
    )
    {
        return 0;
    }

    return 1;
}

void LED_Controller(void)
{
    static uint32_t lastBlinkTime = 0;

    switch (ledState)
    {
        case LED_OFF:

            HAL_GPIO_WritePin(
                LED_PORT,
                LED_PIN,
                GPIO_PIN_RESET
            );

            break;

        case LED_ON:

            HAL_GPIO_WritePin(
                LED_PORT,
                LED_PIN,
                GPIO_PIN_SET
            );

            break;

        case LED_BLINK:

            if (
                HAL_GetTick() -
                lastBlinkTime >=
                500
            )
            {
                HAL_GPIO_TogglePin(
                    LED_PORT,
                    LED_PIN
                );

                lastBlinkTime =
                    HAL_GetTick();
            }

            break;
    }
}

void LCD_EnablePulse(uint8_t data)
{
    uint8_t value;

    value =
        data |
        0x08 |
        0x04;

    HAL_I2C_Master_Transmit(
        &hi2c1,
        LCD_ADDRESS,
        &value,
        1,
        100
    );

    HAL_Delay(1);

    value =
        data |
        0x08;

    HAL_I2C_Master_Transmit(
        &hi2c1,
        LCD_ADDRESS,
        &value,
        1,
        100
    );

    HAL_Delay(1);
}

void LCD_SendCommand(
    uint8_t command
)
{
    uint8_t high;
    uint8_t low;

    high =
        command & 0xF0;

    low =
        (command << 4) & 0xF0;

    LCD_EnablePulse(high);

    LCD_EnablePulse(low);
}

void LCD_SendData(
    uint8_t data
)
{
    uint8_t high;
    uint8_t low;

    high =
        (data & 0xF0) |
        0x01;

    low =
        ((data << 4) & 0xF0) |
        0x01;

    LCD_EnablePulse(high);

    LCD_EnablePulse(low);
}

void LCD_Init(void)
{
    HAL_Delay(50);

    LCD_EnablePulse(0x30);

    HAL_Delay(5);

    LCD_EnablePulse(0x30);

    HAL_Delay(1);

    LCD_EnablePulse(0x30);

    HAL_Delay(1);

    LCD_EnablePulse(0x20);

    HAL_Delay(1);

    LCD_SendCommand(0x28);

    LCD_SendCommand(0x0C);

    LCD_SendCommand(0x06);

    LCD_SendCommand(0x01);

    HAL_Delay(5);
}

void LCD_SetCursor(
    uint8_t row,
    uint8_t column
)
{
    uint8_t address;

    if (row == 0)
    {
        address =
            0x80 + column;
    }
    else
    {
        address =
            0xC0 + column;
    }

    LCD_SendCommand(
        address
    );
}

void LCD_Print(
    char *string
)
{
    while (*string)
    {
        LCD_SendData(
            (uint8_t)*string
        );

        string++;
    }
}

void LCD_Clear(void)
{
    LCD_SendCommand(
        0x01
    );

    HAL_Delay(2);
}

void I2C_Scan(void)
{
    char message[50];

    UART_Print(
        "\r\nI2C SCAN START\r\n"
    );

    for (
        uint8_t address = 1;
        address < 127;
        address++
    )
    {
        if (
            HAL_I2C_IsDeviceReady(
                &hi2c1,
                address << 1,
                2,
                100
            ) == HAL_OK
        )
        {
            snprintf(
                message,
                sizeof(message),
                "I2C DEVICE FOUND: 0x%02X\r\n",
                address
            );

            UART_Print(
                message
            );
        }
    }

    UART_Print(
        "I2C SCAN COMPLETE\r\n"
    );
}

void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(
        &htim16,
        0
    );

    while (
        __HAL_TIM_GET_COUNTER(
            &htim16
        ) < us
    )
    {
    }
}

void DHT_Pin_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct =
        {0};

    GPIO_InitStruct.Pin =
        DHT_PIN;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_OD;

    GPIO_InitStruct.Pull =
        GPIO_PULLUP;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        DHT_PORT,
        &GPIO_InitStruct
    );
}

void DHT_Pin_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct =
        {0};

    GPIO_InitStruct.Pin =
        DHT_PIN;

    GPIO_InitStruct.Mode =
        GPIO_MODE_INPUT;

    GPIO_InitStruct.Pull =
        GPIO_PULLUP;

    HAL_GPIO_Init(
        DHT_PORT,
        &GPIO_InitStruct
    );
}

uint8_t DHT_WaitForState(
    GPIO_PinState state,
    uint16_t timeout
)
{
    uint16_t start =
        __HAL_TIM_GET_COUNTER(
            &htim16
        );

    while (
        HAL_GPIO_ReadPin(
            DHT_PORT,
            DHT_PIN
        ) != state
    )
    {
        uint16_t current =
            __HAL_TIM_GET_COUNTER(
                &htim16
            );

        if (
            (uint16_t)
            (current - start)
            >= timeout
        )
        {
            return 0;
        }
    }

    return 1;
}

uint8_t DHT22_Read(
    uint8_t *humidity,
    uint8_t *temperature
)
{
    uint8_t data[5] = {0};

    DHT_Pin_Output();

    HAL_GPIO_WritePin(
        DHT_PORT,
        DHT_PIN,
        GPIO_PIN_RESET
    );

    HAL_Delay(2);

    HAL_GPIO_WritePin(
        DHT_PORT,
        DHT_PIN,
        GPIO_PIN_SET
    );

    delay_us(30);

    DHT_Pin_Input();

    if (!DHT_WaitForState(
            GPIO_PIN_RESET,
            100))
    {
        UART_Print(
            "DHT ERROR: NO RESPONSE LOW\r\n"
        );

        return 0;
    }

    if (!DHT_WaitForState(
            GPIO_PIN_SET,
            100))
    {
        UART_Print(
            "DHT ERROR: NO RESPONSE HIGH\r\n"
        );

        return 0;
    }

    if (!DHT_WaitForState(
            GPIO_PIN_RESET,
            100))
    {
        UART_Print(
            "DHT ERROR: NO RESPONSE LOW 2\r\n"
        );

        return 0;
    }

    for (uint8_t i = 0; i < 40; i++)
    {
        if (!DHT_WaitForState(
                GPIO_PIN_SET,
                100))
        {
            UART_Print(
                "DHT ERROR: DATA HIGH TIMEOUT\r\n"
            );

            return 0;
        }

        __HAL_TIM_SET_COUNTER(
            &htim16,
            0
        );

        if (!DHT_WaitForState(
                GPIO_PIN_RESET,
                100))
        {
            UART_Print(
                "DHT ERROR: DATA LOW TIMEOUT\r\n"
            );

            return 0;
        }

        uint16_t pulseWidth =
            __HAL_TIM_GET_COUNTER(
                &htim16
            );

        if (pulseWidth > 50)
        {
            data[i / 8] |=
                (1 << (7 - (i % 8)));
        }
    }

    uint8_t checksum =
        data[0] +
        data[1] +
        data[2] +
        data[3];

    if (checksum != data[4])
    {
        char message[100];

        snprintf(
            message,
            sizeof(message),
            "DHT RAW: %02X %02X %02X %02X %02X\r\n",
            data[0],
            data[1],
            data[2],
            data[3],
            data[4]
        );

        UART_Print(message);

        snprintf(
            message,
            sizeof(message),
            "DHT CALC CRC: %02X RECEIVED: %02X\r\n",
            checksum,
            data[4]
        );

        UART_Print(message);

        UART_Print(
            "DHT ERROR: CRC FAILED\r\n"
        );

        return 0;
    }

    uint16_t humidityRaw =
        ((uint16_t)data[0] << 8) |
        data[1];

    uint16_t temperatureRaw =
        ((uint16_t)data[2] << 8) |
        data[3];

    *humidity =
        humidityRaw / 10;

    if (temperatureRaw & 0x8000)
    {
        temperatureRaw &=
            0x7FFF;
    }

    *temperature =
        temperatureRaw / 10;

    return 1;
}
void UART_Print(
    char *message
)
{
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)message,
        strlen(message),
        HAL_MAX_DELAY
    );
}

static void MX_TIM16_Init(void)
{
    __HAL_RCC_TIM16_CLK_ENABLE();

    htim16.Instance =
        TIM16;

    htim16.Init.Prescaler =
        47;

    htim16.Init.CounterMode =
        TIM_COUNTERMODE_UP;

    htim16.Init.Period =
        65535;

    htim16.Init.ClockDivision =
        TIM_CLOCKDIVISION_DIV1;

    htim16.Init.RepetitionCounter =
        0;

    htim16.Init.AutoReloadPreload =
        TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(
        &htim16
    );
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct =
        {0};

    RCC_ClkInitTypeDef RCC_ClkInitStruct =
        {0};

    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSI;

    RCC_OscInitStruct.HSIState =
        RCC_HSI_ON;

    RCC_OscInitStruct.HSIDiv =
        RCC_HSI_DIV1;

    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;

    HAL_RCC_OscConfig(
        &RCC_OscInitStruct
    );

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_HSI;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(
        &RCC_ClkInitStruct,
        FLASH_LATENCY_1
    );
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct =
        {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(
        LED_PORT,
        LED_PIN,
        GPIO_PIN_RESET
    );

    GPIO_InitStruct.Pin =
        LED_PIN;

    GPIO_InitStruct.Mode =
        GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        LED_PORT,
        &GPIO_InitStruct
    );

    GPIO_InitStruct.Pin =
        BUTTON_PIN;

    GPIO_InitStruct.Mode =
        GPIO_MODE_INPUT;

    GPIO_InitStruct.Pull =
        GPIO_PULLUP;

    HAL_GPIO_Init(
        BUTTON_PORT,
        &GPIO_InitStruct
    );

    GPIO_InitStruct.Pin =
        DHT_PIN;

    GPIO_InitStruct.Mode =
        GPIO_MODE_INPUT;

    GPIO_InitStruct.Pull =
        GPIO_PULLUP;

    HAL_GPIO_Init(
        DHT_PORT,
        &GPIO_InitStruct
    );
}

static void MX_USART2_UART_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct =
        {0};

    GPIO_InitStruct.Pin =
        GPIO_PIN_2 |
        GPIO_PIN_3;

    GPIO_InitStruct.Mode =
        GPIO_MODE_AF_PP;

    GPIO_InitStruct.Pull =
        GPIO_PULLUP;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;

    GPIO_InitStruct.Alternate =
        GPIO_AF1_USART2;

    HAL_GPIO_Init(
        GPIOA,
        &GPIO_InitStruct
    );

    huart2.Instance =
        USART2;

    huart2.Init.BaudRate =
        9600;

    huart2.Init.WordLength =
        UART_WORDLENGTH_8B;

    huart2.Init.StopBits =
        UART_STOPBITS_1;

    huart2.Init.Parity =
        UART_PARITY_NONE;

    huart2.Init.Mode =
        UART_MODE_TX_RX;

    huart2.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;

    huart2.Init.OverSampling =
        UART_OVERSAMPLING_16;

    HAL_UART_Init(
        &huart2
    );
}

static void MX_I2C1_Init(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct =
        {0};

    GPIO_InitStruct.Pin =
        GPIO_PIN_6 |
        GPIO_PIN_7;

    GPIO_InitStruct.Mode =
        GPIO_MODE_AF_OD;

    GPIO_InitStruct.Pull =
        GPIO_PULLUP;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_HIGH;

    GPIO_InitStruct.Alternate =
        GPIO_AF6_I2C1;

    HAL_GPIO_Init(
        GPIOB,
        &GPIO_InitStruct
    );

    hi2c1.Instance =
        I2C1;

    hi2c1.Init.Timing =
        0x2000090E;

    hi2c1.Init.OwnAddress1 =
        0;

    hi2c1.Init.AddressingMode =
        I2C_ADDRESSINGMODE_7BIT;

    hi2c1.Init.DualAddressMode =
        I2C_DUALADDRESS_DISABLE;

    hi2c1.Init.OwnAddress2 =
        0;

    hi2c1.Init.GeneralCallMode =
        I2C_GENERALCALL_DISABLE;

    hi2c1.Init.NoStretchMode =
        I2C_NOSTRETCH_DISABLE;

    HAL_I2C_Init(
        &hi2c1
    );
}

int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();

    MX_USART2_UART_Init();

    MX_I2C1_Init();

    MX_TIM16_Init();

    HAL_TIM_Base_Start(
        &htim16
    );

    Buffer_Init(
        &rxBuffer
    );

    UART_Print(
        "\r\n===============================\r\n"
    );

    UART_Print(
        "STM32 C031C6 SENSOR LOGGER\r\n"
    );

    UART_Print(
        "===============================\r\n"
    );

    UART_Print(
        "SYSTEM STARTED\r\n"
    );

    I2C_Scan();

    LCD_Init();

    UART_Print(
        "LCD INIT DONE\r\n"
    );

    LCD_Clear();

    LCD_SetCursor(
        0,
        0
    );

    LCD_Print(
        "STM32 C031C6"
    );

    HAL_Delay(2000);

    while (1)
    {
        uint8_t humidity;
        uint8_t temperature;

        char message[100];

        UART_Print(
            "\r\nReading DHT22...\r\n"
        );

        if (
            DHT22_Read(
                &humidity,
                &temperature
            )
        )
        {
            sensor.temperature =
                temperature;

            sensor.humidity =
                humidity;

            sensor.battery =
                100;

            snprintf(
                message,
                sizeof(message),
                "Temperature: %d C\r\n",
                temperature
            );

            UART_Print(
                message
            );

            snprintf(
                message,
                sizeof(message),
                "Humidity: %d %%\r\n",
                humidity
            );

            UART_Print(
                message
            );

            LCD_Clear();

            LCD_SetCursor(
                0,
                0
            );

            snprintf(
                message,
                sizeof(message),
                "Temp: %d C",
                temperature
            );

            LCD_Print(
                message
            );

            LCD_SetCursor(
                1,
                0
            );

            snprintf(
                message,
                sizeof(message),
                "Humidity: %d %%",
                humidity
            );

            LCD_Print(
                message
            );

            Create_Packet(
                &sensor,
                &packet
            );

            if (
                Validate_Packet(
                    &packet
                )
            )
            {
                UART_Print(
                    "CRC: OK\r\n"
                );
            }
            else
            {
                UART_Print(
                    "CRC: ERROR\r\n"
                );
            }
        }
        else
        {
            UART_Print(
                "DHT22 READ ERROR\r\n"
            );

            LCD_Clear();

            LCD_SetCursor(
                0,
                0
            );

            LCD_Print(
                "DHT22 ERROR"
            );
        }

        LED_Controller();

        HAL_Delay(2000);
    }
}