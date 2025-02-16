#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "bibliotecas/ssd1306.h"

// Definindo pinos do joystick
static const uint32_t VRY_PIN = 27;
static const uint32_t VRX_PIN = 26;
static const uint32_t SW_PIN = 22;

// Definindo pinos do Botão A
static const uint32_t BUTTON_A = 5;

// Definindo pinos dos Leds RGB
static const uint32_t LED_GREEN_PIN = 11;
static const uint32_t LED_BLUE_PIN = 12;
static const uint32_t LED_RED_PIN = 13;

// Definindo pinos do display
static const uint32_t I2C_SDA = 14;
static const uint32_t I2C_SCL = 15;

// Configurações do display
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define SQUARE_SIZE 8
#define I2C_ADDR 0x3C



int main(void)
{
    // Inicialização e lógica do programa




    return 0;
}
