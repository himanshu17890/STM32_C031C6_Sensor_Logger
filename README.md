# STM32C031C6 Low-Power Sensor Logger

A low-power embedded sensor logger built around the **STM32C031C6** microcontroller using the **Arduino framework**.

The project reads temperature and humidity from a **DHT22 sensor**, displays the measurements on a **16x2 I2C LCD**, transmits sensor data through **UART**, generates a **CRC-8 protected data packet**, controls an LED using a simple state machine, and demonstrates an interrupt-based wake-up mechanism with the ARM Cortex-M `WFI` instruction.

The system is designed around a simple low-power operating flow:

```text
                    +------------------+
                    |   STM32C031C6    |
                    |  Low-Power Logger |
                    +---------+--------+
                              |
              +---------------+---------------+
              |               |               |
              v               v               v
          +-------+       +--------+       +--------+
          | DHT22 |       | I2C LCD|       | UART   |
          |Sensor |       | 16x2   |       | Serial |
          +---+---+       +--------+       +--------+
              |
              v
        SensorData Structure
              |
              v
        DataPacket Creation
              |
              v
            CRC-8
              |
              v
        UART Transmission

              ^
              |
       Push Button Interrupt
              |
              v
        Wake From WFI Sleep
```

---

## Features

* STM32C031C6 microcontroller
* DHT22 temperature and humidity sensor
* 16x2 LCD with I2C backpack
* Manual I2C LCD driver
* UART communication at 9600 baud
* Interrupt-based push button wake-up
* Software button debounce
* ARM Cortex-M `WFI` low-power sleep instruction
* LED state machine
* LED OFF, ON, and BLINK states
* Non-blocking LED blinking using `millis()`
* Sensor data structures
* UART data packet creation
* CRC-8 calculation
* Packet start and end markers
* Low-power application flow
* Arduino framework compatibility
* STM32-specific pin configuration

---

# Hardware Requirements

| Component             | Description                     |
| --------------------- | ------------------------------- |
| STM32C031C6           | Main microcontroller            |
| DHT22                 | Temperature and humidity sensor |
| 16x2 LCD              | Character display               |
| I2C LCD Backpack      | PCF8574-based I2C interface     |
| LED                   | Status indicator                |
| Push Button           | Wake-up input                   |
| USB-to-UART Converter | Serial communication            |
| 4.7kΩ Resistor        | DHT22 DATA pull-up              |
| Jumper Wires          | Connections                     |
| Breadboard            | Prototyping                     |

---

# Pin Configuration

## DHT22

| DHT22 | STM32C031C6 |
| ----- | ----------- |
| VCC   | 3.3V        |
| DATA  | PA1         |
| GND   | GND         |

The DHT22 DATA line should have a suitable pull-up resistor connected to 3.3V.

The firmware configuration is:

```c
#define DHT_PIN PA1
#define DHT_TYPE DHT22
```

The project uses the Arduino `DHT` library to communicate with the sensor.

---

## Circuit Diagram

<img width="846" height="477" alt="Circuit Diagram" src="https://github.com/user-attachments/assets/c1727d8c-b8b5-4715-8803-dad8838f1260" />

---

# LED

| LED         | STM32C031C6 |
| ----------- | ----------- |
| LED Anode   | PA5         |
| LED Cathode | GND         |

The LED is controlled using GPIO output.

Configuration:

```c
#define LED_PIN PA5
```

The firmware implements three LED states:

```c
enum LED_State
{
    LED_OFF,
    LED_ON,
    LED_BLINK
};
```

The available states are:

* `LED_OFF`
* `LED_ON`
* `LED_BLINK`

During normal operation, the LED is turned ON while the sensor is being processed and then returns to the BLINK state.

When the system enters low-power mode, the LED is turned OFF.

---

# Push Button

| Button         | STM32C031C6 |
| -------------- | ----------- |
| Button         | PB0         |
| Other terminal | GND         |

The button uses the internal pull-up resistor:

```c
pinMode(BUTTON_PIN, INPUT_PULLUP);
```

Configuration:

```c
#define BUTTON_PIN PB0
```

