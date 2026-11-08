# 0003 - Declaração de pinos em duas fases

- Status: Aceito
- Data: 2026-11-07

## Contexto

Cada HAL lê seus pinos do próprio namespace do `Config` e valida apenas o próprio domínio. Com isso, `display.cs=18` e `input.up=18` passam nas duas validações: o Display configura o GPIO 18 como CS e, em seguida, o Input o reconfigura como `INPUT_PULLUP`. O Display deixa de funcionar e nenhum módulo reporta erro (issue #4).

A detecção precisa:

- falhar antes que qualquer módulo obrigatório inicialize hardware;
- não fazer um HAL consultar o namespace de outro;
- não colocar conhecimento de pinagem no `Boot`.

## Decisão

Separar a declaração de pinos da inicialização.

- `hal_module` ganha `virtual bool claimPins(PinClaims& claims) const`, com implementação padrão que não declara nada.
- Cada HAL com GPIOs sobrescreve `claimPins()`, lê apenas o próprio namespace e declara cada pino em uso com a chave de configuração correspondente. Pinos desligados (`-1`) não são declarados.
- `PinClaims` (`hal/core/pin_claims.hpp`) guarda pares `pin` + `configKey` em um array de tamanho fixo e rejeita o segundo pedido pelo mesmo GPIO, imprimindo as duas chaves.
- `Boot::begin()` executa `Config.begin()`, depois `claimPins()` em todos os módulos e só então o `begin()` de cada um.
- Para a política de módulos opcionais e a prioridade de declaração de pinos, ver [ADR-0004](./0004-modulos-opcionais-e-storage-spi.md).

A validação específica de cada módulo (GPIO existente, capacidade de saída, faixas) continua no `begin()` do HAL.

## Consequências

Positivas:

- conflitos entre módulos obrigatórios falham antes de qualquer configuração de hardware;
- a mensagem identifica as duas chaves envolvidas;
- o `Boot` continua apenas coordenando, sem lista de chaves de pino;
- HALs sem GPIO não precisam implementar nada;
- módulos opcionais conflitantes podem ser ignorados antes de configurar hardware, sem impedir o restante do console.

Negativas:

- `hal_module` ganha um segundo método virtual;
- cada HAL com GPIOs lê seus pinos duas vezes (em `claimPins()` e em `begin()`);
- `configKey` precisa ter duração estática, pois `PinClaims` guarda apenas o ponteiro;
- a capacidade de `PinClaims` (32) precisa acompanhar o número de pinos usados.

## Alternativas consideradas

- **Registro compartilhado reservado dentro de `begin()`**: o conflito só aparece depois que o primeiro módulo já configurou o hardware.
- **Validação central no `Boot` lendo `input.*` e `display.*`**: o `Boot` passaria a conhecer a pinagem de cada HAL, e cada módulo novo exigiria editar essa lista.
