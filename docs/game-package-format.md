# Formato de pacote `.ali`

`.ali` é um container de arquivos para jogos do micro console. O formato V1 é binário, sem compressão, e pode ser lido sequencialmente através da API pública do Storage. O Game Package aceita a extensão minúscula `.ali`, valida o magic e a estrutura do conteúdo, e expõe arquivos; não executa código JavaScript.

Todos os inteiros binários usam little-endian. Caminhos de entrada são relativos à raiz do pacote, sensíveis a maiúsculas/minúsculas e limitados a 127 bytes ASCII imprimíveis. Caminhos absolutos, `\`, segmentos vazios, `.` e `..` são inválidos. Entradas são arquivos, não diretórios.

## Cabeçalho

O cabeçalho tem 12 bytes:

| Offset | Tamanho | Campo | V1 |
|---:|---:|---|---|
| 0 | 4 | Magic | bytes `ALIP` |
| 4 | 2 | Versão do container | `1` |
| 6 | 2 | Quantidade de entradas | `1..64` |
| 8 | 4 | Tamanho do índice em bytes | tamanho exato dos registros seguintes |

O tamanho total do arquivo deve caber em 32 bits. O índice começa imediatamente após o cabeçalho e seus registros seguem na ordem em que foram gravados. O índice pode ocupar no máximo 8704 bytes.

## Índice

Cada registro tem tamanho variável:

| Tamanho | Campo |
|---:|---|
| 1 byte | Comprimento do caminho, entre 1 e 127 |
| variável | Caminho ASCII, sem terminador NUL |
| 4 bytes | Offset absoluto do conteúdo, desde o início do pacote |
| 4 bytes | Tamanho do conteúdo |

Offsets e tamanhos são `uint32` little-endian. Conteúdos devem começar após o cabeçalho e o índice e permanecer dentro do tamanho do pacote. Intervalos de conteúdo não podem se sobrepor. Entradas duplicadas são inválidas. Conteúdos vazios são permitidos.

Não há compressão ou alinhamento implícito na V1. Bytes não indexados entre o índice e os conteúdos são permitidos, assim como lacunas entre conteúdos. A extensão `.ali` não substitui a validação do magic e da estrutura.

## Manifesto

O caminho exato `manifest.json` é obrigatório. Seu conteúdo é UTF-8 JSON, com no máximo 4096 bytes e um objeto na raiz. Todos os campos abaixo são obrigatórios:

| Campo | Tipo | Regra |
|---|---|---|
| `id` | string | 1–63 bytes; somente `a-z`, `0-9`, `.`, `_` e `-` |
| `name` | string | 1–63 bytes |
| `version` | string | 1–31 bytes; versão do jogo |
| `packageVersion` | inteiro | Deve ser `1` e corresponder à versão do container |
| `author` | string | 1–63 bytes |
| `entrypoint` | string | Caminho relativo válido presente no índice |
| `cover` | string | Caminho relativo válido presente no índice |

Strings de metadados não podem conter NUL. Caminhos do manifesto seguem as mesmas regras dos caminhos do índice. O firmware não interpreta o JavaScript nem a imagem durante a validação.

Exemplo:

```json
{
  "id": "example-game",
  "name": "Example Game",
  "version": "1.0.0",
  "packageVersion": 1,
  "author": "Example Author",
  "entrypoint": "Codes/main.js",
  "cover": "Capa.bin"
}
```

## Limites e erros

- Máximo de 64 entradas.
- Caminho de entrada: máximo de 127 bytes.
- Índice: máximo de 8704 bytes.
- Manifesto: máximo de 4096 bytes.
- Offsets e tamanhos: 32 bits; o arquivo inteiro não pode exceder `UINT32_MAX`.
- Versões de container diferentes de V1 são incompatíveis e devem ser rejeitadas.
- Cabeçalho ou índice truncado, leitura curta de conteúdo, intervalo inválido/sobreposto, caminho inválido, manifesto inválido ou referência ausente tornam o pacote inválido.

O leitor mantém o índice e o manifesto limitados pelos valores acima, nunca carrega todos os conteúdos do pacote na RAM e limita cada leitura de arquivo ao intervalo indexado. A abertura de uma entrada avança sequencialmente pelo pacote até seu offset, pois o Storage não oferece `seek`. Uma leitura que encontra EOF antes do tamanho declarado fecha `PackageFile` e retorna os bytes que conseguiu ler; chamadas seguintes retornam zero.

Um `PackageFile` já aberto possui seu próprio `StorageFile` e permanece legível após `GamePackage::close()`. Ele fecha ao chamar `close()` ou ao ser destruído. O `Storage` deve continuar vivo durante toda a vida do `PackageFile`.
