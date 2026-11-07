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
