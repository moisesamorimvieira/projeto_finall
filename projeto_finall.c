/* NOME: MOISES AMORIM VIEIRA

MATRICULA tic370100277
*/



//CÓDIGO PARA REALIZAR A DEMONSTRAÇÃO NO DIAGRAM
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include <stdlib.h>
#include "ssd1306.h"
#include "font.h"
#include <ctype.h>
#include <string.h>

#define SERVO_PIN 22   // Pino do servo motor
#define LED_PIN 12     // Pino do LED
#define BUZZER_PIN 21  // Pino do buzzer
#define BUTTON_PIN 5   // Pino do botão (Botão A)
#define PWM_WRAP 20000 // Configuração do PWM
#define I2C_PORT i2c1  // Porta I2C
#define I2C_SDA 14     // Pino SDA do IC2
#define I2C_SCL 15     // Pino SCL do I2C
#define ENDERECO 0x3C  // Endereço do display OLED

#define ROWS 4
#define COLS 4

const uint8_t rowPins[ROWS] = {28, 27, 26, 20};
const uint8_t colPins[COLS] = {19, 18, 17, 16};
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

char senha[5] = "1234"; // Senha padrão
bool tranca_aberta = false; // Estado da tranca
bool led_state = false; // Estado do LED

// Configura o PWM para o servo motor
void setup_pwm() {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);  // Configura o pino do servo motor como PWM
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN); // Obtém o slice PWM correspondente ao pino

    pwm_set_clkdiv(slice, 125.0); // Configura o divisor de clock para o PWM
    pwm_set_wrap(slice, PWM_WRAP); // Configura o valor máximo para o contador PWM
    pwm_set_enabled(slice, true);  // Habilita o PWM
}

// Define a posição do servo motor
void set_servo_position(uint slice, uint16_t pulse_width_us) {
    uint duty_cycle = (pulse_width_us * PWM_WRAP) / 20000; // Calcula o ciclo de trabalho baseado no tempo do pulso
    pwm_set_gpio_level(SERVO_PIN, duty_cycle); // Ajusta o ciclo de trabalho para o servo
}

// Configura o LED como saída
void setup_led() {
    gpio_init(LED_PIN);          // Inicializa o pino do LED
    gpio_set_dir(LED_PIN, GPIO_OUT); // Define o pino do LED como saída
}

// Configura o botão como entrada com pull-up
void setup_button() {
    gpio_init(BUTTON_PIN);          // Inicializa o pino do botão
    gpio_set_dir(BUTTON_PIN, GPIO_IN); // Define o pino como entrada
    gpio_pull_up(BUTTON_PIN);       // Ativa o resistor pull-up interno
}

// Configura o buzzer como saída
void setup_buzzer() {
    gpio_init(BUZZER_PIN);         // Inicializa o pino do buzzer
    gpio_set_dir(BUZZER_PIN, GPIO_OUT); // Define o pino como saída
}

// Toca o buzzer com diferentes sons
void tocar_buzzer(int tipo_som) {
    int duracao = 100; // Duração padrão do bip

    if (tipo_som == 1) { // Bip curto
        duracao = 100;
    } else if (tipo_som == 2) { // Bip longo
        duracao = 500;
    }

    gpio_put(BUZZER_PIN, 1);  // Liga o buzzer
    sleep_ms(duracao);        // Espera pelo tempo determinado
    gpio_put(BUZZER_PIN, 0);  // Desliga o buzzer
}

