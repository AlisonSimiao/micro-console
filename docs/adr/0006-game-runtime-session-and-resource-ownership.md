# ADR-0006: Sessão única do Game Runtime e posse de recursos

- Status: Aceito
- Data: 2026-11-14
- A [ADR-0007](./0007-javascript-engine.md) (proposta) substituirá, quando
  aceita, apenas a decisão "Não criar interface `GameEngine` agora".

## Contexto

A `GameLibrary` descobre e valida os jogos em `/games`, mas não os executa. Falta
a camada que transforma um `GameInfo` em uma sessão de execução, com começo, loop
e encerramento. A engine JavaScript ainda não foi escolhida, os limites de
memória dela são desconhecidos e não existe Game SDK.

Três decisões precisam ser tomadas antes de escrever código, porque são difíceis
de mudar depois:

1. **Quantas instâncias de `GamePackage` existem.** A `GameLibrary` já mantém um
   `GamePackage` de vida longa (~9,3 KB). Um segundo pacote no Runtime custa RAM
   permanente; compartilhar o da biblioteca exigiria expor estado interno ou
   transferir propriedade durante o lifecycle.
2. **Quem é dono de `Input` e `Display` durante a execução.** Se as telas
   continuarem atualizando, elas consomem as bordas de botão destinadas ao jogo.
3. **Como o jogo termina na V1**, sem engine, sem SDK e sem menu de sistema.

Também é necessário decidir como o Runtime reporta falhas sem depender da
plataforma, para que ele possa ser testado nativamente como `GamePackage` e
`GameLibrary`.

## Decisão

**Uma sessão por vez.** O `GameRuntime` é único, membro do `Console`, e não
existe gerenciador de sessões, fila ou múltiplas instâncias. `launch()` com uma
sessão ativa é rejeitado; o jogo atual não é substituído nem interrompido.

**O Runtime possui seu próprio `GamePackage`.** A duplicação de ~9,3 KB é aceita
para manter a descoberta e a execução independentes: a `GameLibrary` não expõe
seu pacote, não há transferência de propriedade e cada componente tem um dono
claro. O custo é registrado no orçamento de memória e no `static_assert` do
Runtime. Se houver pressão de heap, a alternativa é içar um único `GamePackage`
para o `Console` e injetá-lo em ambos os módulos; isso é um refactor localizado,
não um requisito atual.

**A execução é exclusiva.** Enquanto `state() == Running`, o `Console` não
atualiza nem desenha `HomeScreen` ou `GamesScreen`; apenas chama
`GameRuntime::update()` e trata a saída. O Runtime continua sem depender de
`Input` e `Display`: ele não lê botões nem desenha, e a exclusividade é uma
política do `Console`.

**Estados públicos: `Idle`, `Running`, `Failed`.** `Loading` não é público
porque `launch()` é síncrono e o estado nunca seria observável. `Stopping` não
existe: a limpeza é interna a `stop()`. `Failed` significa sempre "nenhum recurso
retido", permitindo que o `Console` retorne à Home sem limpeza adicional. `Paused`
não existe na V1 por falta de caso de uso real.

**O Runtime reporta falhas por código, não por log.** `launch()` devolve `bool` e
`failure()` devolve um `Failure`; o `Console` traduz para o Serial. Assim
`game/runtime/` não inclui `Arduino.h` e é testável no runner nativo.

**A transição de UI pertence ao `Console`.** `GamesScreen` expõe
`const GameInfo* selected()` e não trata mais `A`/`B`; o `Console` decide
iniciar, encerrar e voltar para a Home. O Runtime não conhece telas. `B` para sair
durante `Running` é explicitamente provisório até existir engine/SDK.

**Não criar interface `GameEngine` agora.** A engine não foi escolhida. O
contrato fixa apenas os pontos de inserção: inicialização depois da validação em
`launch()`, avanço em `update()`, encerramento em `stop()`. Uma abstração sem
consumidor real seria antecipação.

## Consequências

- O console mantém ~26,4 KB em gerenciamento de jogos (`GameLibrary` ~17,0 KB +
  `GameRuntime` ~9,4 KB); o `static_assert` de cada classe passa a ser o
  orçamento oficial.
- `launch()` chama `GamePackage::open()` e usa ~4,1 KB de stack dentro de
  `loop()`.
- O Runtime é testável nativamente com o fake de Storage, sem Arduino e sem
  JavaScript; `Running` é observável por `frameCount()`.
- `GamesScreen` perde a política de fechar a lista com `A`/`B`; o `Console`
  concentra as transições de tela, e o rodapé da lista precisa refletir
  `A: JOGAR` / `B: VOLTAR`.
- `Running` sem engine não executa jogo algum: é um stub de lifecycle, e o
  `Console` mostra um aviso textual para não deixar a lista congelada.
- O botão `B` como saída é dívida explícita da V1 e deve ser reavaliado quando o
  Game SDK definir os controles do jogo.
- A escolha da engine definirá os limites reais de memória e pode exigir carga
  assíncrona, um estado próprio e uma interface de engine; nada disso é
  antecipado aqui.

## Alternativas consideradas

- **Compartilhar o `GamePackage` da `GameLibrary`.** Evita ~9,3 KB, mas expõe
  estado interno ou exige transferência de propriedade; rejeitado para a V1.
- **Receber `GamePackage&` pelo construtor do Runtime.** Mantém uma instância só,
  mas obriga o `Console` a possuir e orquestrar o pacote de execução, ampliando o
  composition root antes de existir necessidade.
- **`launch()` substituir o jogo atual.** Mais simples de chamar, mais difícil de
  raciocinar: esconderia uma parada de sessão dentro de outra operação.
- **`Input`/`Display` dentro do Runtime.** Levaria política de UI para a camada
  de execução e misturaria posse de hardware com lifecycle.
- **Definir `GameEngine` já.** Seria uma interface inventada sem consumidor.

## Referências

- [Game Runtime](../game-runtime.md)
- [Game Library](../game-library.md)
- [Formato do Game Package](../game-package-format.md)
- [ADR-0005: Formato do Game Package](./0005-formato-do-game-package.md)