The button is configured to generate an interrupt on a falling edge:

```c
attachInterrupt(
    digitalPinToInterrupt(BUTTON_PIN),
    Button_ISR,
    FALLING
);
```

When the button is pressed, the interrupt service routine sets:

```c
buttonPressed = true;
```

The main application detects this flag and wakes the system from low-power mode.

---

# I2C LCD

| LCD | STM32C031C6                       |
| --- | --------------------------------- |
| SDA | PB7                               |
| SCL | PB6                               |
| VCC | 3.3V / 5V depending on LCD module |
| GND | GND                               |

The I2C pins are configured using:

```c
#define LCD_SDA PB7
#define LCD_SCL PB6
```

The LCD address is:

```c
#define LCD_ADDRESS 0x27
```

The I2C interface is initialized using:

```c
Wire.setSDA(LCD_SDA);
Wire.setSCL(LCD_SCL);
Wire.begin();
```

The LCD is controlled directly through the I2C backpack without using a dedicated LCD library.

The firmware implements:

```c
lcd_write()
lcd_pulse()
lcd_nibble()
lcd_byte()
lcd_command()
lcd_data()
lcd_init()
lcd_clear()
lcd_set_cursor()
lcd_print()
lcd_print_number()
```

The LCD operates in 4-bit mode.

---

# UART

| UART      | STM32C031C6 |
| --------- | ----------- |
| TX        | PA2         |
| RX        | PA3         |
| Baud Rate | 9600        |
| Data Bits | 8           |
| Stop Bits | 1           |
| Parity    | None        |

UART configuration:

```c
#define UART_TX PA2
#define UART_RX PA3

HardwareSerial Serial1(UART_RX, UART_TX);
```

The UART is initialized using:

```c
Serial1.begin(9600);
```

UART is used to transmit the sensor data packet.

The transmitted packet contains:

```text
Start
Temperature
Humidity
Battery
CRC
End
```

A serial terminal configured for:

```text
9600 8N1
```

can be used to monitor the transmitted data.

---

# System Operation

The system follows an interrupt-driven low-power operating model.

The main flow is:

```text
              System Startup
                    |
                    v
              Initialize GPIO
                    |
                    v
              Initialize UART
                    |
                    v
              Initialize I2C
                    |
                    v
              Initialize LCD
                    |
                    v
              Initialize DHT22
                    |
                    v
              Display SENSOR READY
                    |
                    v
             Wait 2 Seconds
                    |
                    v
          +---------------------+
          | Enter Low Power Mode|
          |       WFI           |
          +----------+----------+
                     |
                     |
             Push Button Press
                     |
                     v
              GPIO Interrupt
                     |
                     v
              Button_ISR()
                     |
                     v
             Set buttonPressed
                     |
                     v
               Wake from WFI
                     |
                     v
             LED = ON
                     |
                     v
             Read DHT22
                     |
                     v
             Display Data
                     |
                     v
            Create Data Packet
                     |
                     v
                CRC-8
                     |
                     v
             Send via UART
                     |
                     v
             LED = BLINK
                     |
                     v
           Wait Approximately 1s
                     |
                     v
          Enter Low Power Mode
```

---

# Low-Power Operation

The application uses the ARM Cortex-M `WFI` instruction:

```c
__asm volatile("dsb");
__asm volatile("wfi");
__asm volatile("isb");
```

`WFI` stands for:

```text
Wait For Interrupt
```

The microcontroller enters a low-power waiting state until an interrupt occurs.

In this project, the push button interrupt is used to wake the system.

Before entering low-power mode, the application:

1. Clears the LCD.
2. Displays `ZZZ` and `...`.
3. Turns OFF the LED.
4. Clears the button event flag.
5. Executes a data synchronization barrier.
6. Executes `WFI`.
7. Waits for an interrupt.
8. Resumes execution after the interrupt.

The LCD displays:

```text
ZZZ
...
```

while the system is in its low-power state.

---

# Button Interrupt and Debouncing

The push button generates an interrupt when pressed.

The interrupt handler is:

```c
void Button_ISR()
{
    uint32_t currentTime = millis();

    if (currentTime - lastInterruptTime > 200)
    {
        buttonPressed = true;
        lastInterruptTime = currentTime;
    }
}
```

A 200 ms software debounce interval is used.

This prevents multiple button interrupts caused by mechanical button bouncing.

The interrupt handler does not directly perform sensor reading or LCD operations.

Instead, it only sets:

```c
buttonPressed = true;
```

The main loop then handles the actual application processing.

This is a safer approach because lengthy operations such as LCD communication and sensor reading should generally not be performed inside an interrupt service routine.

---

# Sensor Data Structure

The sensor measurements are stored using:

```c
struct SensorData
{
    uint8_t temperature;
    uint8_t humidity;
    uint8_t battery;
};
```

The current sensor structure contains:

| Field       | Type      | Description        |
| ----------- | --------- | ------------------ |
| temperature | `uint8_t` | Temperature value  |
| humidity    | `uint8_t` | Relative humidity  |
| battery     | `uint8_t` | Battery percentage |

The current firmware sets:

```c
sensor.battery = 100;
```

The battery value is therefore currently a placeholder.

A future version can replace this with an ADC-based battery measurement.

---

# DHT22 Sensor Reading

The project uses the Arduino DHT library:

```c
#include <DHT.h>
```

The sensor is configured as:

```c
#define DHT_PIN PA1
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);
```

The sensor is initialized using:

```c
dht.begin();
```

Temperature is read using:

```c
float temperature = dht.readTemperature();
```

Humidity is read using:

```c
float humidity = dht.readHumidity();
```

The firmware checks for invalid readings:

```c
if (isnan(humidity) || isnan(temperature))
```

If the sensor reading fails, the LCD displays:

```text
DHT ERROR
```

The firmware then waits for approximately one second before returning from the sensor-reading function.

---

# Sensor Data Display

After a successful DHT22 reading, the LCD is cleared and updated.

The first row displays temperature:

```text
T:25C
```

The second row displays humidity:

```text
H:60%
```

The display is generated using:

```c
displaySensorData();
```

The current implementation stores temperature and humidity as `uint8_t`, so decimal values from the DHT22 are truncated.

For example:

```text
25.6°C -> 25°C
60.8%  -> 60%
```

If decimal precision is required, the `SensorData` structure can be changed to use `float`.

---

# Data Packet

The sensor data is transmitted using a structured packet.

```c
struct DataPacket
{
    uint8_t start;
    uint8_t temperature;
    uint8_t humidity;
    uint8_t battery;
    uint8_t crc;
    uint8_t end;
};
```

The packet format is:

```text
+-------+-------------+----------+---------+-----+-----+
| Start | Temperature | Humidity | Battery | CRC | End |
+-------+-------------+----------+---------+-----+-----+
|  AA   |     XX      |    XX    |   XX    | XX  | 55  |
+-------+-------------+----------+---------+-----+-----+
```

The packet contains six bytes.

| Field       |   Size | Value / Description |
| ----------- | -----: | ------------------- |
| Start       | 1 byte | `0xAA`              |
| Temperature | 1 byte | Temperature         |
| Humidity    | 1 byte | Relative humidity   |
| Battery     | 1 byte | Battery percentage  |
| CRC         | 1 byte | CRC-8               |
| End         | 1 byte | `0x55`              |

The packet is created using:

```c
createPacket();
```

---

# Packet Framing

The packet uses fixed start and end markers.

```text
Start Byte = 0xAA
End Byte   = 0x55
```

The complete packet structure is:

```text
AA TT HH BB CC 55
```

Where:

```text
AA = Start marker
TT = Temperature
HH = Humidity
BB = Battery
CC = CRC-8
55 = End marker
```

For example:

```text
AA 19 3C 64 XX 55
```

represents:

```text
Temperature = 25°C
Humidity    = 60%
Battery     = 100%
```

The CRC byte depends on the temperature, humidity, and battery values.

---

# CRC-8

The project implements a CRC-8 algorithm using polynomial:

```text
0x07
```

The CRC is calculated over three bytes:

