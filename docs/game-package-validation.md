# Validação do Game Package

## Testes nativos

Os testes executam o `GamePackage`, `PackageReader`, `PackageFile`, decodificadores e parser reais. O Storage é substituído apenas no include path por um fake em memória; nenhuma API de produção do Storage é alterada.

```bash
~/.platformio/penv/bin/pio run
bash test/run-game-package-tests.sh
```

O script compila três executáveis nativos com `-Wall -Wextra -Werror`, AddressSanitizer e UndefinedBehaviorSanitizer:

- `game_package_format_test.cpp`: limites de cabeçalho/índice, caminhos, intervalos e manifesto;
- `game_package_reader_test.cpp`: ciclo de vida, falhas de leitura, recursos, leitura incremental e movimentos de `PackageFile`;
- `game_package_fixtures_test.cpp`: fixtures `.ali` gerados por `make_game_package_fixtures.py`, abertos pelo leitor real.

O fake reproduz as regras relevantes do Storage real: normalização de caminhos relativos (prefixo `/`), rejeição de segmentos `..`, limite de 128 bytes no caminho normalizado e no máximo quatro arquivos abertos simultaneamente. Ele também permite limitar o tamanho de cada leitura, forçar EOF em um offset, observar bytes lidos/arquivos abertos e substituir um arquivo entre operações. Para executar localmente sem sanitizers, use `GAME_PACKAGE_SANITIZERS=0 bash test/run-game-package-tests.sh`.

**Resultado local:** as três suítes passaram com ASan/UBSan; o build PlatformIO limpo passou sem warnings. A matriz física abaixo não foi executada neste ambiente.

## Matriz física

**Status: pendente de execução com ESP32, cartão FAT32 e pinagem validada.** Os testes nativos não substituem esta validação. Gere as fixtures em um diretório temporário, copie os arquivos para `/games/` no cartão e use um harness de diagnóstico de desenvolvimento; não inclua o harness no firmware de produção.

| ID | Cenário | Resultado esperado |
|---|---|---|
| HW-01 | Iniciar sem cartão SD | Storage não monta; console continua |
| HW-02 | Cartão sem `/games/` | abertura retorna falso sem travar |
| HW-03 | Abrir `valid.ali` | pacote válido e manifesto correto |
| HW-04 | Ler `Codes/main.js` | bytes correspondem ao fixture |
| HW-05 | Ler `Assets/large.bin` em blocos de 64 bytes | 16 KiB corretos, `byte[i] == i % 256` |
| HW-06 | Abrir cada fixture malformado | rejeição sem crash ou watchdog |
| HW-07 | Consultar arquivo inexistente | `exists()` retorna falso |
| HW-08 | Abrir dois arquivos simultaneamente | posições e conteúdos independentes |
| HW-09 | Fechar e reabrir o pacote | segunda abertura funciona |
| HW-10 | Remover o cartão durante a leitura | `read()` retorna zero após o Storage indicar EOF; sem crash |
| HW-11 | Consultar caminho com caixa diferente | `exists()` retorna falso |

O circuito Wokwi atual não simula o cartão SD, portanto esses casos permanecem fora da validação local automatizada.
