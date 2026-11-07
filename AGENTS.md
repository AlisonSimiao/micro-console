# AGENTS.md

Firmware do micro console baseado em ESP32 DevKit (`esp32dev`) usando PlatformIO e Arduino framework.

O projeto possui firmware próprio para hardware, sistema de configuração, módulos HAL e, futuramente, um runtime para execução de jogos JavaScript.

---

## Commands

O PlatformIO Core não está disponível no `PATH`. Use a versão instalada pelo ambiente do VSCode:

```bash
~/.platformio/penv/bin/pio run
```

Build completo:

```bash
~/.platformio/penv/bin/pio run
```

Upload para o ESP32:

```bash
~/.platformio/penv/bin/pio run -t upload
```

O `upload_port` não é definido; o PlatformIO deve detectar o ESP32 conectado.

Limpar build:

```bash
~/.platformio/penv/bin/pio run -t clean
```

### Verification

Atualmente não existem:

- testes automatizados;
- linter;
- CI.

A principal verificação é:

```bash
~/.platformio/penv/bin/pio run
```

Warnings devem ser avaliados e, quando possível, corrigidos.

O build atual leva aproximadamente 1 segundo.

### Simulação (Wokwi)

O simulador Wokwi carrega o arquivo definido em `wokwi.toml` e não usa `uploadfs`. Para que o LittleFS (`data/config.ini`) exista na simulação, o firmware e a imagem do filesystem são combinados em uma imagem completa da flash:

```bash
./build.sh
```

O script executa `pio run`, `pio run -t buildfs` e gera `.pio/build/esp32/merged.bin` com `esptool merge-bin` (LittleFS no offset `0x290000`, partição `spiffs` da tabela padrão).

`merged.bin` não é atualizado automaticamente: rode `./build.sh` após alterar o firmware ou `data/` antes de iniciar o simulador.

O circuito simulado fica em `diagram.json` (`board-esp32-devkit-c-v4`, flash de 4 MB, 8 botões ligados aos GPIOs abaixo e ao GND).

---

## Project Structure

O `platformio.ini` define:

```ini
src_dir = firmware
```

Portanto, todo código-fonte fica dentro de `firmware/`.

Estrutura atual/futura:

```text
firmware/
├── main.cpp
│
├── boot/
│   ├── boot.hpp
│   └── boot.cpp
│
├── config/
│   ├── config.hpp
│   └── config.cpp
│
├── hal/
│   ├── core/
│   │   ├── hal_module.hpp
│   │   └── hal_updatable.hpp
│   │
│   ├── input/
│   ├── display/
│   ├── audio/
│   └── storage/
│
├── system/
│   └── ...
│
└── runtime/
    └── ...
```

Nem todos os diretórios acima precisam existir enquanto não houver implementação correspondente.

### Includes

Os includes são relativos à raiz `firmware/`.

Use:

```cpp
#include "boot/boot.hpp"
#include "hal/core/hal_module.hpp"
```

Não use caminhos relativos como:

```cpp
#include "../boot/boot.hpp"
```

O `platformio.ini` inclui `firmware/` nos `build_flags`. Preserve essa convenção.

---

# Architecture

O firmware segue uma arquitetura em camadas, com responsabilidades bem definidas.

Fluxo conceitual:

```text
                    ┌──────────────┐
                    │     Boot     │
                    └──────┬───────┘
                           │
                    ┌──────▼───────┐
                    │    Config    │
                    └──────┬───────┘
                           │
          ┌────────────────┼────────────────┐
          │                │                │
          ▼                ▼                ▼
       Input            Display           Audio
          │                │                │
          └────────────────┼────────────────┘
                           ▼
                        System
                           │
                           ▼
                       Runtime
                           │
                           ▼
                        Game JS
```

A direção das dependências deve permanecer previsível.

### Responsabilidades

#### Boot

Orquestra a inicialização do console.

Pode:

- inicializar o `Config`;
- inicializar módulos HAL;
- verificar falhas de inicialização;
- inicializar serviços do sistema;
- preparar o runtime;
- apresentar diagnósticos de inicialização.

Não deve:

- implementar lógica de GPIO;
- implementar drivers de display;
- implementar reprodução de áudio;
- implementar lógica de input;
- executar gameplay;
- interpretar JavaScript;
- concentrar lógica de todos os subsistemas.

A regra principal é:

> **Boot prepara o sistema; os sistemas executam o sistema.**

---

#### Config

Responsável pela configuração persistente do console.

O `Config` deve:

