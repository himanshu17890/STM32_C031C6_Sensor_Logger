# STM32 C031C6 Sensor Logger

A complete embedded C sensor-logging project built around the **STM32C031C6** microcontroller. The project reads temperature and humidity data from a **DHT22 sensor**, displays the measurements on a **16x2 I2C LCD**, communicates through **UART**, validates sensor packets using **CRC-8**, and demonstrates a **circular buffer**, **LED state control**, **I2C device scanning**, and **microsecond timing**.

The project is designed to demonstrate practical embedded systems concepts including GPIO, UART, I2C, timers, sensor communication, packet framing, CRC validation, and modular C programming using the STM32 HAL library.

---

## Features

* DHT22 temperature and humidity sensor interface
* STM32C031C6 microcontroller
* 16x2 LCD with I2C interface
* UART communication at 9600 baud
* I2C bus scanning
* CRC-8 packet generation and validation
* Data packet creation with start/end markers
* Circular buffer implementation
* LED control using a state machine
* LED OFF, ON, and BLINK states
* Timer-based microsecond delay
* DHT22 timing-based data decoding
* Sensor data displayed on LCD
* Sensor data transmitted through UART
* DHT22 checksum verification
* Structured sensor data management
* STM32 HAL-based peripheral configuration

---

## Hardware Requirements

| Component        | Description                     |
| ---------------- | ------------------------------- |
| STM32C031C6      | Main microcontroller            |
| DHT22            | Temperature and humidity sensor |
| 16x2 LCD         | I2C-based character display     |
| I2C LCD Backpack | PCF8574-based interface         |
| LED              | Status indicator                |
| Push Button      | User input                      |
| USB-to-UART      | Serial communication            |
| 4.7kΩ Resistor   | DHT22 data line pull-up         |
| Jumper Wires     | Connections                     |
| Breadboard       | Prototyping                     |

---

## Pin Configuration

### DHT22

| DHT22 | STM32C031C6 |
| ----- | ----------- |
| VCC   | 3.3V        |
| DATA  | PA1         |
| GND   | GND         |

A pull-up resistor should be connected between the DHT22 DATA line and 3.3V.

### circuit diagram  

<img width="846" height="477" alt="image" src="https://github.com/user-attachments/assets/c1727d8c-b8b5-4715-8803-dad8838f1260" />


### result

### initial stage 
<img width="857" height="497" alt="image" src="https://github.com/user-attachments/assets/410ee421-1a4e-465f-a28a-0e7e5592e654" />

###  system wakes up after presseing the push button
<img width="842" height="502" alt="image" src="https://github.com/user-attachments/assets/c56d8478-3000-4a79-8d5c-0d6028ab62e6" />

### goes to sleep(enters low power mode ) 
<img width="875" height="482" alt="image" src="https://github.com/user-attachments/assets/d8c1ddd0-540a-448b-9552-49493f069d1a" />







### LED

| LED | STM32C031C6 |
| --- | ----------- |
| LED | PA5         |
| GND | GND         |

The LED is controlled using GPIO output.

---

### Push Button

| Button         | STM32C031C6 |
| -------------- | ----------- |
| Button         | PB0         |
| Other terminal | GND         |

The button input uses the internal pull-up resistor.

> The current application code initializes the button but does not yet use it for application control.

---

### I2C LCD

| LCD | STM32C031C6                       |
| --- | --------------------------------- |
| SDA | PB7                               |
| SCL | PB6                               |
| VCC | 3.3V / 5V depending on LCD module |
| GND | GND                               |

The LCD address is configured as:

```c
#define LCD_ADDRESS (0x27 << 1)
```

The code uses the STM32 HAL I2C API, which expects the 7-bit I2C address shifted left by one bit.

---

### UART

| UART      | STM32C031C6 |
| --------- | ----------- |
| USART2 TX | PA2         |
| USART2 RX | PA3         |
| Baud Rate | 9600        |
| Data Bits | 8           |
| Stop Bits | 1           |
| Parity    | None        |

UART is used for:

* System startup messages
* I2C scan results
* DHT22 sensor readings
* DHT22 error messages
* CRC status
* Packet information
* Debugging

---

## System Architecture

The basic data flow of the application is:

```text
             +------------------+
             |   STM32C031C6    |
             |   Microcontroller |
             +---------+--------+
                       |
          +------------+------------+
          |            |            |
          v            v            v
      +-------+    +-------+    +---------+
      | DHT22 |    |  I2C  |    | USART2  |
      |Sensor |    | LCD   |    | Debug   |
      +-------+    +-------+    +---------+
          |
          v
    SensorData
          |
          v
    DataPacket
          |
          v
       CRC-8
          |
          v
   Packet Validation
```

