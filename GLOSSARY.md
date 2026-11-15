# Micro Console

Firmware de um console portátil baseado em ESP32 que executa jogos JavaScript sobre uma camada própria de hardware e sistema.

## Language

### Display

**Display**:
A tela do console como um todo: a superfície onde o console e os jogos desenham.
_Avoid_: Tela, screen, LCD

**Painel**:
O módulo físico com o controlador gráfico que compõe o Display.
_Avoid_: Tela, módulo TFT

**Backlight**:
A luz de fundo do Painel.
_Avoid_: LED, luz

**Brilho**:
O nível lógico, de 0 a 255, que controla a intensidade do Backlight.
_Avoid_: Intensidade, luminosidade

### Execução de jogos

**GameRuntime**:
O controlador do ciclo de vida de um jogo por vez: abre o pacote, mantém a sessão, avança e encerra.
_Avoid_: Runtime genérico, GameManager, engine

**Sessão**:
O intervalo entre um `launch()` bem-sucedido e o `stop()` correspondente, incluindo falhas que liberam os recursos.
_Avoid_: Instância, processo, jogo aberto

**JavaScript Engine**:
O componente que interpreta o código JavaScript do jogo e executa o loop da linguagem. Ainda não escolhido.
_Avoid_: Runtime, interpretador do console

**Game SDK**:
A camada de APIs lógicas oferecida ao jogo (`input`, `graphics`, `audio`, filesystem limitado). Ainda não implementada.
_Avoid_: API do firmware, Console API
