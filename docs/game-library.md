# Game Library

A Game Library descobre, valida e representa os jogos disponíveis no cartão microSD. Ela é a camada entre o `Storage`/`GamePackage` e a futura tela de jogos:

```text
/games/
├── space-shooter.ali
├── tetris.ali
└── puzzle.ali
        │
        ▼
   GameLibrary
        ├── GamePackage (uma instância)
        └── GameInfo[MAX_GAMES]
        │
        ▼
   HomeScreen / futura GamesScreen
```

## Responsabilidade

A Game Library apenas:

```text
descobrir → validar → representar
```

Ela **não**:

- executa JavaScript ou o entrypoint;
- carrega assets ou capas na RAM;
- mantém um `GamePackage` por jogo;
- mantém arquivos abertos depois do `scan()`;
- instala, remove, atualiza ou baixa jogos;
- implementa banco de dados, cache, categorias, busca, ordenação sofisticada ou save system;
- desenha ou navega na interface.

O código fica em `firmware/game/library/` e usa somente a API pública do `Storage` e do `GamePackage`.

## Descoberta

A convenção oficial da V1 é:

```text
/games
```

O nome do diretório é sensível à caixa: `/Games` não é procurado.

Somente os filhos imediatos de `/games` são considerados. Subdiretórios **não** são percorridos. Um arquivo entra na biblioteca quando:

- é um arquivo, não um diretório;
- termina exatamente em `.ali`, com essa caixa;
- abre como um pacote `.ali` válido;
- possui manifesto válido e referências válidas a entrypoint e capa.

Exemplos:

```text
/games/mario.ali      → aceito
/games/puzzle.ali     → aceito
/games/GAME.ALI       → ignorado (extensão sensível à caixa)
/games/readme.txt     → ignorado
/games/backup.zip     → ignorado
/games/sub/deep.ali   → ignorado (sem recursão)
```

A ordem de `GameInfo[]` é a ordem de listagem do sistema de arquivos. A V1 não ordena por nome, autor ou versão. IDs duplicados não são removidos.

### Extensão e `GamePackage`

O filtro `.ali` é sensível à caixa porque `GamePackage::open()` também exige o sufixo minúsculo. Aceitar `.ALI` na biblioteca criaria uma entrada que o leitor não conseguiria abrir.

## API

```cpp
class GameLibrary {
public:
    static constexpr const char* DIRECTORY = "/games";
    static constexpr size_t MAX_GAMES = 16;
    static constexpr size_t MAX_INSTANCE_BYTES = 17500;

    explicit GameLibrary(Storage& storage);

    bool scan();
    bool refresh();

    size_t count() const;
    const GameInfo* get(size_t index) const;
    bool isTruncated() const;
};
```

`GameInfo` contém apenas o necessário para a biblioteca/UI:

```cpp
struct GameInfo {
    char id[64];
    char name[64];
    char version[32];
    char author[64];
    char packagePath[128]; // caminho no cartão, ex.: /games/mario.ali
    char coverPath[128];   // caminho da capa dentro do pacote
};
```

`coverPath` **não** é um caminho no cartão. Ele é a entrada interna do pacote que o manifesto declara em `cover`. A futura UI poderá abrir a capa com:

```text
GameInfo.coverPath
        ↓
GamePackage::open(packagePath)
        ↓
GamePackage::openFile(coverPath)
        ↓
Display
```

## Ciclo de vida

A biblioteca guarda um único `GamePackage` de vida longa:

```cpp
class GameLibrary {
    Storage& storage_;
    GamePackage package_;
    GameInfo games_[MAX_GAMES];
    size_t count_;
    bool truncated_;
};
```

Isso evita criar um `GamePackage` de ~9 KB na stack para cada jogo durante o `scan()`.

O `Console` possui a biblioteca, na ordem de dependência:

```text
Config
  ↓
Input / Display / Storage
  ↓
GameLibrary
  ↓
Boot
  ↓
HomeScreen
```

`Console::begin()` chama `scan()` depois do `Boot` e somente quando o Storage montou. Uma falha de descoberta é informada no Serial, mas não bloqueia o boot. A lista é acessível por `console.gameLibrary()`.

A tela principal ainda não consome a lista. A integração com a UI será feita pela futura `GamesScreen`.

## Comportamento de `scan()`

1. limpa a lista atual;
2. verifica se o Storage está montado;
3. abre `/games` como diretório;
4. percorre os filhos imediatos;
5. filtra arquivos `.ali`;
6. abre cada pacote e valida cabeçalho, índice, manifesto, entrypoint e capa;
7. copia os metadados para `GameInfo`;
8. fecha o pacote e o diretório;
9. continua mesmo quando um pacote é inválido.

Resultado:

```text
/games/
├── mario.ali      → válido   → entra
├── puzzle.ali     → válido   → entra
├── broken.ali     → inválido → ignorado
└── readme.txt     → ignorado
```

Estados de retorno:

| Situação | `scan()` | `count()` |
|---|---:|---:|
| Storage não montado | `false` | 0 |
| `/games` ausente | `false` | 0 |
| `/games` vazio | `true` | 0 |
| Só pacotes inválidos | `true` | 0 |
| Pacotes válidos | `true` | até `MAX_GAMES` |

Pacotes inválidos não geram entrada, quarentena ou reparo. Não há cache persistente.

### Limite