// Exibe uma animação no display OLED
void exibir_animacao(ssd1306_t *ssd) {
    static bool cor = true;
    cor = !cor;  // Alterna entre as cores (preto ou branco)

    ssd1306_fill(ssd, !cor); // Preenche a tela com a cor alternada
    ssd1306_rect(ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo alternado
    ssd1306_draw_string(ssd, "TRANCA", 8, 10); // Exibe o texto "TRANCA"
    ssd1306_draw_string(ssd, "SEGURANCA", 20, 30); // Exibe o texto "SEGURANCA"
    ssd1306_draw_string(ssd, "EMBARCATECH", 15, 48); // Exibe o texto "EMBARCATECH"
    ssd1306_send_data(ssd);  // Envia os dados para o display OLED
}

// Verifica se a senha digitada está correta
bool verificar_senha(const char* senha_tentativa) {
    return strcmp(senha_tentativa, senha) == 0; // Compara a senha tentativa com a senha padrão
}

// Mostra a senha parcial digitada no display OLED
void mostrar_senha_no_oled(ssd1306_t *ssd, const char *senha_tentativa, int indice) {
    ssd1306_fill(ssd, false); // Limpa a tela
    ssd1306_draw_string(ssd, "Digite a senha:", 20, 10); // Exibe o texto "Digite a senha"
    char senha_parcial[5] = {'\0'};  // Cria um array para armazenar a senha parcial
    strncpy(senha_parcial, senha_tentativa, indice); // Copia a senha digitada até o índice
    ssd1306_draw_string(ssd, senha_parcial, 50, 30); // Exibe a senha parcial
    ssd1306_send_data(ssd);  // Envia os dados para o display OLED
}

// Função para piscar o LED um número específico de vezes
void piscar_led(int vezes) {
    for (int i = 0; i < vezes; i++) {
        gpio_put(LED_PIN, 1);  // Liga o LED
        sleep_ms(200);         // Espera 200ms
        gpio_put(LED_PIN, 0);  // Desliga o LED
        sleep_ms(200);         // Espera 200ms
    }
}

// Inicialização do teclado
void setup_teclado() {
    // Configura as colunas como saída
    for (int col = 0; col < COLS; col++) {
        gpio_init(colPins[col]);
        gpio_set_dir(colPins[col], GPIO_OUT);
        gpio_put(colPins[col], 1);  // Deixa as colunas em nível alto por padrão
    }

    // Configura as linhas como entrada com pull-up
    for (int row = 0; row < ROWS; row++) {
        gpio_init(rowPins[row]);
        gpio_set_dir(rowPins[row], GPIO_IN);
        gpio_pull_up(rowPins[row]);  // Ativa o pull-up nas linhas
    }
}

// Função para ler o teclado matricial
char ler_teclado() {
    for (int col = 0; col < COLS; col++) {
        // Coloca a coluna em nível baixo para detectar a tecla pressionada
        gpio_put(colPins[col], 0);

        for (int row = 0; row < ROWS; row++) {
            // Verifica se a linha está em nível baixo (tecla pressionada)
            if (gpio_get(rowPins[row]) == 0) {
                // Espera até que o botão seja solto
                while (gpio_get(rowPins[row]) == 0);
                
                // Coloca novamente a coluna em nível alto
                gpio_put(colPins[col], 1);

                // Retorna o valor da tecla pressionada
                return keys[row][col];
            }
        }
        
        // Coloca a coluna de volta em nível alto
        gpio_put(colPins[col], 1);
    }
    return '\0';  // Retorna nulo se nenhuma tecla for pressionada
}

// Função para alterar a senha
void alterar_senha(ssd1306_t *ssd) {
    char nova_senha[5] = {'\0'};  // Array para armazenar a nova senha
    int indice = 0;
    printf("Digite a nova senha:\n");

    while (indice < 4) {
        char c = ler_teclado();  // Lê a tecla pressionada
        if (c != '\0') {
            nova_senha[indice] = c;
            printf("%c", c);  // Exibe a senha digitada no terminal
            mostrar_senha_no_oled(ssd, nova_senha, indice + 1);  // Atualiza a tela OLED
            indice++;
        }
        sleep_ms(100);
    }
    nova_senha[4] = '\0';  // Adiciona o caractere nulo para finalizar a string
    strcpy(senha, nova_senha); // Atualiza a senha com a nova
    printf("Senha alterada com sucesso!\n");
    tocar_buzzer(2);  // Emite um bip de confirmação
    ssd1306_fill(ssd, false);
    ssd1306_draw_string(ssd, "Senha Alterada!", 20, 30);
    ssd1306_send_data(ssd);  // Exibe a mensagem de confirmação no OLED
}

// Função para controlar a tranca eletrônica
void controlar_tranca(ssd1306_t *ssd) {
    char senha_tentativa[5] = {'\0'};  // Array para armazenar a senha tentada
    int indice = 0;

    printf("Digite a senha para abrir/fechar a tranca:\n");
    while (indice < 4) {
        char c = ler_teclado();  // Lê a tecla pressionada
        if (c != '\0') {
            senha_tentativa[indice] = c;
            printf("%c", c);
            mostrar_senha_no_oled(ssd, senha_tentativa, indice + 1);  // Mostra a senha parcial no OLED
            indice++;
        }
        sleep_ms(100);
    }
    senha_tentativa[4] = '\0';

    if (verificar_senha(senha_tentativa)) {
        printf("Senha correta!\n");

        // Se pressionar 'D' após a senha correta, entra no modo de alteração de senha
        if (senha_tentativa[3] == 'D') {
            printf("Alterar senha:\n");
            alterar_senha(ssd);  // Chama a função para alterar a senha
        } else {
            if (tranca_aberta) {
                set_servo_position(pwm_gpio_to_slice_num(SERVO_PIN), 1500);
                tocar_buzzer(2);
                printf("TRANCA FECHADA\n");
                ssd1306_fill(ssd, false);
                ssd1306_draw_string(ssd, "TRANCA FECHADA", 20, 30);
                ssd1306_send_data(ssd);
            } else {
                set_servo_position(pwm_gpio_to_slice_num(SERVO_PIN), 1000);
                tocar_buzzer(1);
                printf("TRANCA ABERTA\n");
                ssd1306_fill(ssd, false);
                ssd1306_draw_string(ssd, "TRANCA ABERTA", 20, 30);
                ssd1306_send_data(ssd);
            }
            tranca_aberta = !tranca_aberta;
            gpio_put(LED_PIN, 1);
            piscar_led(2);  // Pisca o LED como feedback visual
        }
    } else {
        printf("Senha incorreta!\n");
        tocar_buzzer(1);  // Emite um bip de erro
        ssd1306_fill(ssd, false);
        ssd1306_draw_string(ssd, "Senha Incorreta", 20, 30);
        ssd1306_send_data(ssd);
        piscar_led(5);  // Pisca o LED como sinal de erro
    }
    sleep_ms(2000);
}

int main() {
    stdio_init_all();  // Inicializa o sistema de entrada/saída (serial)
    sleep_ms(2000);    // Aguarda 2 segundos para garantir inicialização completa

    // Inicializa os periféricos
    setup_pwm();
    setup_led();
    setup_button();
    setup_buzzer();
    setup_teclado();

    // Configura a comunicação I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa e configura o display OLED
    ssd1306_t ssd;
    ssd1306_init(&ssd, 128, 64, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);

    while (true) {
        exibir_animacao(&ssd);  // Exibe animação inicial
        sleep_ms(2000);  // Aguarda 2 segundos
        controlar_tranca(&ssd);  // Controla a tranca
    }

    return 0;
}


// CÓDIGO PARA REALIZAR A DEMONSTRAÇÃO NA PLACA BITDOGLAB

/*#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include <stdlib.h>
#include "ssd1306.h"
#include "font.h"
#include <ctype.h>
#include <string.h>

#define SERVO_PIN 22   // Pino do servo motor
#define LED_PIN 12     // Pino do LED
#define BUZZER_PIN 21  // Pino do buzzer
#define BUTTON_PIN 5   // Pino do botão (Botão A)
#define PWM_WRAP 20000 // Configuração do PWM
#define I2C_PORT i2c1  // Porta I2C
#define I2C_SDA 14     // Pino SDA do I2C
#define I2C_SCL 15     // Pino SCL do I2C
#define ENDERECO 0x3C  // Endereço do display OLED

char senha[5] = "1234"; // Senha padrão
bool tranca_aberta = false; // Estado da tranca
bool led_state = false; // Estado do LED

// Configura o PWM para o servo motor
void setup_pwm() {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);

    pwm_set_clkdiv(slice, 125.0);
    pwm_set_wrap(slice, PWM_WRAP);
    pwm_set_enabled(slice, true);
}

// Define a posição do servo motor
void set_servo_position(uint slice, uint16_t pulse_width_us) {
    uint duty_cycle = (pulse_width_us * PWM_WRAP) / 20000;
    pwm_set_gpio_level(SERVO_PIN, duty_cycle);
}

// Configura o LED como saída
void setup_led() {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
}

// Configura o botão como entrada com pull-up
void setup_button() {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
}

// Configura o buzzer como saída
void setup_buzzer() {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
}

// Toca o buzzer com diferentes sons
void tocar_buzzer(int tipo_som) {
    int duracao = 100; // Duração padrão do bip

    if (tipo_som == 1) { // Bip curto
        duracao = 100;
    } else if (tipo_som == 2) { // Bip longo
        duracao = 500;
    }

    gpio_put(BUZZER_PIN, 1);
    sleep_ms(duracao);
    gpio_put(BUZZER_PIN, 0);
}

// Exibe uma animação no display OLED
void exibir_animacao(ssd1306_t *ssd) {
    static bool cor = true;
    cor = !cor;

    ssd1306_fill(ssd, !cor);
    ssd1306_rect(ssd, 3, 3, 122, 58, cor, !cor);
    ssd1306_draw_string(ssd, "TRANCA", 8, 10);
    ssd1306_draw_string(ssd, "SEGURANCA", 20, 30);
    ssd1306_draw_string(ssd, "EMBARCATECH", 15, 48);
    ssd1306_send_data(ssd);
}

// Verifica se a senha digitada está correta
bool verificar_senha(const char* senha_tentativa) {
    return strcmp(senha_tentativa, senha) == 0;
}

// Mostra a senha parcial digitada no display OLED
void mostrar_senha_no_oled(ssd1306_t *ssd, const char *senha_tentativa, int indice) {
    ssd1306_fill(ssd, false);
    ssd1306_draw_string(ssd, "Digite a senha:", 20, 10);
    char senha_parcial[5] = {'\0'};
    strncpy(senha_parcial, senha_tentativa, indice);
    ssd1306_draw_string(ssd, senha_parcial, 50, 30);
    ssd1306_send_data(ssd);
}

// Permite alterar a senha
void mudar_senha(ssd1306_t *ssd) {
    char nova_senha[5] = {'\0'};
    int indice = 0;

    printf("Digite a nova senha de 4 dígitos:\n");

    while (indice < 4) {
        char c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT && isdigit(c)) {
            nova_senha[indice] = c;
            printf("%c", c);
            mostrar_senha_no_oled(ssd, nova_senha, indice + 1);
            indice++;
        }
        sleep_ms(100);
    }
    strcpy(senha, nova_senha);
    printf("\nSenha alterada com sucesso!\n");
}

// Função para piscar o LED um número específico de vezes
void piscar_led(int vezes) {
    for (int i = 0; i < vezes; i++) {
        gpio_put(LED_PIN, 1);  // Liga o LED
        sleep_ms(200);         // Espera 200ms
        gpio_put(LED_PIN, 0);  // Desliga o LED
        sleep_ms(200);         // Espera 200ms
    }
}

// Controla a tranca eletrônica
void controlar_tranca(ssd1306_t *ssd) {
    char senha_tentativa[5] = {'\0'};
    int indice = 0;

    printf("Digite a senha para abrir/fechar a tranca:\n");
    while (indice < 4) {
        char c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT && isdigit(c)) {
            senha_tentativa[indice] = c;
            printf("%c", c);
            mostrar_senha_no_oled(ssd, senha_tentativa, indice + 1);
            indice++;
        }
        sleep_ms(100);
    }
    senha_tentativa[4] = '\0';

    if (verificar_senha(senha_tentativa)) {
        if (tranca_aberta) {
            printf("Senha correta! Fechando a tranca...\n");
            set_servo_position(pwm_gpio_to_slice_num(SERVO_PIN), 1500);
            tocar_buzzer(2);
            printf("TRANCA FECHADA\n");
            ssd1306_fill(ssd, false);
            ssd1306_draw_string(ssd, "TRANCA FECHADA", 20, 30);
            ssd1306_send_data(ssd);
        } else {
            printf("Senha correta! Abrindo a tranca...\n");
            set_servo_position(pwm_gpio_to_slice_num(SERVO_PIN), 1000);
            tocar_buzzer(1);
            printf("TRANCA ABERTA\n");
            ssd1306_fill(ssd, false);
            ssd1306_draw_string(ssd, "TRANCA ABERTA", 20, 30);
            ssd1306_send_data(ssd);
        }
        tranca_aberta = !tranca_aberta;
        gpio_put(LED_PIN, 1); // Acende o LED quando a senha estiver correta
        piscar_led(2); // Pisca o LED 2 vezes para senha correta
    } else {
        printf("SENHA INCORRETA!\n");
        tocar_buzzer(1);
        ssd1306_fill(ssd, false);
        ssd1306_draw_string(ssd, "Senha Incorreta", 20, 30);
        ssd1306_send_data(ssd);
        piscar_led(5); // Pisca o LED 5 vezes para senha incorreta
    }
    sleep_ms(2000); // Espera 2 segundos antes de limpar a mensagem
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    setup_pwm();
    setup_led();
    setup_button();
    setup_buzzer();

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);

    while (true) {
        exibir_animacao(&ssd);
        sleep_ms(2000);

        char modo = '\0';
        printf("Pressione '1' para inserir a senha ou '2' para alterar a senha. Pressione 'A' para ligar/desligar o LED:\n");

        // Espera até que o usuário pressione uma tecla válida
        while (modo != '1' && modo != '2') {
            if (gpio_get(BUTTON_PIN) == 0) {  // Verifica se o botão A foi pressionado
                led_state = !led_state;        // Alterna o estado do LED
                gpio_put(LED_PIN, led_state);  // Atualiza o estado do LED
                printf("LED %s\n", led_state ? "LIGADO" : "DESLIGADO");
                sleep_ms(500);  // Delay para debouncing do botão
            }
            modo = getchar_timeout_us(0);
            sleep_ms(100);
        }

        if (modo == '2') {
            mudar_senha(&ssd);
        } else {
            controlar_tranca(&ssd);
        }
    }
    return 0;
}*/