---

## Software Architecture

The project is divided into several logical modules.

### 1. Sensor Data Structure

The `SensorData` structure stores the sensor measurements.

```c
typedef struct
{
    uint8_t temperature;
    uint8_t humidity;
    uint8_t battery;
} SensorData;
```

The battery value is currently set to `100` as a placeholder.

---

### 2. Data Packet

The sensor data is converted into a structured packet.

```c
typedef struct
{
    uint8_t start;
    uint8_t temperature;
    uint8_t humidity;
    uint8_t battery;
    uint8_t crc;
    uint8_t end;
} DataPacket;
```

The packet format is:

```text
+-------+-------------+----------+---------+-----+-----+
| Start | Temperature | Humidity | Battery | CRC | End |
+-------+-------------+----------+---------+-----+-----+
|  AA   |     XX      |    XX    |   XX    | XX  | 55  |
+-------+-------------+----------+---------+-----+-----+
```

### Packet Fields

| Field       |   Size | Description        |
| ----------- | -----: | ------------------ |
| Start       | 1 byte | `0xAA`             |
| Temperature | 1 byte | Temperature value  |
| Humidity    | 1 byte | Humidity value     |
| Battery     | 1 byte | Battery percentage |
| CRC         | 1 byte | CRC-8              |
| End         | 1 byte | `0x55`             |

Total packet size:

```text
6 bytes
```

---

## CRC-8 Implementation

The project implements a CRC-8 algorithm using polynomial:

```text
0x07
```

The CRC is calculated over:

```text
Temperature
Humidity
Battery
```

The CRC calculation is performed by:

```c
uint8_t crc8(uint8_t *data, uint8_t len);
```

The generated CRC is stored in the packet.

The packet is later validated using:

```c
uint8_t Validate_Packet(DataPacket *dataPacket);
```

Validation checks:

1. Start byte is `0xAA`
2. End byte is `0x55`
3. Calculated CRC matches the received CRC

If all checks pass:

```text
CRC: OK
```

Otherwise:

```text
CRC: ERROR
```

---

## Circular Buffer

The project includes a circular buffer implementation.

```c
typedef struct
{
    uint8_t buffer[BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
} CircularBuffer;
```

The buffer size is:

```c
#define BUFFER_SIZE 32
```

The following functions are implemented:

```c
Buffer_Init()
Buffer_IsEmpty()
Buffer_IsFull()
Buffer_Push()
Buffer_Pop()
```

The circular buffer provides FIFO-style data storage.

### Buffer Operation

```text
              +-----------------------+
              |     Circular Buffer   |
              +-----------------------+
              | 00 | 01 | 02 | ... 31 |
              +-----------------------+
                 ^              ^
                 |              |
               Tail           Head
```

The current code initializes the circular buffer but does not yet connect it to UART interrupt or DMA reception.

It can be extended in the future for:

* UART interrupt reception
* UART DMA reception
* Command processing
* Asynchronous communication
* Non-blocking data handling

---

## DHT22 Communication

The DHT22 uses a single-wire communication protocol with strict timing requirements.

The project performs the following sequence:

```text
1. Configure DHT22 pin as output
2. Pull data line LOW
3. Wait for 2 ms
4. Release the data line
5. Configure pin as input
6. Wait for DHT22 response
7. Read 40 data bits
8. Convert bits into 5 bytes
9. Verify checksum
10. Extract temperature and humidity
```

The 40-bit DHT22 frame consists of:

```text
Byte 0: Humidity MSB
Byte 1: Humidity LSB
Byte 2: Temperature MSB
Byte 3: Temperature LSB
Byte 4: Checksum
```

Checksum:

```text
Checksum = Byte0 + Byte1 + Byte2 + Byte3
```

The checksum is compared with Byte 4.

If the checksum is valid, the sensor values are accepted.

---

## DHT22 Data Conversion

Humidity is calculated using:

```c
humidityRaw / 10
```

Temperature is calculated using:

```c
temperatureRaw / 10
```

The code also checks the sign bit for negative temperatures.

The current `SensorData` structure stores temperature and humidity as `uint8_t`, so the application currently displays integer values only.

---

## LCD Display

The project uses a standard 16x2 LCD connected through an I2C backpack.

The LCD operates in 4-bit mode.

The following functions are implemented:

```c
LCD_Init()
LCD_EnablePulse()
LCD_SendCommand()
LCD_SendData()
LCD_SetCursor()
LCD_Print()
LCD_Clear()
```

When a valid DHT22 reading is received, the LCD displays:

```text
Temp: 25 C
Humidity: 60 %
```

If the DHT22 reading fails:

```text
DHT22 ERROR
```

At startup, the LCD displays:

```text
STM32 C031C6
```

