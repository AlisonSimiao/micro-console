# ADR-0005: Formato do Game Package

- Status: Aceito
- Data: 2026-11-08

## Contexto

O Game Package precisa validar um pacote de jogo armazenado no cartão SD e expor seus arquivos sem carregar o pacote inteiro na RAM. A API pública atual do Storage lê arquivos em blocos, mas não oferece busca aleatória. ZIP mantém seu índice central no final do arquivo e exigiria ampliar a API do Storage ou adicionar uma estratégia de busca aleatória.

O formato também precisa permitir limites previsíveis de memória e validação rigorosa de offsets e caminhos no ESP32.

## Decisão

Usar um container binário próprio `.ali`, sequencial e sem compressão na versão 1. O cabeçalho e o índice ficam no começo do arquivo. O índice contém caminhos relativos e intervalos absolutos dos conteúdos, permitindo que o leitor valide limites e localize arquivos sem carregar seus dados.

O manifesto fica em `manifest.json`, é codificado em JSON e limitado a 4096 bytes. ArduinoJson será usado para interpretá-lo. A versão V1 limita o pacote a 64 entradas e caminhos a 127 bytes; os detalhes do layout e das regras de validação estão em [Formato de pacote `.ali`](../game-package-format.md).

O Game Package depende somente da API pública do Storage. Ao abrir um arquivo interno, o leitor avança sequencialmente até seu offset e limita a leitura ao tamanho declarado. Nenhum código do jogo é executado.

## Consequências

- Não há dependência de descompressão nem leitura aleatória na V1.
- A memória usada para metadados é limitada pelos máximos definidos no formato, não pelo tamanho do pacote.
- Abrir um arquivo cujo payload esteja longe do início pode exigir descartar bytes sequencialmente; essa troca evita mudar o contrato do Storage.
- O formato próprio passa a ser um contrato mantido pelo projeto. Futuras mudanças incompatíveis exigem uma nova versão do container.
- ZIP ou compressão podem ser reavaliados em uma versão futura somente se houver necessidade concreta e suporte adequado à leitura incremental.
