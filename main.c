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

// Definindo pinos do ssd
static const uint32_t I2C_SDA = 14;
static const uint32_t I2C_SCL = 15;

// Definindo constantes para os parâmetros do I2C e do ssd
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// Variáveis globais
static ssd1306_t ssd;                         // Variável global para o ssd
static volatile bool led_green_state = false; // variável para ficar alterando o estado do led verde ao apertar o botão do joystick
static volatile bool pwm_enabled = true;
static volatile uint8_t border_style = 0;
static volatile uint32_t last_button_time = 0;
const uint32_t DEBOUNCE_DELAY = 200000; // 200ms em microssegundos

void setup_pwm(uint gpio);                               // Função para configurar pinos com pwm
void set_pwm(uint gpio, uint16_t value);                 // Função para setar valor do pwm
uint16_t map_joystick_to_pwm(uint16_t value);            // Função para mapear valores do joystick para PWM
static void gpio_irq_handle(uint gpio, uint32_t events); // Função para a interrupção

// Estrutura para armazenar os limites de cada estilo de borda
typedef struct
{
    uint8_t left;
    uint8_t right;
    uint8_t top;
    uint8_t bottom;
} BorderLimits;

// Array global para armazenar os limites de cada estilo
static BorderLimits border_limits[] = {
    {1, WIDTH - SQUARE_SIZE - 1, 1, HEIGHT - SQUARE_SIZE - 1}, // Estilo 0: Minimalista
    {4, WIDTH - SQUARE_SIZE - 4, 4, HEIGHT - SQUARE_SIZE - 4}, // Estilo 1: Moldura dupla
    {5, WIDTH - SQUARE_SIZE - 5, 5, HEIGHT - SQUARE_SIZE - 5}, // Estilo 2: Cantos estilizados
    {2, WIDTH - SQUARE_SIZE - 2, 2, HEIGHT - SQUARE_SIZE - 2}, // Estilo 3: Efeito 3D
    {1, WIDTH - SQUARE_SIZE - 1, 1, HEIGHT - SQUARE_SIZE - 1}, // Estilo 4: Cantos arredondados
    {6, WIDTH - SQUARE_SIZE - 6, 6, HEIGHT - SQUARE_SIZE - 6}  // Estilo 5: Efeito profundidade
};

// Função para aplicar limites baseados no estilo atual
void apply_square_limits(int16_t *square_x, int16_t *square_y, uint8_t style)
{
    BorderLimits limits = border_limits[style % 6];

    // Aplica os limites
    *square_x = (*square_x < limits.left) ? limits.left : (*square_x > limits.right) ? limits.right
                                                                                     : *square_x;

    *square_y = (*square_y < limits.top) ? limits.top : (*square_y > limits.bottom) ? limits.bottom
                                                                                    : *square_y;
}

