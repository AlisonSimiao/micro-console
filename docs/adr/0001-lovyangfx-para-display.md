# 0001 - LovyanGFX para o Display

- Status: Aceito
- Data: 2026-11-06

## Contexto

O Display usa um painel ILI9341 240x320 ligado por SPI. O console lê a configuração de hardware em tempo de execução do `Config` (`data/config.ini`), e os pinos do Display precisam acompanhar esse modelo, como já acontece com o Input.

A escolha mais comum para ILI9341 no ESP32 é o TFT_eSPI. Porém o TFT_eSPI configura os pinos em tempo de compilação, por `User_Setup.h` ou por `-D` nos build flags. Usá-lo exigiria gerar flags de build a partir do `Config` ou fixar o mapa de pinos no firmware, contrariando o modelo de configuração do console.

## Decisão

Usar a LovyanGFX como driver do Painel. A configuração do barramento e do painel é feita em tempo de execução, antes da inicialização, com os pinos lidos do `Config`.

A LovyanGFX fica confinada ao `display.cpp`. O header público do HAL não inclui nem expõe tipos da biblioteca.

## Consequências

Positivas:

- os pinos permanecem no `Config`, sem recompilar o firmware para cada mapa de hardware;
- rotação, frequência e painel podem ser ajustados sem alterar o header público;
- o mesmo binário atende variações de pinagem.

Negativas:

- a LovyanGFX é maior e mais complexa do que o necessário para um único painel;
- a versão precisa ser fixada no `platformio.ini` e revalidada a cada upgrade;
- o encapsulamento exige que o objeto do painel viva inteiramente dentro do `display.cpp`.

## Alternativas consideradas

- **TFT_eSPI**: pinos em tempo de compilação; incompatível com pinos no `Config`.
- **Adafruit_ILI9341**: simples, mas sem configuração de barramento em runtime e com desempenho inferior.
- **Driver SPI próprio**: reimplementaria sequências de inicialização e primitivas gráficas sem ganho para o MVP.
