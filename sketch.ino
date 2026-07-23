#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>

#define DHT_PIN PA1
#define DHT_TYPE DHT22

#define LED_PIN PA5
#define BUTTON_PIN PB0

#define UART_TX PA2
#define UART_RX PA3

#define LCD_SDA PB7
#define LCD_SCL PB6

#define LCD_ADDRESS 0x27

#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE 0x04
#define LCD_RS 0x01

HardwareSerial Serial1(UART_RX, UART_TX);
DHT dht(DHT_PIN, DHT_TYPE);

volatile bool buttonPressed = false;
volatile uint32_t lastInterruptTime = 0;

uint32_t lastBlinkTime = 0;

enum LED_State
{
    LED_OFF,
    LED_ON,
    LED_BLINK
};

LED_State ledState = LED_BLINK;

struct SensorData
{
    uint8_t temperature;
    uint8_t humidity;
    uint8_t battery;
};

struct DataPacket
{
    uint8_t start;
    uint8_t temperature;
    uint8_t humidity;
    uint8_t battery;
    uint8_t crc;
    uint8_t end;
};

SensorData sensor;
DataPacket packet;

void lcd_write(uint8_t data)
{
    Wire.beginTransmission(LCD_ADDRESS);
    Wire.write(data | LCD_BACKLIGHT);
    Wire.endTransmission();
}

void lcd_pulse(uint8_t data)
{
    lcd_write(data | LCD_ENABLE);
    delayMicroseconds(1);
    lcd_write(data & ~LCD_ENABLE);
    delayMicroseconds(50);
}

void lcd_nibble(uint8_t nibble, uint8_t mode)
{
    uint8_t data = (nibble & 0xF0) | mode;
    lcd_write(data);
    lcd_pulse(data);
}

void lcd_byte(uint8_t value, uint8_t mode)
{
    lcd_nibble(value, mode);
    lcd_nibble(value << 4, mode);
}

void lcd_command(uint8_t command)
{
    lcd_byte(command, 0);
}

void lcd_data(uint8_t data)
{
    lcd_byte(data, LCD_RS);
}

void lcd_init()
{
    delay(50);

    lcd_nibble(0x30, 0);
    delay(5);

    lcd_nibble(0x30, 0);
    delayMicroseconds(150);

    lcd_nibble(0x30, 0);
    lcd_nibble(0x20, 0);

    lcd_command(0x28);
    lcd_command(0x0C);
    lcd_command(0x06);
    lcd_command(0x01);

    delay(2);
}

void lcd_clear()
{
    lcd_command(0x01);
    delay(2);
}

void lcd_set_cursor(uint8_t column, uint8_t row)
{
    uint8_t address = row ? 0x40 : 0x00;
    lcd_command(0x80 | (address + column));
}

void lcd_print(const char *text)
{
    while (*text)
    {
        lcd_data(*text++);
    }
}

void lcd_print_number(uint8_t value)
{
    if (value >= 100)
        lcd_data('0' + value / 100);

    if (value >= 10)
        lcd_data('0' + (value / 10) % 10);

    lcd_data('0' + value % 10);
}

uint8_t crc8(const uint8_t *data, uint8_t length)
{
    uint8_t crc = 0;

    while (length--)
    {
        crc ^= *data++;

        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }

    return crc;
}

void Button_ISR()
{
    uint32_t currentTime = millis();

    if (currentTime - lastInterruptTime > 200)
    {
        buttonPressed = true;
        lastInterruptTime = currentTime;
    }
}

void createPacket()
{
    uint8_t crcData[3];

    packet.start = 0xAA;
    packet.temperature = sensor.temperature;
    packet.humidity = sensor.humidity;
    packet.battery = sensor.battery;

    crcData[0] = packet.temperature;
    crcData[1] = packet.humidity;
    crcData[2] = packet.battery;

    packet.crc = crc8(crcData, 3);
    packet.end = 0x55;
}

void sendPacket()
{
    Serial1.write(packet.start);
    Serial1.write(packet.temperature);
    Serial1.write(packet.humidity);
    Serial1.write(packet.battery);
    Serial1.write(packet.crc);
    Serial1.write(packet.end);
}

void LED_Controller()
{
    switch (ledState)
    {
        case LED_OFF:
            digitalWrite(LED_PIN, LOW);
            break;

        case LED_ON:
            digitalWrite(LED_PIN, HIGH);
            break;

        case LED_BLINK:
            if (millis() - lastBlinkTime >= 500)
            {
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
                lastBlinkTime = millis();
            }
            break;
    }
}

void displaySensorData()
{
    lcd_clear();

    lcd_set_cursor(0, 0);
    lcd_print("T:");
    lcd_print_number(sensor.temperature);
    lcd_print("C");

    lcd_set_cursor(0, 1);
    lcd_print("H:");
    lcd_print_number(sensor.humidity);
    lcd_print("%");
}

void readSensor()
{
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature))
    {
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("DHT ERROR");
        delay(1000);
        return;
    }

    sensor.temperature = (uint8_t)temperature;
    sensor.humidity = (uint8_t)humidity;
    sensor.battery = 100;

    displaySensorData();

    createPacket();
    sendPacket();

    delay(500);
}

void enterLowPowerMode()
{
    lcd_clear();

    lcd_set_cursor(0, 0);
    lcd_print("ZZZ");

    lcd_set_cursor(0, 1);
    lcd_print("...");

    digitalWrite(LED_PIN, LOW);

    buttonPressed = false;

    delay(50);

    __asm volatile("dsb");
    __asm volatile("wfi");
    __asm volatile("isb");
}

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    Serial1.begin(9600);

    Wire.setSDA(LCD_SDA);
    Wire.setSCL(LCD_SCL);
    Wire.begin();

    lcd_init();

    dht.begin();

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("SENSOR READY");

    attachInterrupt(
        digitalPinToInterrupt(BUTTON_PIN),
        Button_ISR,
        FALLING
    );

    delay(2000);
}

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