```text
Temperature
Humidity
Battery
```

The CRC function is:

```c
uint8_t crc8(const uint8_t *data, uint8_t length)
```

The algorithm processes each byte and performs eight bit-level operations.

The CRC is generated in:

```c
createPacket();
```

The following data is passed to the CRC function:

```c
crcData[0] = packet.temperature;
crcData[1] = packet.humidity;
crcData[2] = packet.battery;

packet.crc = crc8(crcData, 3);
```

This provides basic error detection for the sensor data payload.

---

# UART Packet Transmission

The complete packet is transmitted byte-by-byte using:

```c
void sendPacket()
{
    Serial1.write(packet.start);
    Serial1.write(packet.temperature);
    Serial1.write(packet.humidity);
    Serial1.write(packet.battery);
    Serial1.write(packet.crc);
    Serial1.write(packet.end);
}
```

The transmitted data is binary rather than ASCII text.

Therefore, a standard serial monitor may display non-printable characters for some packet bytes.

For debugging, a hexadecimal representation of the packet can be added in a future version.

For example:

```text
Packet: AA 19 3C 64 XX 55
```

---

# LED State Machine

The LED control uses an enumeration:

```c
enum LED_State
{
    LED_OFF,
    LED_ON,
    LED_BLINK
};
```

The current state is stored in:

```c
LED_State ledState = LED_BLINK;
```

The LED controller handles each state.

## LED_OFF

```c
digitalWrite(LED_PIN, LOW);
```

The LED remains OFF.

## LED_ON

```c
digitalWrite(LED_PIN, HIGH);
```

The LED remains ON.

## LED_BLINK

The LED toggles every 500 ms:

```c
if (millis() - lastBlinkTime >= 500)
{
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlinkTime = millis();
}
```

The LED state machine provides a simple example of event-driven embedded application control.

---

# Main Loop

The main application loop is:

```c
void loop()
{
    if (!buttonPressed)
    {
        enterLowPowerMode();
    }

    if (buttonPressed)
    {
        noInterrupts();
        buttonPressed = false;
        interrupts();

        ledState = LED_ON;
        LED_Controller();

        readSensor();

        ledState = LED_BLINK;

        delay(1000);
    }
}
```

The application remains in low-power mode until the push button is pressed.

When the button is pressed:

```text
Button Press
    |
    v
Interrupt Generated
    |
    v
Button_ISR()
    |
    v
buttonPressed = true
    |
    v
Wake From WFI
    |
    v
LED ON
    |
    v
Read DHT22
    |
    v
Display Sensor Data
    |
    v
Create CRC Packet
    |
    v
Transmit UART Packet
    |
    v
LED BLINK
    |
    v
Return to Low Power
```

---

# Startup Sequence

During startup, the following peripherals are initialized:

```text
GPIO
  |
  v
UART
  |
  v
I2C
  |
  v
LCD
  |
  v
DHT22
  |
  v
Button Interrupt
```

The LCD initially displays:

```text
SENSOR READY
```

The system waits approximately two seconds and then enters low-power mode.

---

# Result

## Initial Stage

When the system starts, the LCD displays:

```text
SENSOR READY
```

<img width="857" height="497" alt="Initial System Stage" src="https://github.com/user-attachments/assets/410ee421-1a4e-465f-a28a-0e7e5592e654" />

---

## System Wakes After Pressing the Push Button

When the push button is pressed, the interrupt wakes the system.

The system then:

1. Turns the LED ON.
2. Reads the DHT22.
3. Displays temperature and humidity.
4. Creates the data packet.
5. Calculates the CRC-8.
6. Sends the packet through UART.

<img width="842" height="502" alt="System Wake-Up" src="https://github.com/user-attachments/assets/c56d8478-3000-4a79-8d5c-0d6028ab62e6" />

---

## System Enters Low-Power Mode

After processing the sensor data, the system returns to its low-power state.

The LED is turned OFF and the LCD displays:

```text
ZZZ
...
```

The Cortex-M `WFI` instruction is then executed.

<img width="875" height="482" alt="Low Power Mode" src="https://github.com/user-attachments/assets/d8c1ddd0-540a-448b-9552-49493f069d1a" />