- carregar configuração de um arquivo;
- armazenar valores em memória;
- fornecer acesso tipado;
- permitir alteração de valores;
- futuramente persistir alterações.

API conceitual:

```text
begin(file)

getInt(key)
getString(key)

setInt(key, value)
setString(key, value)
```

As chaves devem possuir namespaces estáveis:

```text
input.up
input.down
input.left
input.right

input.a
input.b
input.x
input.y

audio.pin
audio.volume

display.sck
display.mosi
display.cs
display.dc
display.led
display.miso
display.rst
display.rotation
display.brightness
```

O `Config` representa configuração do **console**.

Configurações específicas de um jogo pertencem ao pacote do jogo e não devem ser misturadas com a configuração global do console.

---

# HAL

`hal/` contém componentes responsáveis por interagir diretamente com o hardware.

Exemplos:

```text
Input
Display
Audio
Storage
```

Os HALs escondem detalhes específicos do ESP32 e dos periféricos.

Código de sistema e jogos não devem depender diretamente de GPIOs, registros ou APIs específicas de hardware quando uma abstração HAL apropriada existir.

---

## HAL lifecycle

Existem dois contratos separados.

### `hal_module`

Representa um módulo que precisa ser inicializado.

Responsabilidade:

```text
begin()
```

### `hal_updatable`

Representa um módulo que também precisa ser atualizado periodicamente.

Responsabilidade:

```text
begin()
update()
```

`update()` não deve ser obrigatório para todos os HALs.

Nem todo módulo precisa de processamento periódico.

Exemplo:

```text
Input
 ├── hal_module
 └── hal_updatable

Audio
 ├── hal_module
 └── hal_updatable

Display
 └── hal_module

Storage
 └── hal_module
```

Não adicione `update()` ao contrato base apenas para manter uma interface uniforme.

---

## HAL configuration

HALs podem receber `Config` durante sua inicialização.

Exemplo conceitual:

```text
Boot
 │
 ├── Config.begin()
 │
 └── HALs
      ├── Input(config)
      ├── Display(config)
      ├── Audio(config)
      └── Storage(config)
```

Cada HAL deve consultar apenas as configurações relevantes ao seu domínio.

Exemplo:

```text
Input   → input.*
Audio   → audio.*
Display → display.*
Storage → storage.*
```

Um HAL não deve alterar configurações pertencentes a outro módulo.

---

## Hardware configuration

Não use um `pin` genérico como requisito da classe base de todos os HALs.

Os módulos possuem necessidades diferentes:

```text
Input   → vários GPIOs
Audio   → um ou mais pinos
Display → vários pinos / SPI
Storage → vários pinos / interface
```

Portanto, a configuração de hardware deve ser específica de cada módulo.

Evite uma hierarquia baseada em algo como:

```text
HalModule(int pin)
```

quando o módulo não possui um único `pin`.

---

# Boot and HAL collection

`Boot` pode receber uma coleção de módulos HAL e inicializá-los em ordem.

Conceitualmente:

```text
Boot
 └── HAL[]
      ├── Input
      ├── Display
      ├── Audio
      └── Storage
```

O `Boot` deve apenas coordenar a inicialização.

A lógica de hardware continua pertencendo ao próprio HAL.

A ordem de inicialização deve ser explícita quando existir dependência entre módulos.

Por exemplo:

```text
Config
  ↓
Storage
  ↓
Display / Input / Audio
  ↓
System
  ↓
Runtime
```

Não introduza dependências implícitas entre HALs sem necessidade.

---

# System

`system/` contém funcionalidades de nível superior ao hardware.

Exemplos futuros:

```text
InputManager
AudioManager
GameManager
Settings
Filesystem
```

A diferença fundamental é:

```text
HAL
→ "como falar com o hardware"

System
→ "como o console utiliza esse hardware"
```

Por exemplo:

```text
Audio HAL
→ controla PWM/I2S/buzzer

Audio System
→ controla volume, efeitos, fila e comportamento de reprodução
```

Evite colocar lógica de negócio do sistema dentro dos HALs.

---

# Runtime

`runtime/` será responsável pela execução dos jogos.

A fronteira planejada é:

```text
Game JavaScript
       │
       ▼
 Game Runtime API
       │
       ▼
     System
       │
       ▼
      HAL
       │
       ▼
    Hardware
```

Jogos não devem acessar diretamente:

- GPIO;
- SPI;
- PWM;
- registradores do ESP32;
- drivers;
- objetos HAL internos.

O jogo deve utilizar APIs lógicas do console, por exemplo:

