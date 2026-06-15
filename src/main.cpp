#define F_CPU 16000000UL        // Define a frequência do relógio do ATmega328P (16MHz)
#include <avr/io.h>             // Biblioteca padrão para entrada/saída de pinos
#include <avr/interrupt.h>      // Biblioteca para manipulação de interrupções
#include <util/delay.h>         // Biblioteca para funções de atraso (delay)
#include <math.h>               // Biblioteca para funções matemáticas (fabsf)
#include <stdio.h>              // Biblioteca para entrada/saída formatada (sprintf)
#include <stdlib.h>             // Biblioteca padrão para conversões (atoi)

#define BAUDRATE 9600           // Define a velocidade da comunicação serial
#define MYUBRR ((F_CPU / (16UL * BAUDRATE)) - 1) // Cálculo do registo para o Baudrate

// Variáveis globais voláteis (usadas dentro e fora da interrupção)
volatile uint8_t d[3] = {0, 0, 0};      // Armazena os 3 dígitos a serem exibidos
volatile uint8_t digito_atual = 0;      // Controla qual dígito está a ser refrescado agora
volatile uint16_t amostras_V = 500;     // Número de amostras para média de Tensão
volatile uint16_t amostras_A = 500;     // Número de amostras para média de Corrente

// Mapeamento de bits para segmentos de um display Cátodo Comum (0-9)
uint8_t digitos_map[10] = {
    0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
    0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111
};

// --- INTERRUPÇÃO DO TIMER 1 (Executada automaticamente a cada 2ms para o Display) ---
ISR(TIMER1_COMPA_vect) {
    // Apaga os pinos PB0, PB1 e PB2 (desliga os 3 dígitos para evitar brilho fantasma)
    PORTB &= ~((1<<PB0)|(1<<PB1)|(1<<PB2)); 

    // Obtém o padrão de bits do dígito atual no array
    uint8_t valor = digitos_map[d[digito_atual]];
    
    // Atualiza PORTD (segmentos A a F nos pinos PD2 a PD7), mantendo PD0 e PD1 (UART)
    PORTD = (PORTD & 0x03) | ((valor & 0x3F) << 2);
    
    // Se o bit 6 (Segmento G) estiver ativo, liga PC2, senão desliga
    if ((valor >> 6) & 1) PORTC |= (1 << PC2); else PORTC &= ~(1 << PC2);
    
    // Liga o Ponto Decimal (PC3) apenas quando estamos no segundo dígito (PB1)
    if (digito_atual == 1) PORTC |= (1 << PC3); else PORTC &= ~(1 << PC3);

    // Lógica para ligar o dígito físico e supressão de zero à esquerda
    if (digito_atual == 0) {
        if (d[0] > 0) PORTB |= (1 << PB0); // Só liga o 1º dígito se o valor for >= 10.0
    } 
    else if (digito_atual == 1) {
        PORTB |= (1 << PB1); // Segundo dígito sempre liga (ex: 0.X)
    } 
    else if (digito_atual == 2) {
        PORTB |= (1 << PB2); // Terceiro dígito sempre liga
    }

    // Alterna para o próximo dígito (0 -> 1 -> 2 -> 0...)
    digito_atual = (digito_atual + 1) % 3;
}

// --- FUNÇÃO DE INICIALIZAÇÃO DA UART (Serial) ---
void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);    // Define a parte alta do baudrate
    UBRR0L = (unsigned char)ubrr;           // Define a parte baixa do baudrate
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);   // Ativa o receptor e o transmissor serial
    UCSR0C = (3 << UCSZ00);                 // Define formato: 8 bits de dados, 1 stop bit
}

// --- FUNÇÃO PARA ENVIAR TEXTO PELA SERIAL ---
void uart_print(char* s) {
    while (*s) {                            // Enquanto houver caracteres na string
        while (!(UCSR0A & (1 << UDRE0)));   // Espera o buffer de transmissão ficar vazio
        UDR0 = *s++;                        // Coloca o caractere no buffer para enviar
    }
}

// --- FUNÇÃO PARA PROCESSAR COMANDOS RECEBIDOS (Ex: V500 ou A200) ---
void processa_entrada_serial() {
    if (UCSR0A & (1 << RXC0)) {             // Se houver dados novos para ler
        char tipo = UDR0;                   // Lê o primeiro caractere (V ou A)
        _delay_ms(30);                      // Espera o resto dos dígitos chegarem
        char buf[10]; uint8_t i = 0;        // Buffer temporário para o número
        while ((UCSR0A & (1 << RXC0)) && i < 9) buf[i++] = UDR0; // Lê o valor numérico
        buf[i] = '\0';                      // Finaliza a string do buffer
        uint16_t val = atoi(buf);           // Converte o texto recebido em número inteiro
        if (val > 0) {
            if (tipo == 'V' || tipo == 'v') {
                amostras_V = val;           // Atualiza o limite de média da Tensão
                uart_print("\r\n[OK] Media Tensao atualizada.\r\n");
            } else if (tipo == 'A' || tipo == 'a') {
                amostras_A = val;           // Atualiza o limite de média da Corrente
                uart_print("\r\n[OK] Media Corrente atualizada.\r\n");
            }
        }
    }
}