`MAX_GAMES` é **16**. Os primeiros 16 pacotes válidos, na ordem do diretório, entram na lista; os excedentes não são armazenados.

O `scan()` continua percorrendo o diretório depois de atingir `MAX_GAMES`, porque `isTruncated()` só pode responder se ainda existe **outro pacote válido**, e não apenas outro arquivo. Um `.ali` inválido depois dos 16 primeiros não caracteriza truncamento:

```text
16 válidos + 1 inválido   → isTruncated() == false
16 válidos + 1 válido     → isTruncated() == true
16 válidos + 10 inválidos → isTruncated() == false
```

Por isso o `break` ao atingir o limite não deve ser reintroduzido: ele responde "existe mais um `.ali`?" em vez de "existe mais um jogo válido?".

### Arquivos abertos

Ao final de `scan()`:

```text
Storage open files = 0
```

O diretório é fechado explicitamente e o `GamePackage` também. Os testes verificam `storage.openFileCount() == 0` após cada cenário. Caso uma pessoa use `scan()` diretamente, o `StorageDirectory` e o `GamePackage` continuam fechando por escopo/destruição mesmo em erro de pacote.

## Memória

O custo é um orçamento arquitetural, verificado por `static_assert`:

| Componente | Bytes |
|---|---:|
| `GameInfo` | 480 |
| `GameInfo[MAX_GAMES]` (16) | 7.680 |
| `GamePackage` | ~9.336 |
| `GameLibrary` (com contador e flag) | ~17.040 |

O build ESP32 atual usa ~46 KB de RAM estática, contra ~29 KB antes da Game Library. `scan()` usa adicionalmente ~4 KB de stack por causa do buffer de manifesto de `GamePackage::open()`.

Se `MAX_GAMES` aumentar, revise o orçamento, o `static_assert` e esta documentação. Não crie um `GamePackage` por jogo.

## Relação com o Storage

A descoberta exigiu uma extensão pequena e somente-leitura no HAL Storage:

```text
Storage::openDirectory(path, StorageDirectory&)
StorageDirectory::next(StorageEntry&)
StorageEntry { name[128], isDirectory }
```

`openDirectory()` aceita apenas diretórios e `next()` lista apenas filhos imediatos. O `StorageDirectory` ocupa um dos quatro slots de arquivo aberto do Storage, como um `StorageFile`; durante o `scan()`, o pico é o diretório mais o arquivo de manifesto.

Nenhuma escrita, remoção ou criação de diretório foi adicionada.

## Testes

Os testes nativos ficam em `test/game_library_test.cpp` e usam o fake em memória do Storage:

```bash
~/.platformio/penv/bin/pio run
bash test/run-game-package-tests.sh
```

Cobertura atual:

- Storage não montado;
- `/games` ausente, vazio e com caixa diferente;
- zero, um e vários jogos;
- `.ali` aceito; `.ALI`, `.txt`, `.zip` e subdiretórios ignorados;
- pacote corrompido, manifesto inválido e entrypoint ausente ignorados;
- limite `MAX_GAMES` com e sem excedentes válidos/inválidos, verificando `isTruncated()`;
- `refresh()` limpando resultados anteriores;
- `storage.openFileCount() == 0` ao final de cada `scan()`;
- orçamento de memória por `static_assert`.

### Limites de `GameInfo`

`GameInfo` é preenchido com `copyText()`, que trunca silenciosamente valores maiores que o campo. Hoje `GameManifest` e `GameInfo` têm limites independentes, então um `id` de manifesto com mais de 63 bytes vira um ID truncado na biblioteca. O truncamento é seguro contra overflow, mas ainda não há contrato entre os dois tipos. Alinhar os limites (especialmente `id`) é uma mudança separada, com decisão explícita entre rejeitar o manifesto ou ampliar `GameInfo`.


## Matriz física

**Status: pendente de execução com ESP32, cartão FAT32 e pinagem validada.** O fake cobre a lógica, mas não substitui a listagem real do SD.

| ID | Cenário | Resultado esperado |
|---|---|---|
| GL-01 | Iniciar sem cartão | `scan()` não é chamado; console continua |
| GL-02 | Cartão sem `/games` | `scan()` retorna falso; menu continua |
| GL-03 | `/games` vazio | `scan()` verdadeiro, `count() == 0` |
| GL-04 | Um `.ali` válido | `count() == 1`, metadados corretos |
| GL-05 | Válido + corrompido | `count() == 1`; corrompido ignorado |
| GL-06 | `readme.txt` e `.zip` em `/games` | ignorados |
| GL-07 | Subdiretório com `.ali` | ignorado, sem recursão |
| GL-08 | `MAX_GAMES + 1` pacotes válidos | 16 entradas e `isTruncated()` verdadeiro |
| GL-08b | `MAX_GAMES` válidos + `.ali` inválidos | 16 entradas e `isTruncated()` falso |
| GL-09 | Nome `GAME.ALI` | ignorado pela extensão sensível à caixa |
| GL-10 | Diretório `/Games` | não encontrado |
| GL-11 | Após o boot com cartão | Serial informa a contagem; UI segue responsiva |
| GL-12 | Pacote grande / muitos arquivos | `scan()` termina sem watchdog e sem vazamento de slots |

O Wokwi deste projeto não simula cartão microSD, então esses casos precisam de placa física.
