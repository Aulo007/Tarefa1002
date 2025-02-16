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

// Variáveis globais
static ssd1306_t display;
static volatile bool led_green_state = false; // variável para ficar alterando o estado do led verde ao apertar o botão do joystick
static volatile bool pwm_enabled = true;
static volatile uint8_t border_style = 0;
static volatile uint32_t last_button_time = 0;
const uint32_t DEBOUNCE_DELAY = 200000; // 200ms em microssegundos

void setup_pwm(uint gpio); // Função para configurar pinos com pwm

int main(void)
{
    // Inicialização e lógica do programa

    return 0;
}

// Configuração do PWM para o LED
void setup_pwm(uint gpio)
{
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f); // Define o divisor de clock para evitar flickering
    pwm_config_set_wrap(&config, 4095);   // Mesma resolução do ADC
    pwm_init(slice, &config, true);
}

// Configura o valor PWM para um LED
void set_pwm(uint gpio, uint16_t value)
{
    if (pwm_enabled)
        pwm_set_gpio_level(gpio, value);
    else
        pwm_set_gpio_level(gpio, 0);
}
