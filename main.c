/**
 * @file main.c
 * @brief Sistema de medição de distância usando sensor HCSR04 com display OLED e comunicação Bluetooth via HC-05/06 (UART)
 * @details Este arquivo implementa um sistema completo de medição de distância que:
 *          - Utiliza sensor ultrassônico HCSR04 para medir distâncias
 *          - Exibe resultados em display OLED SSD1306 via I2C
 *          - Transmite dados via Bluetooth HC-05/06 usando UART
 *          - Controla interface através de botões físicos
 * @author Beatriz Moreira e Gustavo de Melo
 * @date 2025
 */

// Inclusão de bibliotecas padrão do C
#include <stdio.h>
#include <string.h>
#include <math.h>

// Inclusão de bibliotecas específicas do SDK da Raspberry Pi Pico
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "inc/ssd1306.h"

/**
 * @defgroup PinDefinitions Definições de Pinagem
 * @brief Definições dos pinos GPIO utilizados no sistema
 * @{
 */

/** @brief Pino SDA para comunicação I2C com display OLED */
#define I2C_SDA 2

/** @brief Pino SCL para comunicação I2C com display OLED */
#define I2C_SCL 3

/** @brief Pino do botão A para ativar medição de distância */
#define BUTTON_A_PIN 5

/** @brief Pino ECHO do sensor ultrassônico HCSR04 */
#define ECHO_PIN 14

/** @brief Pino TRIG do sensor ultrassônico HCSR04 */
#define TRIG_PIN 20

/** @brief Identificador da UART para comunicação Bluetooth */
#define UART_ID uart0

/** @brief Pino TX da UART para transmissão Bluetooth */
#define UART_TX_PIN 0

/** @brief Pino RX da UART para recepção Bluetooth */
#define UART_RX_PIN 1

/** @} */

/**
 * @defgroup GlobalVariables Variáveis Globais
 * @brief Variáveis de estado para controle de interface
 * @{
 */

/** @brief Flag de irq do botão A */
static volatile uint32_t flag_a = 0;

/** @} */

/**
 * @defgroup Functions Funções
 * @brief Implementação das funções do sistema
 * @{
 */

/**
 * @brief Mede a distância usando sensor ultrassônico HCSR04
 * @details Esta função implementa o protocolo de medição do sensor HCSR04:
 *          1. Envia um pulso de 10μs no pino TRIG
 *          2. Aguarda o pino ECHO subir (início da reflexão)
 *          3. Aguarda o pino ECHO descer (fim da reflexão)
 *          4. Calcula a distância baseada no tempo de eco
 * @return float Distância medida em centímetros
 * @note A velocidade do som é considerada como 343 m/s
 * @note Fórmula: distância = (tempo_eco_us / 58.0) cm
 * @warning Esta função é bloqueante e pode demorar até 30ms para retornar
 */
float measure_distance_cm() {
    // Pulso no TRIG - protocolo HCSR04 requer pulso de 10μs
    gpio_put(TRIG_PIN, 0);
    sleep_us(2);  // Garante que o sinal está baixo
    gpio_put(TRIG_PIN, 1);
    sleep_us(10);  // Pulso de 10μs conforme especificação
    gpio_put(TRIG_PIN, 0);

    // Espera ECHO subir - início da reflexão do ultrassom
    absolute_time_t start, end;
    while (gpio_get(ECHO_PIN) == 0) {
        start = get_absolute_time();  // Marca início do eco
    }
    while (gpio_get(ECHO_PIN) == 1) {
        end = get_absolute_time();  // Marca fim do eco
    }

    // Calcula duração do pulso em microssegundos
    int64_t pulse_us = absolute_time_diff_us(start, end);

    // Distancia em cm (velocidade do som = 343 m/s -> 29.1 us/cm ida e volta)
    // Fórmula: distância = (tempo_eco_us / 58.0) cm
    return (float)pulse_us / 58.0f;

}

/**
 * @brief ISR de GPIO: seta flag em borda de subida.
 * @param gpio   GPIO que gerou a IRQ.
 * @param events Máscara de eventos (ex.: GPIO_IRQ_EDGE_RISE).
 * @note Use 'volatile' em flag_a e trate no loop principal (fora da ISR).
 */
static volatile uint64_t last_irq_us = 0;
static void gpio_irq_cb(uint gpio, uint32_t events) {
    uint64_t now = time_us_64();
    if (now - last_irq_us < 50000) return;  // 50 ms
    last_irq_us = now;

    if (events & GPIO_IRQ_EDGE_RISE) {
        flag_a = 1; // Borda de subida
    }
}

/**
 * @brief Função principal do sistema
 * @details Esta função implementa o loop principal do sistema de medição de distância:
 *          - Inicializa todos os periféricos (I2C, UART, GPIO)
 *          - Configura o display OLED SSD1306
 *          - Configura o sensor HCSR04
 *          - Implementa loop infinito para:
 *            * Detectar pressionamento do botão A
 *            * Medir distância quando botão for pressionado
 *            * Exibir resultado no display OLED
 *            * Transmitir dados via Bluetooth
 * @return int Código de retorno (nunca retorna devido ao loop infinito)
 * @note O sistema entra em loop infinito após inicialização
 * @warning Esta função não retorna - o programa roda indefinidamente
 */
int main() {
    // Inicialização do sistema
    stdio_init_all();

    // Inicialização do I2C para comunicação com o display OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);  // Pull-up interno para estabilidade do sinal
    gpio_pull_up(I2C_SCL);  // Pull-up interno para estabilidade do sinal
    ssd1306_init();

    // Configuração da área de renderização para o display OLED
    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);
    uint8_t ssd[ssd1306_buffer_length];  // Buffer para dados do display

    // Inicialização do botão A
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);  // Configura como entrada
    gpio_pull_up(BUTTON_A_PIN);  // Pull-up interno para detecção de pressão
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_irq_cb); // Ativar irq do botão A

    // Inicialização do sensor HCSR04
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);  // Pino de saída para pulso de trigger
    gpio_put(TRIG_PIN, 0);  // Inicia com nível baixo

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);  // Pino de entrada para sinal de eco

    // Inicialização UART (Bluetooth HC-05)
    uart_init(UART_ID, 9600);  // Baud rate padrão para HC-05
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Loop principal
    while (true) {
        if (flag_a) {
            flag_a = 0;

            float dist = measure_distance_cm();

            // Escreve no display OLED  
            memset(ssd, 0, ssd1306_buffer_length);
            char msg[32];
            snprintf(msg, sizeof(msg), "Dist: %.1f cm", dist);
            ssd1306_draw_string(ssd, 0, 0, " HC-SR04");
            ssd1306_draw_string(ssd, 0, 16, msg);
            render_on_display(ssd, &frame_area);

            // Envia por bluetooth (HC-06)
            char bt_msg[64];
            snprintf(bt_msg, sizeof(bt_msg), "Dist: %.2f cm\r\n", dist);
            uart_puts(UART_ID, bt_msg);
        }
    }
    return 0;
}

/** @} */