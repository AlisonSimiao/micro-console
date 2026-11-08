# Micro Console

Firmware para um console baseado em ESP32 DevKit (`esp32dev`), desenvolvido com PlatformIO e Arduino. O projeto está em desenvolvimento: atualmente inclui inicialização do console, configuração via LittleFS, HALs de input, display e Storage, leitor de pacotes de jogos e o menu principal navegável.

## Recursos atuais

- Inicialização coordenada por `Boot`, com validação de conflitos entre GPIOs antes de configurar o hardware.
- Configuração do console em `data/config.ini`, carregada do LittleFS.
- Oito botões com `INPUT_PULLUP`, debounce de 25 ms e consulta de estado e bordas.
- Painel ILI9341 240x320 via SPI, com rotação, desenho imediato e brilho por PWM.
- Storage HAL para microSD por SPI, com consultas e leitura de arquivos em blocos; sua falha não bloqueia o console.
- Game Package para validar pacotes `.ali`, ler o manifesto e abrir arquivos internos incrementalmente.
- Menu principal com as opções Jogos, Configuracoes e Sobre, com seleção circular por UP/DOWN.
- Simulação do circuito no Wokwi.

Game Library, áudio, sistema e runtime JavaScript ainda não estão implementados.
O formato `.ali` V1 está descrito em [`docs/game-package-format.md`](./docs/game-package-format.md), e sua decisão arquitetural em [`ADR-0005`](./docs/adr/0005-formato-do-game-package.md).

## Hardware padrão

| Componente | Sinal | GPIO |
|---|---|---:|
| Botões | A / B / X / Y | 13 / 14 / 16 / 17 |
| Direcional | UP / DOWN / LEFT / RIGHT | 18 / 19 / 21 / 22 |
| Display ILI9341 | SCK / MOSI / CS / DC | 4 / 23 / 27 / 32 |
| Display ILI9341 | LED (backlight) | 33 |
| Display ILI9341 | MISO / RST | desligados (`-1`) |
| microSD (SPI dedicado) | SCK / MISO / MOSI / CS | 5 / 35 / 2 / 15 |

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
| `storage.sck` | 5 | Clock SPI do cartão microSD |
| `storage.miso` | 35 | Dados do cartão para o ESP32 |
| `storage.mosi` | 2 | Dados do ESP32 para o cartão |
| `storage.cs` | 15 | Chip select do cartão |
| `storage.frequency` | 20000000 | Frequência SPI em Hz (400 kHz a 40 MHz) |

Os pinos do display devem ser válidos para o ESP32 e não podem se repetir entre si. GPIOs 6–11 são reservados para a flash. GPIOs 34–39 são somente entrada e não servem para sinais de saída do display. Conflitos entre pinos de módulos obrigatórios impedem a inicialização; conflito com o módulo Storage opcional desabilita apenas o Storage.

Os pinos de Storage são defaults configuráveis, não uma pinagem definitiva. Os quatro sinais SPI são obrigatórios; GPIOs 2, 5 e 15 são pinos de strapping, então confirme que o módulo microSD não afeta os níveis de boot e ajuste `data/config.ini` conforme a placa. O Storage usa um barramento VSPI dedicado, diferente do SPI2_HOST usado pelo Display. Uma falha de montagem ou conflito nos pinos do módulo opcional é reportado, mas não impede Display, Input ou o menu de iniciarem. A montagem SD é síncrona durante o boot e a ausência do cartão pode atrasar a exibição do menu. O Wokwi atual não simula o cartão SD, então a leitura de arquivos precisa de validação física.

## Interfaces disponíveis

O loop principal chama `Console::update()`, que atualiza o input e a tela inicial. A interface pública do console dá acesso ao input, ao display e ao Storage:

```cpp
console.input().isPressed("A");
console.input().wasPressed("A");
console.input().wasReleased("A");

console.display().clear(0x0000); // RGB565
console.display().fillRect(10, 10, 40, 30, 0xFFFF);
console.display().drawText(10, 50, "Ola", 0xFFFF, 1);

if (console.storage().isMounted()) {
    StorageFile file;
    if (console.storage().open("/games/list.txt", file)) {
        uint8_t buffer[64];
        const size_t count = file.read(buffer, sizeof(buffer));
    }
}
```

`StorageFile` fecha automaticamente ao sair de escopo e também pode ser fechado explicitamente; leituras em um arquivo fechado retornam zero. `StorageFileInfo` contém apenas `isDirectory` e `size`. As consultas `wasPressed()` e `wasReleased()` representam bordas detectadas na atualização corrente; consulte-as após a atualização de input. O console inicia no menu principal: UP e DOWN alternam a opção selecionada de forma circular, e os demais botões ainda não executam ações.

O `GamePackage` recebe uma referência ao Storage; ele ainda não é um membro do `Console` nem é usado pelo menu:

```cpp
#include "game/package/game_package.hpp"

if (console.storage().isMounted()) {
    GamePackage game(console.storage());
    if (game.open("/games/example.ali")) {
        const GameManifest& manifest = game.manifest();
        PackageFile entrypoint;
        if (game.openFile(manifest.entrypoint, entrypoint)) {
            uint8_t buffer[64];
            const size_t count = entrypoint.read(buffer, sizeof(buffer));
        }
    }
}
```

Use `PackageFile` enquanto consumir o conteúdo; a leitura é limitada ao tamanho declarado no índice e pode ser feita em blocos. O manifesto e o entrypoint são dados, não são executados. O formato, limites e campos do manifesto estão na [especificação `.ali`](./docs/game-package-format.md).

## Estrutura

```text
firmware/
├── main.cpp
├── boot/       # Orquestra a inicialização
├── config/     # Configuração em memória carregada do LittleFS
├── game/
│   └── package/ # Container .ali, manifesto e arquivos internos
├── console/    # Composição do console
├── ui/
│   └── screens/ # Telas da interface (menu principal)
└── hal/
    ├── core/   # Contratos dos módulos e declarações de pinos
    ├── display/
    ├── input/
    └── storage/ # API independente do backend e implementação Arduino SD
data/
└── config.ini
docs/adr/       # Decisões de arquitetura
docs/game-package-format.md
```

Os includes do firmware são relativos a `firmware/`, por exemplo `#include "hal/input/input.hpp"`.

## Verificação

Não há CI configurada. O build do firmware e os testes nativos do formato são:

```bash
~/.platformio/penv/bin/pio run
bash test/run-game-package-tests.sh
python3 test/make_game_package_fixtures.py /path/to/fixture-output
```

Os testes nativos validam o cabeçalho, os registros do índice, os caminhos e o manifesto. O gerador cria pacotes válidos e casos malformados para copiar ao cartão; eles não simulam a integração com o Storage. Para validar o cartão, use uma placa ESP32 e cartão FAT funcional com a pinagem definida em `data/config.ini`. Teste cartão ausente, pacotes gerados, arquivo interno inexistente e leitura incremental do `Assets/large.bin` (16 KiB, maior que o buffer de exemplo). O Wokwi deste projeto não cobre esses testes físicos; sem cartão, a inicialização opcional do Storage deve falhar sem impedir o restante do console.
