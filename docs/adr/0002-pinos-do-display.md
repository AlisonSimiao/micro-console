# 0002 - Mapa de pinos do Display

- Status: Aceito
- Data: 2026-11-06

## Contexto

O ESP32 DevKit expõe o SPI nativo (VSPI) em GPIO 18 (SCK), 19 (MISO), 23 (MOSI) e 5 (CS). Os oito botões do console já ocupam 13, 14, 16, 17, 18, 19, 21 e 22. Usar o barramento nativo exigiria remapear botões.

Além disso, GPIO 25 e 26 são os dois canais de DAC do ESP32 e ficam reservados para o áudio futuro. GPIO 6–11 são usados pela flash e não podem ser usados.

O painel é write-only: o firmware não lê dados de volta, então MISO não é necessário. O RST do painel pode ficar desligado (`-1`), já que o painel é resetado no power-on e a simulação do Wokwi não expõe esse pino.

## Decisão

| Sinal | GPIO |
|---|---|
| SCK | 4 |
| MOSI | 23 |
| CS | 27 |
| DC | 32 |
| LED (Backlight) | 33 |
| MISO | -1 |
| RST | -1 |

GPIO 25 e 26 permanecem reservados para o áudio.

## Consequências

Positivas:

- os botões mantêm o mapa atual;
- 25/26 ficam livres para DAC;
- um pino a menos ocupado (RST) e nenhum para MISO.

Negativas:

- SCK em GPIO 4 passa pela matriz de GPIO, não pelo IO_MUX; o SPI fica limitado a cerca de 40 MHz, que é a frequência fixada;
- trocar o mapa de pinos exige atualizar `Config`, `data/config.ini`, `diagram.json` e este ADR;
- sem MISO, não há leitura de registradores do painel.

## Alternativas consideradas

- **VSPI nativo (18/19/23/5)**: conflita com UP e DOWN.
- **SPI por software**: mais lento e desnecessário.
- **RST em GPIO dedicado**: consome um pino sem benefício no MVP.