---

## I2C Scanner

The project contains an I2C bus scanner.

The scanner checks addresses from:

```text
0x01 to 0x7E
```

When a device responds, the address is printed through UART.

Example:

```text
I2C SCAN START
I2C DEVICE FOUND: 0x27
I2C SCAN COMPLETE
```

This is useful for debugging I2C devices and verifying the LCD address.

---

## LED State Machine

The project implements three LED states:

```c
typedef enum
{
    LED_OFF,
    LED_ON,
    LED_BLINK
} LED_State;
```

The states are:

### LED_OFF

The LED remains OFF.

### LED_ON

The LED remains ON.

### LED_BLINK

The LED toggles every 500 ms.

The LED controller uses:

```c
HAL_GetTick()
```

to control the blink timing without blocking delays.

This approach allows other application tasks to execute while the LED is blinking.

---

## Timer and Microsecond Delay

TIM16 is used to generate microsecond-level timing.

The timer is configured with:

```c
Prescaler = 47
```

This configuration is intended to provide approximately a 1 MHz timer counter when the timer clock is 48 MHz.

Therefore:

```text
1 timer count ≈ 1 microsecond
```

The project uses:

```c
delay_us()
```

for short timing operations.

The timer is also used to measure DHT22 pulse widths.

The DHT22 encodes bits based on the duration of the HIGH pulse.

The code determines whether a received bit is `0` or `1` by checking:

```c
if (pulseWidth > 50)
```

---

## Main Application Flow

The application starts with:

```text
HAL_Init()
        |
        v
SystemClock_Config()
        |
        v
GPIO Initialization
        |
        v
UART Initialization
        |
        v
I2C Initialization
        |
        v
TIM16 Initialization
        |
        v
Start TIM16
        |
        v
Initialize Circular Buffer
        |
        v
Print Startup Information
        |
        v
I2C Scan
        |
        v
LCD Initialization
        |
        v
Enter Main Loop
```

The main loop performs:

```text
Read DHT22
    |
    +---- Failed ----> Print Error
    |                  Display Error
    |
    +---- Success ---> Store Sensor Data
                       |
                       v
                  Print UART Data
                       |
                       v
                  Update LCD
                       |
                       v
                  Create Packet
                       |
                       v
                  Calculate CRC
                       |
                       v
                  Validate Packet
                       |
                       v
                  Print CRC Status
                       |
                       v
                  LED Controller
                       |
                       v
                  Wait 2 Seconds
```

---

## Example UART Output

A successful system startup may produce:

```text
===============================
STM32 C031C6 SENSOR LOGGER
===============================
SYSTEM STARTED

I2C SCAN START
I2C DEVICE FOUND: 0x27
I2C SCAN COMPLETE

LCD INIT DONE

Reading DHT22...

Temperature: 25 C
Humidity: 60 %
CRC: OK
```

A packet may look like:

```text
Packet: AA 19 3C 64 XX 55
```

Where:

```text
AA = Start Byte
19 = Temperature
3C = Humidity
64 = Battery
XX = CRC
55 = End Byte
```

---

## Error Handling

The project provides UART debugging messages for several failure conditions.

### DHT22 Response Timeout

```text
DHT ERROR: NO RESPONSE LOW
```

### DHT22 Response High Timeout

```text
DHT ERROR: NO RESPONSE HIGH
```

### DHT22 Second Response Timeout

```text
DHT ERROR: NO RESPONSE LOW 2
```

### DHT22 Data High Timeout

```text
DHT ERROR: DATA HIGH TIMEOUT
```

### DHT22 Data Low Timeout

```text
DHT ERROR: DATA LOW TIMEOUT
```

### DHT22 Checksum Failure

```text
DHT ERROR: CRC FAILED
```

### Application-Level DHT22 Error

```text
DHT22 READ ERROR
```

---

## Project Configuration

The main configuration values are:

```c
#define DHT_PORT GPIOA
#define DHT_PIN GPIO_PIN_1

#define LED_PORT GPIOA
#define LED_PIN GPIO_PIN_5

#define BUTTON_PORT GPIOB
#define BUTTON_PIN GPIO_PIN_0

#define LCD_ADDRESS (0x27 << 1)

#define BUFFER_SIZE 32
```

UART configuration:

```text
Baud Rate: 9600
Data Bits: 8
Stop Bits: 1
Parity: None
Flow Control: None
```

---

## STM32 HAL APIs Used

The project uses several STM32 HAL APIs, including:

```c
HAL_Init()
HAL_Delay()
HAL_GetTick()

HAL_GPIO_Init()
HAL_GPIO_WritePin()
HAL_GPIO_ReadPin()
HAL_GPIO_TogglePin()

HAL_UART_Init()
HAL_UART_Transmit()

HAL_I2C_Init()
HAL_I2C_Master_Transmit()
HAL_I2C_IsDeviceReady()

HAL_TIM_Base_Init()
HAL_TIM_Base_Start()
```

