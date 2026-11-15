# JavaScript Engine

Este documento registra a pesquisa, a decisão proposta e o contrato mínimo da
JavaScript Engine do console. A decisão está na
[ADR-0007](./adr/0007-javascript-engine.md), com status **Proposto** até o spike
validar os gates em hardware; a validação de campo está na
[issue #8](https://github.com/AlisonSimiao/micro-console/issues/8). Nenhum código
da engine existe ainda.

A engine executa o entrypoint JavaScript de um jogo, um frame por vez, e é
consumida apenas pelo `GameRuntime`. Ela não desenha, não lê `Input`, não abre o
`.ali` e não conhece telas.

## Premissas de hardware

A decisão assume o alvo **ESP32-S3 com PSRAM**:

| Item | Alvo |
|---|---|
| SoC | ESP32-S3, dual-core LX7 a 240 MHz, 512 KB de SRAM, 384 KB de ROM |
| PSRAM | 8 MB (módulo octal, ex.: N8R8) |
| Flash | 8 MB ou 16 MB, com tabela de partições própria |
| Display | ILI9341 240x320 por SPI (inalterado) |
| Botões | 8 botões (inalterado) |

O `platformio.ini` atual ainda usa `board = esp32dev` (ESP32 clássico, sem PSRAM).
O firmware atual, medido em build local de release, ocupa **625.795 bytes de
flash** (47,7% da partição de aplicação de 1.310.720 bytes) e **55.684 bytes de
RAM estática** (17,0% de 327.680 bytes).

O ESP32 clássico não é alvo desta decisão: sem PSRAM, a arena da engine
disputaria os ~320 KB de DRAM com Display, pilha e o restante do firmware. A
migração para S3 é pré-requisito da integração, não uma consequência dela. Até
existir a placa, o ambiente `esp32` permanece válido e intocado; o spike cria um
ambiente `esp32s3` separado.

## Requisitos

Os requisitos abaixo vêm do produto, não da engine:

1. **Um jogo por vez**, iniciado a partir de um `GameInfo` da `GameLibrary`.
2. **O fonte do jogo vive no `.ali` no cartão** e não é conhecido no build do
   firmware. Não há etapa de build por jogo no dispositivo do autor.
3. **Um frame por chamada**: a engine não possui loop, timer ou task própria. O
   `Console` decide quando cada frame acontece.
4. **Memória limitada e previsível**: o jogo não pode consumir a RAM interna do
   console; o custo da engine precisa caber em um orçamento declarado.
5. **Falha do jogo isolada**: erro de sintaxe, exceção, laço infinito ou falta de
   memória causados pelo jogo encerram a sessão com `Failed`; não derrubam o
   console nem reiniciam a placa pelo watchdog. Bugs da própria engine ou do SDK
   não são contidos (ver [Segurança e isolamento](#segurança-e-isolamento)).
6. **Sem acesso a hardware**: o jogo só alcança o que o Game SDK expõe. GPIO,
   SPI, registradores e objetos HAL não são visíveis.
7. **Teardown determinístico**: ao final da sessão, toda a memória da engine é
   liberada e a próxima sessão começa do zero.
8. **Sem task FreeRTOS nova**: o projeto adota loop cooperativo e não deve criar
   uma task por módulo. A engine precisa caber na stack da task de loop do
   Arduino (8 KB por padrão) ou o desvio precisa ser justificado e documentado.
9. **Licença permissiva** para firmware de produto.
10. **API C/C++ embutível**, sem dependência de sistema operacional, filesystem
    ou rede.

O requisito 8 é o mais restritivo e elimina candidatos que precisam de 32–64 KB
de stack C para o parser.

## Critérios de avaliação

| Critério | Por que importa |
|---|---|
| Embutibilidade | Ser uma biblioteca C chamável, não um firmware completo |
| Licença | Distribuição do firmware do console |
| Nível de linguagem | Autores de jogos escrevem JS comum, sem transpilador no dispositivo |
| Compilação no dispositivo | O fonte chega pelo cartão em tempo de execução |
| Modelo de memória | Arena fixa ou alocador controlável, sem fragmentar a DRAM |
| Stack C | Cabe no loop cooperativo? |
| Interrupção de execução | Laço infinito não pode travar o console |
| Bytecode | Otimização opcional; nunca confiável vindo de pacote de terceiro |
| Manutenção | Projeto ativo ou código pequeno o suficiente para manter |
| Esforço de integração | PlatformIO + Arduino, sem toolchain paralelo |

## Comparação

Valores de footprint são os publicados pelos projetos, em plataformas diferentes;
servem para ordem de grandeza, não para comparação exata. O spike mede o número
real em Xtensa.

| Engine | Licença | Linguagem | Memória | Flash | GC / alocação | Compila no device | Stack C |
|---|---|---|---|---|---|---|---|
| **MQuickJS** | MIT | Subconjunto ES5 estrito (`var`/`function`) | arena fornecida pelo chamador; mínimo 10 KB | ~100 KB (ARM Thumb-2) | tracing + compacting; sem `malloc` | sim, parser sem recursão | limitada por construção |
| **QuickJS-ng** | MIT | ES2025 | heap via `malloc` (limite configurável) | 367 KiB no hello world x86; centenas de KB em Xtensa | reference counting + ciclo | sim, parser recursivo | alta (32–64 KB no ESP32-S3) |
| **JerryScript** | Apache-2.0 | ES5.1 + parte de ES2020 (84% do Test262 ES2020) | heap fixo `JERRY_GLOBAL_HEAP_SIZE` (ex.: 128 KB) | ~258 KB (ARM Thumb-2) | mark-sweep, sem compactação | sim, parser recursivo | média |
| **XS (Moddable)** | LGPL-3.0 (runtime) | ES2023 | ~162 KB de VM por padrão no Moddable Six | ~170 KB adicionais no ESP32 | mark-sweep + compactação | desencorajado; modelo é pré-compilar no PC | baixa |
| **Espruino** | MPLv2 | ES5.1 + parte de ES6 | firmware inteiro | ~1,36 MB de imagem no ESP32 | próprio | sim | — (não é biblioteca) |
| **Duktape** | MIT | ES5.1 | heap próprio | ~200 KB | reference counting + mark-sweep | sim | média |

## Análise

### MQuickJS

Motor novo de Fabrice Bellard (primeira versão em dezembro de 2025), derivado do
QuickJS e desenhado exatamente para este caso: roda com 10 KB de RAM, ~100 KB de
ROM, recebe **um buffer de memória do chamador** e só aloca dentro dele. O parser
não é recursivo, o bytecode é gerado em uma passada e a VM não usa a stack da
CPU.

O que decide a favor:

- **Arena explícita**: o adapter aloca um único bloco de, por exemplo, 1 MB em
  PSRAM e o entrega ao MQuickJS, que só aloca dentro dele. Não há `malloc` por
  objeto, não há fragmentação da DRAM interna e o custo é auditável com
  constantes, como o resto do projeto.
- **GC compactante**: resolve fragmentação sem MMU, o que importa em uma arena
  de 1 MB usada por um jogo que aloca e descarta objetos por frame.
- **Parser sem recursão**: a engine cabe na task de loop; não é preciso criar
  task só para dar stack ao parser (requisito 8).
- **`JS_SetInterruptHandler`**: o adapter impõe o orçamento de cada chamada e
  aborta laços infinitos antes do watchdog.
- **MIT** e um único `mquickjs.c` (~560 KB de fonte) + header: vendorizar é
  trivial e o código é auditável.
- **Bytecode persistente** existe, mas fica fora da V1 (ver segurança).

O que pesa contra:

- **Linguagem ES5 estrita**: sem `let`/`const`, `class`, arrow functions,
  template literals, destructuring ou spread. Arrays não podem ter buracos
  (`a[10] = 1` em um array vazio é `TypeError`), `Date` só tem `Date.now()` e
  `RegExp` faz case folding apenas em ASCII.
- **Projeto com menos de um ano** e mantenedor único. É o risco principal.
- **GC móvel**: bindings nativos não podem guardar `JSValue` entre alocações;
  precisam usar `JSGCRef`. Isso restringe como o Game SDK é escrito.
- **Footprint em Xtensa desconhecido**: ~100 KB é a medida em ARM Thumb-2.

### QuickJS-ng

É o sucessor comunitário do QuickJS, muito ativo, com componente oficial
`espressif/quickjs-ng` no registro do ESP-IDF e uso documentado em ESP32-S3 com
heap em PSRAM. Linguagem completa (ES2025), API C estável, `JS_SetMemoryLimit`,
`JS_SetInterruptHandler` e alocador customizável.

É o **fallback** da decisão, não a escolha, por dois motivos concretos:

- **Stack**: o parser é recursivo e projetos reais em S3 usam 32–64 KB de stack
  na task principal. Isso viola o requisito 8 e forçaria uma task dedicada ou o
  aumento da stack do loop.
- **Alocação por `malloc`**: mesmo com `JS_SetMemoryLimit`, o heap é fragmentável
  e o pico depende do jogo; a arena fixa do MQuickJS é mais fácil de orçar.

O tamanho (~700 KB–1 MB de flash) pesa menos: no S3, com 8–16 MB de flash e
tabela de partições própria, ele cabe; só seria impeditivo na partição padrão
de 1,31 MB do ESP32 clássico, que já não é alvo.

A API é prima da do MQuickJS, então a migração, se necessária, afeta
principalmente os bindings do SDK e o alocador — não o contrato do `GameRuntime`.

### JerryScript

Era o candidato "seguro" clássico: ES5.1 completo, parte relevante de ES2020,
API C madura, snapshot de bytecode e uso comprovado em ESP32. Dois detalhes o
desqualificam para este alvo:

- **Heap estático em `.bss`**: `JERRY_GLOBAL_HEAP_SIZE` vira um array global. Em
  S3, 128–512 KB em `.bss` precisam ser colocados em PSRAM por atributo de seção
  e opção de build (`EXT_RAM_BSS_ATTR`), o que exige patch no vendor e configuração
  específica. MQuickJS resolve isso na API, sem patch.
- **Manutenção lenta**: a última release é a 3.0.0 (dezembro de 2024), com
  atividade esparsa desde então.

Também é mais lento que QuickJS/MQuickJS e o parser usa stack da CPU.

### XS (Moddable)

Tecnicamente o motor mais maduro para microcontroladores e o mais econômico em
RAM, porque compila e linka o JS no PC e executa bytecode direto da flash. O
modelo, porém, é o oposto do necessário: o `.ali` chega depois, no cartão, e o
parser em dispositivo é justamente o recurso desabilitado por padrão. Além disso,
o runtime é LGPL-3.0 e a integração exige o toolchain do Moddable SDK
(`mcconfig`), em paralelo ao PlatformIO/Arduino.

### Espruino

Não é uma engine embutível: é um firmware completo, com loop de eventos próprio
e uma única instância global do interpretador. Adotá-lo significaria substituir o
`Console`, o `Boot` e a UI, não integrar uma engine ao `GameRuntime`. A licença
MPLv2 e a imagem de ~1,36 MB também pesam.

### Duktape e mJS/Elk

Duktape está em manutenção de baixa intensidade e não oferece vantagem sobre
JerryScript ou MQuickJS. mJS e Elk são deliberadamente minimalistas (sem
biblioteca padrão suficiente, sem GC comparável) e não sustentam um SDK de jogos.

## Decisão

**MQuickJS**, vendorizado em versão fixa, atrás de uma fachada `JsEngine`
consumida pelo `GameRuntime`. **QuickJS-ng** é o fallback documentado: se o spike
falhar um dos gates, a decisão é revisitada por uma ADR que substitui a 0007, sem
mudar o contrato descrito abaixo.

A decisão é **proposta** (ADR-0007) até o spike medir os gates em hardware. O
contrato abaixo (arena fixa, um frame por chamada, falhas por código e nenhum
bytecode vindo de pacote) é independente da engine e não deve mudar se o
fallback for adotado.

A escolha não é "a engine mais completa", e sim a única que satisfaz ao mesmo
tempo arena fixa em PSRAM, parser sem recursão, interrupção de execução e licença
permissiva.

## Contrato mínimo

A fachada é a única fronteira entre o `GameRuntime` e a engine. Ela não inclui
`Arduino.h`, HAL, UI ou tipos do MQuickJS.

```cpp
// firmware/game/engine/js_engine.hpp (planejado)
class JsEngine {
public:
    enum class Result {
        Ok,
        OutOfMemory,
        SyntaxError,
        MissingFrame,
        RuntimeError,
        Timeout,
        Internal
    };

    virtual ~JsEngine() = default;

    // Aloca a arena de tamanho fixo e cria o contexto. Chamar duas vezes sem
    // end() é erro de uso; o retorno é false. Falta de memória também é false.
    virtual bool begin(size_t arenaBytes) = 0;

    // Reserva o buffer temporário do fonte, com exatamente `length` bytes.
    // Retorna nullptr se `length` exceder o limite do fonte ou faltar memória.
    // O buffer pertence à engine e é liberado por load() ou end().
    virtual char* reserveSource(size_t length) = 0;

    // Compila o fonte reservado, executa o topo do script e resolve onFrame.
    // Tem orçamento próprio, separado do orçamento de frame.
    virtual Result load(uint32_t budgetMs) = 0;

    // Executa exatamente um frame: chama onFrame(dtMs). O orçamento conta só
    // o tempo de script; o tempo dentro das funções nativas do SDK é descontado.
    virtual Result frame(uint32_t dtMs, uint32_t budgetMs) = 0;

    // Libera contexto, buffer do fonte e arena. Idempotente; a próxima sessão
    // chama begin() de novo.
    virtual void end() = 0;

    virtual size_t heapUsed() const = 0;
    virtual size_t heapPeak() const = 0;
    virtual const char* errorMessage() const = 0;
    virtual int errorLine() const = 0;
};
```

Regras do contrato:

- **Um contexto por sessão.** `begin()` no `launch()`, `end()` no `stop()` e em
  qualquer falha. Nada sobrevive à sessão.
- **`load()` não desenha e não lê input.** Ele apenas compila e executa o topo do
  script. Efeitos visuais pertencem a `onFrame`.
- **`frame()` executa exatamente um frame.** Não há timers, `setTimeout`,
  `setInterval`, promises ou event loop. O tempo vem do `Console`.
- **`onFrame(dt)` é obrigatório.** Ausente ou não-função ⇒ `MissingFrame` em
  `load()`. O SDK pode, no futuro, oferecer `onStart`/`onStop`, mas a V1 não exige.
- **Falhas por código, sem log.** A engine devolve `Result` e mensagem/linha; o
  `Console` traduz para o Serial. O `GameRuntime` continua sem `Arduino.h`.
- **`Timeout`** é retorno de orçamento estourado (interrupt handler), não de um
  watchdog da placa.
- **Orçamento é tempo de script.** `load()` e `frame()` têm orçamentos separados
  (ponto de partida: 500 ms e 20 ms). O relógio do orçamento pausa enquanto uma
  função nativa do SDK executa, então o desenho no Display (SPI) não provoca
  `Timeout`. O tempo total do frame, incluindo desenho, é medido à parte no spike.
- **`OutOfMemory`** é a arena esgotada ou uma alocação recusada. Nunca deve
  abortar o firmware.
- **A engine é dona da memória da sessão.** A implementação MQuickJS aloca a
  arena e o buffer do fonte em PSRAM e os libera em `end()`. O `GameRuntime` não
  chama `heap_caps_malloc` e continua testável com um fake que não aloca nada.
  Como o Runtime chama `end()` em toda falha, `Failed` continua significando
  "nenhum recurso retido" (ADR-0006).
- **`heapUsed()`/`heapPeak()`** existem para o diagnóstico e para os gates do
  spike: o adapter deriva o uso do relatório de memória do MQuickJS
  (`JS_DumpMemory` capturado pelo write func) e amostra o pico ao fim de cada
  frame. Não é um contador de precisão de byte.

## Modelo de execução

```text
Console::update()
 ├── Input::update()
 ├── GameRuntime::update(dtMs)        // apenas em Running
 │    └── engine.frame(dtMs, 20 ms)
 │         └── onFrame(dt)
 │              └── SDK: input.isPressed("A"), display.fillRect(...)
 ├── Runtime em Failed?               // log, stop, HomeScreen::begin()
 └── senão: telas
```

O `Console` mede `dt` e o repassa; o `GameRuntime` não lê relógio. Só a
implementação MQuickJS usa o relógio da plataforma (`esp_timer`), e apenas para
o interrupt handler aplicar o orçamento. O jogo não recebe tempo real além de
`dt` e `Date.now()`.

`onFrame` roda com os botões já atualizados no frame corrente; o SDK lê bordas e
estado do `Input` e as consome. Durante `Running`, as telas não são atualizadas
(regra já fixada na ADR-0006).

## Memória

Orçamento inicial da V1, a confirmar no spike:

| Item | Tamanho | Onde |
|---|---:|---|
| Arena JavaScript | 1 MB | PSRAM |
| Buffer do fonte do jogo | até 256 KB, temporário | PSRAM |
| Pico de stack da task de loop, de ponta a ponta | ≤ 6 KB | task de loop (8 KB) |
| Código da engine | ~100–200 KB (a medir em Xtensa) | flash |
| Contexto C++ da fachada | < 1 KB | DRAM |

- A arena é alocada pela implementação MQuickJS em `begin()`, uma vez por
  sessão, com `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`, e liberada em `end()`.
  Não há heap global da engine.
- O buffer do fonte é reservado na engine por `reserveSource()` e liberado ao fim
  de `load()`. O Runtime o preenche a partir do `PackageFile` em blocos de 4 KB;
  o pacote não é carregado inteiro.
- O limite de stack vale para a cadeia inteira dentro de `loop()`:
  `Console::update()` → `GameRuntime::update()` → engine → bindings do SDK →
  `Display`/LovyanGFX. Medir só a engine não basta.
- O GC é compactante: pausa alvo abaixo de 5 ms por coleta, medida no spike.
- Jogos maiores que 256 KB de fonte ou que não caibam na arena falham em `load()`
  com `OutOfMemory`, sem tentativa de degradação.

## Carregamento do `.ali`

1. O `Console` chama `GameRuntime::launch(game)`.
2. O Runtime abre o pacote, valida o manifesto e abre o entrypoint.
3. O Runtime chama `engine.begin()` e `engine.reserveSource(tamanho)`; entrypoint
   maior que 256 KB falha com `OutOfMemory`.
4. O Runtime copia o entrypoint em blocos para o buffer reservado.
5. A fachada compila e executa o topo (`load`), com orçamento de carregamento.
6. Se `onFrame` existir, a sessão entra em `Running`; senão, `Failed(MissingFrame)`.
7. O buffer do fonte é liberado dentro de `load()`; a arena permanece.

`launch()` continua síncrono: a leitura do cartão e o `load()` bloqueiam o loop
até terminar (gate de 500 ms para 64 KB).

**Bytecode nunca é carregado de um pacote.** O formato do MQuickJS não é
verificado antes da execução e não há compatibilidade entre versões. Só fonte é
aceita na V1. Bytecode pré-compilado pode ser considerado no futuro apenas para
jogos de primeira parte, embarcados no firmware.

## Segurança e isolamento

O isolamento é **de API, não de memória**. O jogo executa no mesmo espaço de
endereços do firmware; um bug na engine ou no SDK pode derrubar o console. O que
a V1 garante:

- O jogo só alcança o objeto do Game SDK (e `Math`, `JSON`, `String`, etc.).
  Não há `Storage`, `Display`, `Input`, `System` nem GPIO expostos diretamente.
- Não há `eval` direto, `Function` constructor, `import`, `setTimeout` ou acesso a
  rede. O MQuickJS só aceita `eval` global; a V1 não o expõe.
- Laço infinito é interrompido pelo `JS_SetInterruptHandler` no orçamento de
  `load()` ou do frame.
- OOM vira exceção e `Failed(OutOfMemory)`.
- O pacote é lido pelo firmware; o jogo não recebe `PackageFile` nem caminhos do
  cartão.

Riscos aceitos e registrados: jogo malicioso pode consumir CPU até o orçamento de
cada frame (não há limite de frames), e um bug de memória da engine não é contido.

## Subconjunto JavaScript

A V1 fala um subconjunto ES5 estrito. Isso precisa estar na documentação do Game
SDK, porque muda como os jogos são escritos:

- `var` e `function`; sem `let`, `const`, `class`, arrow functions, template
  literals, destructuring ou spread.
- `for...of` só sobre arrays; `for...in` itera apenas propriedades próprias.
- Arrays sem buracos: escrever além do fim é erro, exceto no fim exato.
- `Date.now()` apenas; `RegExp` com case folding ASCII.
- `Math`, `JSON` e `String` básicos disponíveis; sem `Promise`, `Map`, `Set`,
  `Symbol` ou módulos.

Jogos que precisem de sintaxe moderna podem ser transpilados para ES5 fora do
console; o firmware não executa Babel. Essa é uma decisão de produto em aberto,
não um requisito da engine.

## Integração e posse

```text
Console (composition root)
 ├── JsEngine (implementação MQuickJS; aloca a arena em PSRAM por sessão)
 ├── GameRuntime(JsEngine&, Storage&, GamePackage próprio)
 └── GameSdk (bindings nativos, registrados no contexto)
```

- O `Console` possui o objeto da engine; a engine possui a memória da sessão. O
  `GameRuntime` recebe `JsEngine&` e permanece testável nativamente com um fake,
  como já ocorre com o `Storage`.
- O `GameSdk` é o único código que conhece a API C do MQuickJS. Ele registra as
  funções nativas no contexto e traduz os tipos. A fachada não expõe `JSContext*`.
- A stdlib do MQuickJS é gerada no host por `mquickjs_build` a partir de um
  arquivo `.c` que declara a API visível ao jogo. O header gerado é versionado no
  repositório e regenerado só quando o SDK muda; o build do firmware não roda
  codegen.
- `GameRuntime` continua sem `Arduino.h`, sem `Input`, sem `Display` e sem log.

A integração muda a API hoje fechada do `GameRuntime` em dois pontos:

- `update()` passa a ser `update(uint32_t dtMs)`, porque o Runtime não lê
  relógio e precisa repassar `dt` à engine.
- `Failure` ganha um valor para cada `JsEngine::Result` diferente de `Ok`
  (`ScriptOutOfMemory`, `ScriptSyntaxError`, `ScriptMissingFrame`,
  `ScriptRuntimeError`, `ScriptTimeout`, `EngineInternal`) e `EngineBeginFailed`
  para `begin()` falso. Mensagem e linha continuam vindo de `errorMessage()` e
  `errorLine()`, lidas pelo `Console` antes de `stop()`.

Essas mudanças entram junto com a integração e atualizam
[game-runtime.md](./game-runtime.md) e o `AGENTS.md` no mesmo commit.

A ADR-0006 adiou a criação de uma interface de engine porque não havia consumidor
nem engine escolhida. Agora há os dois: o `GameRuntime` consome e os testes
nativos precisam de um fake. A fachada `JsEngine` é a menor interface que atende
a esses dois usos.

## Riscos

| Risco | Mitigação |
|---|---|
| MQuickJS é novo e tem mantenedor único | Versão fixa vendorizada, fachada pequena, testes nativos do contrato, fallback QuickJS-ng documentado |
| Subconjunto ES5 limita autores | Documentar o subconjunto no SDK; transpilação opcional fora do console |
| GC móvel quebra bindings | Regras de `JSGCRef` no código do SDK, revisão específica, testes de frame |
| Footprint em Xtensa maior que o estimado | Medição no spike; partição de aplicação própria no S3 |
| PSRAM mais lenta que SRAM | Medir frame e pausa de GC no spike; se estourar, reduzir a arena ou mover estruturas quentes da engine para SRAM |
| Desenho imediato não cabe em 30 FPS | Tela cheia RGB565 a 40 MHz leva ~31 ms de SPI; o SDK deve incentivar redesenho parcial, e o spike mede o tempo total do frame com desenho |
| Framebuffer futuro disputa PSRAM | 8 MB comportam arena (1 MB) + 2 framebuffers 240x320 RGB565 (~300 KB) |
| Migração S3 muda pinos, partições e Wokwi | Spike em ambiente separado; `esp32` continua válido até a placa existir |

## Validação

O spike mede, em uma placa ESP32-S3 real com PSRAM:

1. Footprint de flash e DRAM da engine compilada no ambiente `esp32s3`.
2. Pico de stack da task de loop no parse e no frame, com SDK e Display na
   cadeia (`uxTaskGetStackHighWaterMark`).
3. Tempo de `load()` para fontes de 16 KB, 64 KB e 256 KB.
4. Tempo de frame de um jogo de referência (tilemap 30x20, ~200 sprites 8x8,
   texto e input), separado em tempo de script e tempo total com desenho.
5. Pausa máxima do GC e `heapPeak()` para o mesmo jogo.
6. Comportamento em `while (true) {}` (deve terminar em `Timeout` no orçamento).
7. Comportamento em alocação acima da arena (deve terminar em `OutOfMemory`).
8. Retorno da PSRAM após `end()` (sem vazamento entre sessões).

Gates mínimos: pico de stack da task de loop ≤ 6 KB, frame ≤ 16 ms de script,
pausa de GC ≤ 5 ms, `load()` ≤ 500 ms para 64 KB, nenhum reset por watchdog,
memória devolvida ao final. Falha em qualquer gate reabre a decisão. O tempo
total do frame com desenho é registrado, mas não é gate da engine: ele depende
do Display e do SDK.

A execução e o registro das medições ficam na
[issue #8](https://github.com/AlisonSimiao/micro-console/issues/8).

## Perguntas em aberto

- Tamanho final da arena (1 MB é o ponto de partida; o jogo de referência define).
- `Math.random` e determinismo: usar o do MQuickJS ou uma função do `System` com
  semente controlada?
- `console.log` em build de release: habilitado com limite de taxa ou removido?
- Formato do `onFrame`: apenas `dt` ou também um snapshot de input?
- Transpilação ES6→ES5 no fluxo de autoria de jogos (fora do dispositivo).
- Áudio no SDK e seu impacto no orçamento de frame.
- FPS alvo do console e se o SDK oferece framebuffer em PSRAM ou só desenho
  imediato com redesenho parcial.

## Referências

- [MQuickJS](https://github.com/bellard/mquickjs) — README e licença MIT
- [QuickJS-ng](https://github.com/quickjs-ng/quickjs) e o componente
  [`espressif/quickjs-ng`](https://components.espressif.com/components/espressif/quickjs-ng)
- [JerryScript 3.0.0](https://github.com/jerryscript-project/jerryscript/releases/tag/v3.0.0)
- [Moddable XS: compilation on embedded devices](https://www.moddable.com/blog/eval/)
  e [licenças do Moddable SDK](https://github.com/Moddable-OpenSource/moddable/blob/public/licenses/readme.md)
- [Espruino](https://github.com/espruino/Espruino)
- [ESP32-S3 datasheet](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf)
- [Game Runtime](./game-runtime.md) e
  [ADR-0006](./adr/0006-game-runtime-session-and-resource-ownership.md)