```text
input.isPressed("A")
audio.play("jump")
display.draw(...)
```

Os detalhes físicos permanecem dentro do firmware.

---

# Input

O console possui oito botões:

```text
A
B
X
Y

UP
DOWN
LEFT
RIGHT
```

Os jogos devem utilizar esses nomes lógicos e não números de GPIO.

### Current hardware

A implementação atual utiliza:

```text
A     → GPIO 13
B     → GPIO 14
X     → GPIO 16
Y     → GPIO 17

UP    → GPIO 18
DOWN  → GPIO 19
LEFT  → GPIO 21
RIGHT → GPIO 22
```

Esses GPIOs evitam os GPIOs 6–11, utilizados pela flash no ESP32 DevKit padrão.

Os botões utilizam:

```text
INPUT_PULLUP
```

Portanto:

```text
LOW  → pressionado
HIGH → não pressionado
```

A entrada deve utilizar níveis compatíveis com 3.3 V.

### Configuration

Os GPIOs são lidos do `Config` (`data/config.ini`) pelas chaves abaixo. Os valores de `Current hardware` são os defaults no código, usados quando a chave não existe:

```text
input.a
input.b
input.x
input.y
input.up
input.down
input.left
input.right
```

### Debounce e bordas

`Button::update()` aplica debounce de 25 ms antes de mudar o estado.

`Input` expõe:

```text
isPressed(key)    → botão mantido pressionado
wasPressed(key)   → pressionado nesta atualização
wasReleased(key)  → solto nesta atualização
```

`wasPressed()` e `wasReleased()` valem apenas até o próximo `Input::update()`. Leia-os no mesmo ciclo do loop, depois de `Input::update()`.

---

# Display

O console usa um Painel ILI9341 240x320 ligado por SPI, em modo imediato: cada chamada de desenho escreve direto no Painel, sem framebuffer nem `flush()`.

A biblioteca LovyanGFX fica confinada a `firmware/hal/display/display.cpp`. O `display.hpp` não inclui nem menciona tipos da biblioteca.

### Current hardware

```text
SCK   → GPIO 4
MOSI  → GPIO 23
CS    → GPIO 27
DC    → GPIO 32
LED   → GPIO 33 (Backlight)

MISO  → não usado (-1)
RST   → não usado (-1)
```

O SPI roda a 40 MHz. O barramento nativo do ESP32 não é usado porque GPIO 18 e 19 já pertencem ao Input. GPIO 25 e 26 permanecem reservados para o áudio.

### Configuration

Os pinos, a rotação e o Brilho vêm do `Config` (`data/config.ini`):

```text
display.sck
display.mosi
display.cs
display.dc
display.led
display.miso
display.rst
display.rotation
display.brightness
```

Defaults usados quando a chave não existe: `sck=4`, `mosi=23`, `cs=27`, `dc=32`, `led=33`, `miso=-1`, `rst=-1`, `rotation=0`, `brightness=255`.

`-1` significa "não usado": MISO desligado, RST desligado e LED desligado (Backlight fixo em 3V3). Pinos fora de 0–39, duplicados entre si ou em 6–11 fazem `begin()` retornar `false`.

### Geometria e rotação

`display.rotation` aceita 0–3; valores fora da faixa são limitados com aviso no Serial. `width()` e `height()` retornam 240x320 em rotação par e 320x240 em rotação ímpar.

### Desenho

A API pública é:

```text
begin()            → valida a configuração e inicializa o Painel
width() / height() → geometria atual, considerando a rotação
clear(color)       → preenche o Display inteiro
fillRect(x, y, w, h, color)
drawText(x, y, text, color, size)
setBrightness(level)
```

As cores são RGB565 (`uint16_t`). `drawText` usa a fonte embutida, datum superior esquerdo e tamanho inteiro.

### Brilho

`setBrightness()` controla o Backlight por PWM direto no pino `display.led` (LEDC, canal 0, 8 bits, 5 kHz). `0` apaga o Backlight e `255` usa intensidade máxima. O Brilho inicial vem de `display.brightness`; alterações em runtime não são persistidas no `Config`. O controle de Backlight da LovyanGFX não é usado.

---

# Main Loop and Updates

Atualmente:

```text
setup()
 ├── Boot::addModule(display)
 ├── Boot::addModule(input)
 └── Boot::begin()   → em caso de falha, imprime o motivo e para (haltOnBootFailure)

loop()
 └── Input::update()
```

O sistema ainda não possui o loop de atualização completo.