---

## Project Requirements

### Software

* STM32CubeIDE
* STM32 HAL Library
* ARM GCC Toolchain
* STM32CubeMX configuration support

### Hardware

* STM32C031C6 development board
* DHT22 sensor
* 16x2 I2C LCD
* USB-to-UART converter
* LED
* Push button
* Required resistors and wiring

---

## How to Build and Run

### Step 1: Clone the Repository

Clone the project repository to your local machine.

### Step 2: Open the Project

Open the project using STM32CubeIDE.

### Step 3: Check Peripheral Configuration

Verify:

* GPIO configuration
* USART2 configuration
* I2C1 configuration
* TIM16 configuration
* System clock configuration

### Step 4: Connect Hardware

Connect the DHT22, LCD, LED, and UART interface according to the pin configuration described above.

### Step 5: Build the Project

Build the project using STM32CubeIDE.

### Step 6: Flash the STM32

Program the firmware into the STM32C031C6 using an ST-Link debugger/programmer.

### Step 7: Open Serial Terminal

Open a serial terminal with:

```text
Baud Rate: 9600
Data Bits: 8
Parity: None
Stop Bits: 1
```

### Step 8: Reset the Board

After resetting the board, observe:

* Startup messages
* I2C scan results
* LCD initialization
* DHT22 sensor readings
* CRC validation results

---

## Troubleshooting

### LCD Not Working

Check:

* SDA connection to PB7
* SCL connection to PB6
* GND connection
* Power supply
* I2C address

Run the I2C scanner and check the UART output.

If the LCD address is different, modify:

```c
#define LCD_ADDRESS (0x27 << 1)
```

For example, if the detected 7-bit address is `0x3F`:

```c
#define LCD_ADDRESS (0x3F << 1)
```

---

### DHT22 Not Responding

Check:

* DHT22 power supply
* DATA connection to PA1
* Pull-up resistor
* Sensor wiring
* Sensor timing
* Ground connection

The DHT22 DATA line requires a suitable pull-up resistor.

---

### Incorrect DHT22 Readings

Check:

* Sensor power supply
* Timing configuration
* TIM16 clock configuration
* Timer prescaler
* DHT22 wiring
* Sensor stability

DHT22 sensors should generally not be read too frequently. The application currently waits approximately 2 seconds between readings.

---

### No UART Output

Check:

* PA2 TX connection
* PA3 RX connection
* UART baud rate
* USB-to-UART converter
* Common ground
* Terminal configuration

Use:

```text
9600 8N1
```

---

### CRC Error

Check:

* Packet data
* CRC calculation
* Packet field order
* Start byte
* End byte

The packet uses:

```text
Start = 0xAA
End   = 0x55
```

The CRC is calculated from:

```text
Temperature
Humidity
Battery
```

---

## Future Improvements

The project can be extended with:

* UART interrupt-based reception
* UART DMA
* Circular buffer integration with UART RX
* Button-controlled LED states
* Low-power sleep modes
* Real battery voltage measurement
* ADC-based battery monitoring
* RTC timestamping
* SD card data logging
* EEPROM/Flash data storage
* More robust DHT22 timing implementation
* Floating-point temperature and humidity values
* Negative temperature support
* Packet transmission over UART
* Packet receiver implementation
* State-machine-based application architecture
* Non-blocking LCD updates
* Error counters and diagnostics
* Watchdog timer
* FreeRTOS task integration
* Sensor data logging with timestamps

---

## Learning Concepts Demonstrated

This project demonstrates practical implementation of:

* Embedded C programming
* STM32 HAL
* GPIO configuration
* UART communication
* I2C communication
* I2C device scanning
* Timer configuration
* Microsecond timing
* DHT22 communication protocol
* Checksum validation
* CRC-8
* Data packet framing
* Structures
* Enumerations
* Pointers
* Arrays
* Circular buffers
* State machines
* Non-blocking LED control
* Embedded debugging
* Sensor data processing

---

## Project Status

**Status:** Functional prototype

The project currently demonstrates sensor acquisition, LCD display, UART debugging, CRC packet generation/validation, I2C scanning, and embedded software concepts.

The circular buffer and button are implemented as reusable components but are not yet integrated into the main application flow.

---

## Author

**Himanshu Pawar**

Electronics & Telecommunication Engineering
Embedded Systems | Firmware | STM32 | Embedded C

---

## License

This project is available for educational and personal use.

Feel free to modify and extend the project for learning, experimentation, and embedded systems development.