// --- FUNÇÃO DE LEITURA DO ADC (Conversor Analógico-Digital) ---
uint16_t adc_leitura(uint8_t canal) {
    uint32_t soma = 0;                      // Variável para somar amostras
    ADMUX = (1 << REFS0) | (canal & 0x0F);  // Define referência 5V e o canal do pino
    _delay_us(50);                          // Tempo para estabilizar a tensão no pino
    for(uint8_t i=0; i<64; i++) {           // Faz 64 leituras rápidas para reduzir ruído
        ADCSRA |= (1 << ADSC);              // Inicia a conversão
        while (ADCSRA & (1 << ADSC));       // Espera terminar
        soma += ADC;                        // Acumula o valor lido
    }
    return (uint16_t)(soma / 64);           // Retorna a média das 64 leituras
}

// --- FUNÇÃO PRINCIPAL ---
int main(void) {
    // Configuração de Direção de Pinos (1=Saída, 0=Entrada)
    DDRD |= 0b11111100;                     // Pinos PD2 a PD7 como saída (segmentos)
    DDRC |= 0b00111100;                     // Pinos PC2 a PC5 como saída (seg G, ponto e LEDs)
    DDRB |= 0b00000111;                     // Pinos PB0 a PB2 como saída (controlo dígitos)
    PORTB |= (1 << PB3);                    // Ativa Pull-up interno no pino do Botão (PB3)

    // Configuração do Timer 1 para refrescar o Display
    TCCR1B |= (1 << WGM12) | (1 << CS11);   // Modo CTC, Prescaler de 8
    OCR1A = 3999;                           // Define teto para contar até 4000 (2ms)
    TIMSK1 |= (1 << OCIE1A);                // Ativa interrupção por comparação

    uart_init(MYUBRR);                      // Inicializa a porta Serial
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Ativa ADC com prescaler 128
    sei();                                  // Ativa as interrupções globais

    uint8_t modo = 0;                       // 0 = Tensão, 1 = Corrente
    uint8_t btn_last = 1;                   // Estado anterior do botão (pull-up = 1 solto)
    char buffer[128];                       // Buffer para criar as mensagens de texto
    float acum_serial = 0;                  // Acumulador para a média do Serial
    uint16_t count_serial = 0;              // Contador de amostras feitas para o Serial

    while (1) {                             // Loop infinito
        processa_entrada_serial();          // Verifica se o PC enviou comandos

        // Lógica de leitura do Botão (Debounce simples)
        uint8_t btn_now = (PINB & (1 << PB3)) != 0; 
        if (btn_last && !btn_now) {         // Se detectou clique (transição de 1 para 0)
            modo ^= 1;                      // Inverte o modo (V <-> A)
            acum_serial = 0;                // Reseta a média ao trocar de modo
            count_serial = 0;               // Reseta o contador
            _delay_ms(200);                 // Pausa para evitar múltiplos cliques
        }
        btn_last = btn_now;                 // Salva o estado atual para o próximo ciclo

        float valor_inst = 0;               // Armazena a leitura instantânea calculada
        uint16_t limite = (modo == 0) ? amostras_V : amostras_A; // Escolhe limite conforme o modo

        if (modo == 0) {                    // --- MODO TENSÃO ---
            PORTC |= (1 << PC4);            // Liga LED de Voltagem
            PORTC &= ~(1 << PC5);           // Desliga LED de Corrente
            valor_inst = adc_leitura(0) * 0.0299; // Converte bits para Tensão (ajusta se necessário)
        } else {                            // --- MODO CORRENTE ---
            PORTC |= (1 << PC5);            // Liga LED de Corrente
            PORTC &= ~(1 << PC4);           // Desliga LED de Voltagem
            uint16_t r = adc_leitura(1);    // Lê o canal ADC1
            valor_inst = fabsf(((r * 5.0 / 1024.0) - 2.5) / 0.185); // Cálculo para sensor ACS712
            if (valor_inst < 0.05) valor_inst = 0; // Filtro para ignorar pequenos ruídos
        }

        // --- ATUALIZAÇÃO DOS DADOS DO DISPLAY (Formato XX.X) ---
        uint16_t n = (uint16_t)(valor_inst * 10 + 0.5); // Arredonda para 1 casa decimal
        if (n > 999) n = 999;               // Limita ao máximo do display (99.9)
        d[0] = (n / 100) % 10;              // Dezena
        d[1] = (n / 10) % 10;               // Unidade
        d[2] = n % 10;                      // Decimal

        // --- CÁLCULO DA MÉDIA PARA O SERIAL ---
        acum_serial += valor_inst;          // Soma o valor atual ao acumulador
        count_serial++;                     // Incrementa contador de amostras

        if (count_serial >= limite) {       // Quando atingir o número de amostras definido
            float v_med = acum_serial / count_serial; // Calcula a média real
            
            // Divide floats em Parte Inteira e Decimal para o sprintf (ATmega gasta menos memória)
            int i_inst = (int)valor_inst; int d_inst = (int)((valor_inst - i_inst) * 100);
            int i_med = (int)v_med; int d_med = (int)((v_med - i_med) * 100);

            // Monta a mensagem final: [Modo] Valor Instantâneo | Valor Médio
            sprintf(buffer, "[%s] Agora: %d.%02d | Media(%u): %d.%02d\r\n", 
                    (modo == 0 ? "V" : "A"), i_inst, d_inst, limite, i_med, d_med);
            
            uart_print(buffer);             // Envia a mensagem para o Monitor Serial
            acum_serial = 0;                // Limpa acumulador para nova média
            count_serial = 0;               // Limpa contador para nova média
        }
        _delay_ms(1);                       // Pequena pausa para estabilidade do loop principal
    }
}