A direção planejada para o MVP é um loop cooperativo:

```text
loop()
 ├── Input::update()
 ├── Audio::update()
 ├── System::update()
 └── Runtime::update()
```

Não utilizar tarefas FreeRTOS para cada módulo simplesmente para paralelizar componentes.

---

# FreeRTOS

O Arduino framework do ESP32 utiliza FreeRTOS internamente.

Isso não significa que o projeto deva utilizar uma task FreeRTOS para cada componente.

Para o MVP:

- Arduino framework continua sendo a base;
- o loop cooperativo é a estratégia principal;
- `update()` deve ser rápido e não bloqueante;
- operações demoradas devem ser evitadas dentro do loop.

FreeRTOS poderá ser utilizado posteriormente quando existir uma necessidade concreta de:

- concorrência;
- tarefas independentes;
- filas;
- sincronização;
- operações de I/O demoradas;
- processamento em background.

Não introduza FreeRTOS apenas por antecipação.

---

# Memory and Embedded Constraints

O ESP32 possui recursos limitados.

Priorize:

- estruturas de tamanho fixo quando apropriado;
- alocação dinâmica mínima;
- evitar cópias desnecessárias;
- evitar `String` em caminhos críticos sem necessidade;
- evitar containers excessivamente complexos;
- armazenamento previsível.

Não otimizar prematuramente, mas também não introduzir abstrações que gerem alocações desnecessárias em caminhos executados frequentemente.

---

# Naming and API Conventions

Use nomes consistentes.

Quando uma API representar um estado booleano:

```text
isX()
```

Quando alterar esse estado:

```text
setX()
```

O estado de inicialização dos HALs usa `isInstantiated()` / `setInstantiated()` (definidos em `hal_module`). A grafia correta é `Instantiated`; não reintroduza variantes como `instanciated`.

---

# Known Current Issues

Os itens abaixo descrevem o estado atual do projeto e não devem ser tratados automaticamente como decisões arquiteturais.

- `Boot` não possui `update()`; `loop()` chama `Input::update()` diretamente.
- Ainda não existem Audio, Storage, System nem Runtime.
- O `main.cpp` ainda não desenha uma tela de teste no Display.

Problemas conhecidos devem ser corrigidos quando fizerem parte da área afetada pela mudança ou quando forem explicitamente tratados.

Não introduza `-fpermissive` ou outros mecanismos apenas para mascarar problemas de const-correctness.

---

# Development Rules

Ao alterar o firmware:

1. Preserve a separação entre Boot, Config, HAL, System e Runtime.
2. Não coloque lógica de hardware dentro de `Boot`.
3. Não coloque lógica de sistema dentro de HALs.
4. Não exponha detalhes físicos do hardware para jogos.
5. Não adicione abstrações apenas por antecipação.
6. Prefira interfaces pequenas e específicas.
7. Não obrigue todos os módulos a implementar `update()`.
8. Evite alocação dinâmica desnecessária.
9. Mantenha configurações específicas dentro de seus namespaces.
10. Execute o build completo após alterações.

Antes de criar uma nova abstração, verifique se ela representa uma responsabilidade real ou apenas evita algumas linhas de código.

---

# Current Development Direction

A evolução planejada do firmware é:

```text
1. Boot
      ↓
2. Config
      ↓
3. HAL core
      ↓
4. Input
      ↓
5. Display
      ↓
6. Audio
      ↓
7. Storage
      ↓
8. System
      ↓
9. Game Runtime
      ↓
10. JavaScript games
```

A implementação deve evoluir incrementalmente.

Não implementar antecipadamente o runtime completo de jogos, FreeRTOS, sistema de arquivos avançado ou outras partes futuras enquanto as fundações ainda estiverem sendo definidas.

---

# VSCode

Os arquivos abaixo são gerados/controlados pelo ambiente:

```text
.vscode/c_cpp_properties.json
.vscode/launch.json
```

Eles são gitignored e devem ser regenerados pelo PlatformIO em vez de editados manualmente.

`.vscode/extensions.json` deliberadamente rejeita a extensão/pacote Microsoft C++ em favor do PlatformIO IDE.

Não altere essas configurações sem uma necessidade concreta do projeto.

## Agent skills

### Issue tracker

Issues e specs deste repo vivem no GitHub Issues, operados via `gh`. See `docs/agents/issue-tracker.md`.

### Triage labels

As cinco roles canônicas usam os nomes default (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`). See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: `GLOSSARY.md` e `docs/adr/` na raiz do repo. See `docs/agents/domain.md`.