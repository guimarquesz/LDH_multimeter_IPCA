# LDH Multimeter IPCA

Projeto académico desenvolvido no âmbito da unidade curricular de **Laboratório e Desenvolvimento de Hardware (LDH)**, no IPCA.

O projeto consiste no desenvolvimento de um **multímetro digital com datalogger**, baseado no microcontrolador **ATmega328P**, com leitura de tensão, leitura de corrente, apresentação dos valores num display de 7 segmentos e envio dos dados através de comunicação série UART.

---

## Objetivo do Projeto

O objetivo principal deste projeto é desenvolver um sistema eletrónico capaz de:

* Medir tensão elétrica;
* Medir corrente elétrica;
* Apresentar o valor medido num display de 7 segmentos;
* Alternar entre modo tensão e modo corrente através de botão;
* Enviar valores instantâneos e médios pela porta série;
* Permitir configuração do número de amostras usadas no cálculo da média.

---

## Funcionamento Geral

O sistema utiliza o **ADC interno do ATmega328P** para ler sinais analógicos provenientes dos circuitos de medição.

Existem dois modos principais de funcionamento:

### Modo Tensão

No modo tensão, o sistema lê o sinal no canal **ADC0** e converte o valor digital numa leitura de tensão.

O valor é apresentado no display no formato:

```text
XX.X V
```

### Modo Corrente

No modo corrente, o sistema lê o sinal no canal **ADC1**, associado ao sensor de corrente **ACS712**.

A corrente é calculada através da tensão de saída do sensor, considerando a sensibilidade aproximada de:

```text
185 mV/A
```

Para reduzir leituras falsas causadas por ruído, valores inferiores a `0.05 A` são considerados como zero.

---

## Principais Funcionalidades

* Leitura de tensão através do ADC;
* Leitura de corrente com sensor ACS712;
* Display de 7 segmentos multiplexado;
* Atualização do display por interrupção do Timer1;
* Comunicação UART a 9600 baud;
* Envio de valores instantâneos e médios por serial;
* Botão físico para alternar entre tensão e corrente;
* LEDs indicadores do modo ativo;
* Média configurável por comandos enviados pela porta série.

---

## Comunicação Série

A comunicação série funciona a:

```text
9600 baud
```

O sistema envia mensagens com o valor instantâneo e o valor médio calculado.

Exemplo de saída:

```text
[V] Agora: 12.35 | Media(500): 12.31
[A] Agora: 1.24 | Media(500): 1.21
```

---

## Comandos Série

É possível alterar o número de amostras usadas para o cálculo da média através da porta série.

### Alterar média da tensão

```text
V500
```

Define 500 amostras para o cálculo da média em modo tensão.

### Alterar média da corrente

```text
A200
```

Define 200 amostras para o cálculo da média em modo corrente.

---

## Mapeamento de Pinos

| Função                     | Pino          |
| -------------------------- | ------------- |
| Segmentos A a F do display | PD2 a PD7     |
| Segmento G                 | PC2           |
| Ponto decimal              | PC3           |
| Controlo dos 3 dígitos     | PB0, PB1, PB2 |
| Botão de troca de modo     | PB3           |
| LED modo tensão            | PC4           |
| LED modo corrente          | PC5           |
| Entrada tensão             | ADC0          |
| Entrada corrente           | ADC1          |
| UART RX/TX                 | PD0 / PD1     |

---

## Componentes Principais

* ATmega328P;
* Display de 7 segmentos de 3 dígitos;
* Sensor de corrente ACS712;
* Circuito divisor de tensão;
* Botão de seleção de modo;
* LEDs indicadores;
* Interface UART/FTDI;
* PCB desenvolvida no âmbito do projeto.

---

## Estrutura do Código

O ficheiro principal do projeto é o `main`.

O código encontra-se organizado nas seguintes partes principais:

* Definição da frequência do microcontrolador;
* Configuração dos pinos de entrada e saída;
* Configuração da UART;
* Configuração do ADC;
* Configuração do Timer1;
* Multiplexagem do display por interrupção;
* Leitura e processamento dos valores analógicos;
* Cálculo de médias;
* Envio de dados pela porta série;
* Gestão do botão de troca de modo.

---

## Interrupção do Timer1

O Timer1 é utilizado para atualizar o display de 7 segmentos de forma periódica.

A interrupção ocorre aproximadamente a cada:

```text
2 ms
```

Esta abordagem permite refrescar os três dígitos do display de forma contínua, evitando cintilação visível e reduzindo o efeito de brilho fantasma.

---

## Leitura ADC

Para reduzir ruído nas medições, cada leitura do ADC é composta por uma média de 64 leituras rápidas.

Isto permite obter uma leitura mais estável antes da conversão para tensão ou corrente.

---

## Tecnologias Utilizadas

* Linguagem C/C++;
* AVR-GCC;
* PlatformIO;
* ATmega328P;
* UART;
* ADC;
* Interrupções;
* Timer1;
* Display multiplexado;
* Altium Designer para desenvolvimento da PCB.

---

## Como Compilar

O projeto pode ser compilado através do PlatformIO.

```bash
platformio run
```

---

## Como Enviar para a Placa

```bash
platformio run --target upload
```

---

## Monitor Série

Para visualizar os valores enviados pelo microcontrolador:

```bash
platformio device monitor --baud 9600
```

---

## Autor

**Guilherme Marques**
Projeto desenvolvido no âmbito académico no IPCA.

---

## Estado do Projeto

Projeto funcional, com leitura de tensão e corrente, visualização local em display e envio de dados por comunicação série.
