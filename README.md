# Micro Console

Firmware para um console baseado em ESP32 DevKit (`esp32dev`), desenvolvido com PlatformIO e Arduino. O projeto está em desenvolvimento: atualmente inclui inicialização do console, configuração via LittleFS, HALs de input e display e uma tela de teste do painel.

## Recursos atuais

- Inicialização coordenada por `Boot`, com validação de conflitos entre GPIOs antes de configurar o hardware.
- Configuração do console em `data/config.ini`, carregada do LittleFS.
- Oito botões com `INPUT_PULLUP`, debounce de 25 ms e consulta de estado e bordas.
- Painel ILI9341 240x320 via SPI, com rotação, desenho imediato e brilho por PWM.
- Simulação do circuito no Wokwi.

Áudio, armazenamento de jogos, sistema e runtime JavaScript ainda não estão implementados.

## Hardware padrão

| Componente | Sinal | GPIO |
|---|---|---:|
| Botões | A / B / X / Y | 13 / 14 / 16 / 17 |
| Direcional | UP / DOWN / LEFT / RIGHT | 18 / 19 / 21 / 22 |
| Display ILI9341 | SCK / MOSI / CS / DC | 4 / 23 / 27 / 32 |
| Display ILI9341 | LED (backlight) | 33 |
| Display ILI9341 | MISO / RST | desligados (`-1`) |

Os botões são ligados entre o GPIO e GND; o firmware usa `INPUT_PULLUP`, então nível LOW significa pressionado. A entrada deve ser compatível com 3,3 V. O circuito do simulador e as conexões estão em [`diagram.json`](./diagram.json).

## Requisitos

- ESP32 DevKit com flash de 4 MB, ou o simulador Wokwi.
- PlatformIO Core e os pacotes da plataforma ESP32. No ambiente VS Code deste projeto, o executável está em `~/.platformio/penv/bin/pio`.
- Para gerar a imagem de simulação, também são necessários `esptool` e a instalação de pacotes PlatformIO usada pelo script.

## Compilar e gravar

Na raiz do repositório:

```bash
# Compilar
~/.platformio/penv/bin/pio run

# Gravar o firmware (detecta a porta serial)
~/.platformio/penv/bin/pio run -t upload

# Gravar data/ no LittleFS
~/.platformio/penv/bin/pio run -t uploadfs

# Monitor serial
~/.platformio/penv/bin/pio device monitor
```

O monitor serial usa 115200 baud. Grave o filesystem após alterar `data/config.ini`; a configuração é montada em `/config.ini` no dispositivo. Caso o arquivo não exista, o firmware informa o ocorrido e usa os valores padrão definidos nos módulos.

## Simular no Wokwi

O Wokwi usa a imagem completa da flash indicada em [`wokwi.toml`](./wokwi.toml), incluindo o firmware e o LittleFS. Gere ou atualize essa imagem depois de alterar o firmware ou `data/`:

```bash
./build.sh
```

O script compila o firmware e o filesystem e combina as imagens em `.pio/build/esp32/merged.bin`. Inicie então a simulação pelo Wokwi. A imagem combinada não é atualizada automaticamente.

## Configuração

Os valores de `data/config.ini` são pares `chave=valor`. Pinos, rotação e brilho podem ser alterados sem recompilar o firmware:

| Chave | Padrão | Descrição |
|---|---:|---|
| `input.a`, `input.b`, `input.x`, `input.y` | 13, 14, 16, 17 | GPIOs dos botões de ação |
| `input.up`, `input.down`, `input.left`, `input.right` | 18, 19, 21, 22 | GPIOs do direcional |
| `display.sck` | 4 | Clock SPI |
| `display.mosi` | 23 | Dados SPI para o display |
| `display.cs` | 27 | Chip select |
| `display.dc` | 32 | Data/command |
| `display.led` | 33 | Backlight; `-1` deixa o backlight sem controle por GPIO |
| `display.miso`, `display.rst` | `-1` | MISO e reset, não usados por padrão |
| `display.rotation` | 0 | Rotação de 0 a 3 |
| `display.brightness` | 255 | Brilho PWM de 0 a 255 |

Os pinos do display devem ser válidos para o ESP32 e não podem se repetir entre si. GPIOs 6–11 são reservados para a flash. GPIOs 34–39 são somente entrada e não servem para sinais de saída do display. Conflitos entre pinos de módulos diferentes também impedem a inicialização.

## Interfaces disponíveis

O loop principal chama `Console::update()`, que atualiza o input. A interface pública do console dá acesso ao input e ao display:

```cpp
console.input().isPressed("A");
console.input().wasPressed("A");
console.input().wasReleased("A");

console.display().clear(0x0000); // RGB565
console.display().fillRect(10, 10, 40, 30, 0xFFFF);
console.display().drawText(10, 50, "Ola", 0xFFFF, 1);
```

As consultas `wasPressed()` e `wasReleased()` representam bordas detectadas na atualização corrente; consulte-as após a atualização de input. A tela desenhada durante a inicialização é temporária e serve para verificar o display.

## Estrutura

```text
firmware/
├── main.cpp
├── boot/       # Orquestra a inicialização
├── config/     # Configuração em memória carregada do LittleFS
├── console/    # Composição do console e tela de teste
└── hal/
    ├── core/   # Contratos dos módulos e declarações de pinos
    ├── display/
    └── input/
data/
└── config.ini
docs/adr/       # Decisões de arquitetura
```

Os includes do firmware são relativos a `firmware/`, por exemplo `#include "hal/input/input.hpp"`.

## Verificação

Não há testes automatizados ou CI configurados atualmente. A verificação principal após mudanças é:

```bash
~/.platformio/penv/bin/pio run
```