int main(void)
{
    // Ativando comunição serial
    stdio_init_all();

    // Configuração do ADC
    adc_init();
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);

    // Inicializando 2 botões
    gpio_init(SW_PIN);
    gpio_init(BUTTON_A);

    // Configurando direção dos botões para entrada
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_A, GPIO_IN);

    // Ativando pull up interno para os botões.
    gpio_pull_up(SW_PIN);
    gpio_pull_up(BUTTON_A);

    // Configuração do PWM para LEDs
    setup_pwm(LED_RED_PIN);
    setup_pwm(LED_BLUE_PIN);
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);

    // Inicialização do I2C a 400 kHz
    i2c_init(I2C_PORT, 400 * 1000);

    // Configura os pinos GPIO para a função I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Configura o pino de dados para I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Configura o pino de clock para I2C
    gpio_pull_up(I2C_SDA);                     // Ativa o pull-up no pino de dados
    gpio_pull_up(I2C_SCL);                     // Ativa o pull-up no pino de clock

    // Inicialização e configuração do ssd SSD1306                                               // Cria a estrutura do ssd
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o ssd com as especificações fornecidas
    ssd1306_config(&ssd);                                         // Configura os parâmetros do ssd
    ssd1306_send_data(&ssd);                                      // Envia os dados iniciais de configuração para o ssd

    // Limpeza do ssd. O ssd inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false); // Preenche o ssd com o valor especificado (false = apagado)
    ssd1306_send_data(&ssd);   // Envia os dados de preenchimento para o ssd

    uint16_t adc_value_x;
    uint16_t adc_value_y;

    int16_t square_x;
    int16_t square_y;

    // Configuração da interrupção com o callback
    gpio_set_irq_enabled_with_callback(SW_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handle);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handle);

    while (true)
    {

        /*
        Explicação sobre os valores lidos do ADC, acontece que, ao usar o código exemplo, no qual configura o pino 26 como eixo x e 27 como eixo y, uma coisa me incomodou
        no caso, ele lia nessa configuração o x na direção vertical do joystick e y na honrizontal.
        */

        // Seleciona o ADC para eixo Y. O pino 26 como entrada analógica
        adc_select_input(0);
        adc_value_y = adc_read(); // Inverte o valor lido do eixo Y

        // Seleciona o ADC para eixo X. O pino 27 como entrada analógica
        adc_select_input(1);
        adc_value_x = adc_read(); // Inverte o valor lido do eixo X

        printf("Valor em y: %d \n", adc_value_y); // prints para poder debbugar o código
        printf("Valor em x: %d \n", adc_value_x); // prints para poder debbugar o código

        /*
        testes
        sleep_ms(200);

            for (int i = 0; i < 6; i++)
            {
                draw_border(&ssd, i);
                ssd1306_send_data(&ssd);
                sleep_ms(1000);
                ssd1306_fill(&ssd, false);
                ssd1306_send_data(&ssd);
            }

        */

        // Calcula a posição do quadrado no eixo X
        square_x = (WIDTH / 2 - SQUARE_SIZE / 2) + ((int32_t)(adc_value_x - 2048) * (WIDTH - SQUARE_SIZE)) / 4096;

        // Calcula a posição do quadrado no eixo Y (invertido(sinal de mensos) porque estava com erro.)
        square_y = (HEIGHT / 2 - SQUARE_SIZE / 2) - ((int32_t)(adc_value_y - 2048) * (HEIGHT - SQUARE_SIZE)) / 4096;

        // Limites do quadrado no eixo X e Y de forma dinâmica

        apply_square_limits(&square_x, &square_y, border_style);

        // Atualização do ssd
        ssd1306_fill(&ssd, false);
        draw_border(&ssd, border_style);
        draw_square(&ssd, square_x, square_y);
        ssd1306_send_data(&ssd);

        // configurando pwm pros leds.

        set_pwm(LED_BLUE_PIN, map_joystick_to_pwm(adc_value_y));
        set_pwm(LED_RED_PIN, map_joystick_to_pwm(adc_value_x));

        sleep_ms(20); // Pequeno delay para estabilidade
    }

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

// Função para mapear valores do joystick para PWM
uint16_t map_joystick_to_pwm(uint16_t value)
{
    int16_t centered = value - 2048;
    if (abs(centered) < 100)
        return 0; // Zona morta
    return (uint16_t)abs(centered);
}

static void gpio_irq_handle(uint gpio, uint32_t events)
{
    // Cria uma varíavel que pega o tempo atual do sistema em microsegundos.
    uint32_t current_time_us = to_us_since_boot(get_absolute_time());

    if (current_time_us - last_button_time > DEBOUNCE_DELAY)

    {
        last_button_time = current_time_us;

        if (gpio == SW_PIN)
        {
            led_green_state = !led_green_state;
            gpio_put(LED_GREEN_PIN, led_green_state);

            border_style++;
            border_style = (border_style >= 5) ? 0 : border_style;
        }

        else if (gpio == BUTTON_A)
        {
            pwm_enabled = !pwm_enabled; // Troca estado do pwm toda vez que for apertado
        }
    }
}
