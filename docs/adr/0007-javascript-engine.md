# ADR-0007: Engine JavaScript, arena em PSRAM e contrato de frame

- Status: Proposto (validação no spike [#8](https://github.com/AlisonSimiao/micro-console/issues/8))
- Data: 2026-11-15

## Contexto

O `GameRuntime` implementa o lifecycle de uma sessão, mas não executa nada: não
há engine JavaScript nem Game SDK. O próximo passo é escolher a engine, porque
ela define o orçamento de memória, o modelo de execução do jogo, o desenho do SDK
e o tamanho dos pacotes `.ali`.

A ADR-0006 adiou essa decisão e fixou apenas os pontos de inserção (inicialização
em `launch()`, avanço em `update()`, encerramento em `stop()`), além de proibir
uma interface `GameEngine` sem consumidor. Os dois consumidores agora existem: o
`GameRuntime` e os testes nativos, que precisam de um fake.

O alvo é o **ESP32-S3 com 8 MB de PSRAM**, com o firmware atual medido em 626 KB
de flash e 56 KB de RAM estática. O fonte do jogo chega em tempo de execução pelo
cartão, é executado um frame por vez no loop cooperativo e não pode derrubar o
console. A pesquisa completa, os números e os candidatos descartados estão em
[javascript-engine.md](../javascript-engine.md).

## Decisão

**MQuickJS**, vendorizado em versão fixa, é a JavaScript Engine proposta para o
console. A versão inicial é o commit `4119f68` (2026-11-09), o último da `main`
na data desta decisão; o repositório não publica tags. O hash e a data devem ser
conferidos no repositório no momento da vendorização.

A escolha decorre de quatro propriedades que os outros candidatos não reúnem:

1. **Arena fornecida pelo chamador.** O adapter aloca um bloco fixo de PSRAM por
   sessão e o entrega ao MQuickJS, que só aloca dentro dele; não há `malloc` por
   objeto nem fragmentação da DRAM interna.
2. **Parser sem recursão e VM sem stack da CPU.** A engine cabe na task de loop
   do Arduino, sem criar uma task FreeRTOS só para dar stack ao parser.
3. **Interrupção de execução** (`JS_SetInterruptHandler`), que permite impor um
   orçamento por frame e encerrar laços infinitos antes do watchdog.
4. **Licença MIT** e código pequeno e auditável, compatível com firmware de
   produto.

**QuickJS-ng é o fallback documentado.** Se o spike falhar um gate de stack,
footprint ou memória, uma nova ADR substitui esta. O contrato abaixo não muda: a
API é prima da do MQuickJS e a migração afeta os bindings do SDK e o alocador.

**A fronteira é uma fachada `JsEngine`**, sem tipos do MQuickJS, sem `Arduino.h`
e sem HAL:

```text
begin(arenaBytes)                → aloca a arena e cria o contexto
reserveSource(length)            → buffer temporário do fonte, da engine
load(budgetMs)                   → compila, executa o topo e resolve onFrame
frame(dtMs, budgetMs)            → executa exatamente um frame
end()                            → libera contexto, buffer do fonte e arena
heapUsed() / heapPeak() / errorMessage() / errorLine()
```

Resultados: `Ok`, `OutOfMemory`, `SyntaxError`, `MissingFrame`, `RuntimeError`,
`Timeout`, `Internal`.

**O modelo é um frame por chamada.** Não há timers, `setTimeout`, `setInterval`,
promises, event loop ou task da engine. `onFrame(dt)` é obrigatório; o tempo vem
do `Console`. O topo do script executa em `load()`, com orçamento próprio (500 ms
de partida), separado do orçamento de frame (20 ms de partida).

**O orçamento conta tempo de script.** O relógio pausa dentro das funções nativas
do SDK; desenhar no Display por SPI não provoca `Timeout`. O tempo total do
frame, com desenho, é medido à parte.

**A engine é dona da memória da sessão.** Arena de 1 MB em PSRAM, alocada em
`begin()` e liberada em `end()`; fonte do jogo limitado a 256 KB em um buffer
temporário da engine. O `GameRuntime` não aloca PSRAM e continua testável com um
fake. O pico de stack da task de loop, de ponta a ponta, fica em até 6 KB. O jogo
que exceder a arena falha com `OutOfMemory`, sem degradação.

**Bytecode nunca vem do pacote.** O formato do MQuickJS não é verificado antes da
execução e não tem compatibilidade entre versões; apenas fonte é aceita na V1.
Bytecode pré-compilado fica restrito, no futuro, a jogos embarcados no firmware.

**O isolamento é de API, não de memória.** O jogo roda no mesmo espaço de
endereços do firmware e só alcança o objeto do Game SDK. Não há acesso a GPIO,
SPI, `Storage`, `Display`, `Input`, rede ou `eval`.

**O `GameSdk` é o único código que conhece a API C do MQuickJS.** Ele registra as
funções nativas e traduz tipos, respeitando o GC móvel (`JSGCRef`). A stdlib é
gerada no host por `mquickjs_build`; o header gerado é versionado e o build do
firmware não roda codegen.

Quando aceita, esta ADR substitui apenas o item da ADR-0006 que adiava a
interface de engine; o restante da ADR-0006 (sessão única, posse do
`GamePackage`, execução exclusiva, estados e transições de UI) continua valendo.
Enquanto estiver proposta, a ADR-0006 permanece integralmente em vigor.

## Consequências

- O `Console` passa a possuir o objeto da engine; o `GameRuntime` recebe
  `JsEngine&` e continua sem `Arduino.h`, testável nativamente com um fake.
- A API fechada do `GameRuntime` muda: `update()` vira `update(dtMs)` e `Failure`
  ganha um valor por `JsEngine::Result` de erro, mais `EngineBeginFailed`.
- A sessão ganha ~1,25 MB de PSRAM em uso, o que exige a migração para S3. O
  ambiente `esp32` atual permanece válido até a placa existir.
- A partição de aplicação do S3 precisa ser própria; a partição padrão de 1,31 MB
  já está 47,7% ocupada pelo firmware atual.
- Desenho imediato de tela cheia (~31 ms de SPI a 40 MHz) não cabe em 30 FPS; o
  SDK precisará favorecer redesenho parcial ou um framebuffer em PSRAM.
- O jogo é escrito em ES5 estrito (`var`/`function`), sem `let`/`const`, `class`
  ou arrow functions. Isso precisa constar na documentação do Game SDK.
- Falhas de jogo (`Timeout`, `OutOfMemory`, `RuntimeError`) passam a ser
  observáveis como `Failed` do Runtime e logadas pelo `Console`.
- O `GameRuntime` deixa de ser um stub: `frameCount()` passa a refletir frames
  realmente executados.
- A saída provisória com `B` precisa ser reavaliada quando o SDK definir os
  controles do jogo (dívida já registrada na ADR-0006).
- Um jogo malicioso pode consumir CPU até o orçamento de cada frame; não há
  limite de frames nem isolamento de memória.

## Alternativas consideradas

- **QuickJS-ng.** Linguagem completa (ES2025), muito ativo e com componente
  oficial do ESP-IDF, mas exige 32–64 KB de stack para o parser e usa heap por
  `malloc`; o tamanho (~700 KB–1 MB de flash) cabe no S3. Fica como fallback
  porque o contrato não depende da engine.
- **JerryScript.** ES5.1 + parte de ES2020 e API madura, mas o heap é um array
  estático em `.bss`, que precisaria ser colocado em PSRAM por patch e opção de
  build, e a manutenção está lenta desde a 3.0.0.
- **XS (Moddable).** O motor mais econômico em RAM, mas o modelo é pré-compilar no
  PC; o parser em dispositivo é desabilitado por padrão, o runtime é LGPL-3.0 e a
  integração exige o toolchain do Moddable SDK.
- **Espruino.** Não é biblioteca embutível: é um firmware completo com loop de
  eventos próprio, o que substituiria o `Console` em vez de integrar-se a ele.
- **Escrever um interpretador próprio.** Custaria mais que integrar uma engine e
  sem ganho de linguagem; rejeitado.
- **Uma interface `GameEngine` genérica agora.** A fachada `JsEngine` é a menor
  interface que atende ao Runtime e aos testes; não há abstração para trocar de
  engine em runtime.

## Referências

- [JavaScript Engine](../javascript-engine.md)
- [Game Runtime](../game-runtime.md)
- [ADR-0006: Sessão única do Game Runtime e posse de recursos](./0006-game-runtime-session-and-resource-ownership.md)
- [MQuickJS](https://github.com/bellard/mquickjs)