---

# Example Sensor Data

A successful sensor reading may produce:

```text
Temperature: 25°C
Humidity: 60%
Battery: 100%
```

The LCD displays:

```text
T:25C
H:60%
```

The UART packet contains:

```text
AA 19 3C 64 XX 55
```

Where:

```text
AA = Start marker
19 = 25°C
3C = 60% humidity
64 = 100% battery
XX = CRC-8
55 = End marker
```

---

# Error Handling

If the DHT22 returns an invalid reading, the firmware detects it using:

```c
isnan()
```

The LCD displays:

```text
DHT ERROR
```

The sensor data is not updated or transmitted when an invalid reading is detected.

---

# Software Architecture

The application is divided into the following logical components:

```text
+----------------------------+
|        Main Application    |
+-------------+--------------+
              |
      +-------+-------+
      |               |
      v               v
 Button ISR       Low Power
      |               |
      v               v
 Wake Event          WFI
      |
      v
 Sensor Read
      |
      +--------+---------+
      |        |         |
      v        v         v
     DHT22    LCD      UART
               |         |
               |         v
               |     DataPacket
               |         |
               |         v
               |       CRC-8
               |         |
               |         v
               |      Transmit
               |
               v
          Sensor Display
```

---

# Project Configuration

Main pin configuration:

```c
#define DHT_PIN PA1
#define DHT_TYPE DHT22

#define LED_PIN PA5
#define BUTTON_PIN PB0

#define UART_TX PA2
#define UART_RX PA3

#define LCD_SDA PB7
#define LCD_SCL PB6

#define LCD_ADDRESS 0x27
```

UART configuration:

```text
Baud Rate: 9600
Data Bits: 8
Stop Bits: 1
Parity: None
```

---

# Libraries Used

The project uses:

```c
#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
```

### Arduino.h

Used for:

* GPIO control
* Timing functions
* Interrupts
* UART
* Core Arduino functionality

### Wire.h

Used for:

* I2C communication
* LCD communication

### DHT.h

Used for:

* DHT22 initialization
* Temperature measurement
* Humidity measurement

---

# Software Requirements

The project can be built using an Arduino-compatible STM32 development environment.

Required components:

* Arduino IDE or compatible Arduino build environment
* STM32 Arduino Core
* DHT sensor library
* Wire library

The STM32 board configuration should be set to the appropriate **STM32C031C6** target.

---

# How to Build and Run

## Step 1: Install the Required Software

Install:

* Arduino IDE
* STM32 Arduino Core
* DHT sensor library

## Step 2: Connect the Hardware

Connect the components according to the pin configuration.

## Step 3: Open the Project

Open the `.ino` source file in Arduino IDE.

## Step 4: Select the STM32 Board

Select the correct STM32C031C6 board configuration.

## Step 5: Build the Project

Compile the firmware.

## Step 6: Upload the Firmware

Upload the firmware to the STM32C031C6.

## Step 7: Open Serial Monitor

Configure the serial terminal for:

```text
Baud Rate: 9600
Data Bits: 8
Stop Bits: 1
Parity: None
```

## Step 8: Test the System

After startup:

1. The LCD displays `SENSOR READY`.
2. The system enters low-power mode.
3. Press the push button.
4. The system wakes up.
5. The LED turns ON.
6. The DHT22 is read.
7. The LCD displays temperature and humidity.
8. A CRC-protected packet is transmitted through UART.
9. The LED changes to BLINK mode.
10. The system returns to low-power mode.

---

# Troubleshooting

## LCD Not Working

Check:

* SDA connected to PB7
* SCL connected to PB6
* LCD power supply
* Common ground
* I2C address

The current code uses:

```c
#define LCD_ADDRESS 0x27
```

If your LCD backpack uses a different address, update this value.

---

## DHT22 Not Responding

Check:

* DHT22 power supply
* DATA connection to PA1
* Pull-up resistor
* Ground connection
* Sensor wiring

The DHT22 DATA line should have a suitable pull-up resistor.

---

## Incorrect Sensor Readings

Check:

* DHT22 wiring
* Sensor power supply
* Sensor stability
* Sensor sampling interval

The DHT22 should not be read too frequently.

The current application performs a delay after a sensor reading before returning to its low-power flow.

---

## No UART Data

Check:

* PA2 TX connection
* PA3 RX connection
* USB-to-UART converter
* Common ground
* Baud rate

Use:

```text
9600 8N1
```

Note that the application transmits the sensor packet as **binary data**, so some bytes may not appear as readable characters in a normal serial monitor.

---

## Button Does Not Wake the System

Check:

* Button connected to PB0
* Other button terminal connected to GND
* Internal pull-up configuration
* Interrupt configuration
* Correct STM32 pin mapping

The button is configured as:

```c
pinMode(BUTTON_PIN, INPUT_PULLUP);
```

and uses a falling-edge interrupt.

---

# Limitations

The current implementation has a few limitations:

* Temperature is stored as `uint8_t`, so decimal precision is lost.
* Humidity is stored as `uint8_t`, so decimal precision is lost.
* Battery level is fixed at `100` and is not measured using an ADC.
* The UART packet is transmitted as binary data.
* There is no UART packet receiver implemented.
* CRC generation is implemented, but packet reception and CRC verification are not included in the current application.
* The circular buffer is not included in the current code.
* No I2C scanner is included in the current application.
* The LCD uses blocking I2C communication.
* `delay()` is used during parts of the application flow.
* The low-power implementation uses `WFI`, but the project does not configure the full STM32 low-power peripheral modes such as STOP or STANDBY.
* The exact power consumption depends on the STM32 clock configuration and board hardware.

---

# Future Improvements

Possible future improvements include:

* Store temperature as `float` or signed integer format.
* Store humidity with decimal precision.
* Add ADC-based battery voltage measurement.
* Add UART receiver implementation.
* Add UART interrupt-based reception.
* Integrate a circular buffer for UART RX.
* Add packet parser and CRC validation on received packets.
* Add configurable packet commands.
* Add RTC timestamping.
* Store sensor readings in internal Flash.
* Add external EEPROM or SD card storage.
* Implement STM32 STOP mode for lower power consumption.
* Add RTC-based periodic wake-up.
* Add watchdog timer.
* Add sensor measurement error counters.
* Add UART packet debugging in hexadecimal format.
* Replace blocking delays with non-blocking state-machine logic.
* Add multiple sensor support.
* Add wireless communication using an external module.
* Implement a complete low-power data logger architecture.

---

# Learning Concepts Demonstrated

This project demonstrates practical embedded systems concepts including:

* Embedded C/C++ programming
* STM32 microcontroller programming
* Arduino framework on STM32
* GPIO configuration
* UART communication
* I2C communication
* DHT22 sensor interfacing
* LCD interfacing
* Interrupt handling
* External interrupt wake-up
* Software button debouncing
* Low-power embedded systems
* ARM Cortex-M `WFI`
* Structures
* Enumerations
* Pointers
* Arrays
* CRC-8
* Packet framing
* State machines
* Non-blocking LED control
* Sensor data processing
* Binary UART communication
* Modular embedded software design

---

# Project Status

**Status: Functional Prototype**

The current project demonstrates:

* DHT22 temperature and humidity acquisition
* 16x2 I2C LCD display
* UART communication
* CRC-8 packet generation
* Data packet framing
* LED state control
* Push-button interrupt handling
* Software debounce
* Low-power `WFI` sleep
* Interrupt-based system wake-up

The project can be further developed into a complete low-power environmental data logger by adding battery monitoring, RTC timestamps, non-volatile storage, UART packet reception, and deeper STM32 low-power modes.

---

# Author

**Himanshu Pawar**

Electronics & Telecommunication Engineering

**Areas of Interest:**

* Embedded Systems
* Firmware Development
* Embedded C/C++
* STM32
* Microcontrollers
* UART
* I2C
* SPI
* Low-Power Embedded Systems

---

# License

This project is available for educational and personal use.

Feel free to modify and extend the project for learning, experimentation, and embedded systems development.
