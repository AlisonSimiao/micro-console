# System

`System` disponibiliza serviços básicos da plataforma para o firmware sem
espalhar dependências das APIs Arduino/ESP32 pelos módulos consumidores.
Nesta versão, oferece somente uptime e uma leitura pontual de heap livre.

## Responsabilidades e limites

O módulo consulta informações da plataforma. Não coordena módulos, participa
do boot, mantém estado próprio nem executa monitoramento periódico. A
composição da instância pertence ao `Console`.

Não fazem parte da versão atual: estado global, gerenciamento de energia,
reinicialização, delta time, FPS, frame limiter, scheduler, FreeRTOS, eventos,
logs centralizados, gerenciamento de exceções, monitoramento periódico de
memória ou métricas de PSRAM.

## API pública

```cpp
class System {
public:
    uint32_t getUptimeMs() const;
    uint32_t getFreeHeap() const;
};
```

- `getUptimeMs()` retorna diretamente o valor de `millis()` em milissegundos,
  contado desde a inicialização do dispositivo. Não mantém um timestamp próprio
  nem faz aritmética que altere o comportamento unsigned em rollover.
- `getFreeHeap()` retorna o valor de `ESP.getFreeHeap()` em bytes. A consulta
  não exige alocação dinâmica.

As dependências de plataforma ficam em `system.cpp`; a interface pública usa
somente tipos padrão de tamanho fixo.

## Integração com Console

`Console` possui uma instância de `System` por valor e a expõe por referência.
O `System` não requer `begin()` e não participa da sequência de inicialização.

```cpp
System& system = console.system();
uint32_t uptimeMs = system.getUptimeMs();
uint32_t freeHeap = system.getFreeHeap();
```

## Evoluções futuras

Outros serviços de plataforma poderão ser avaliados quando houver requisitos e
consumidores concretos. Estado global, controle do ciclo de vida de jogos e
serviços de execução pertencem a módulos próprios, não a esta primeira versão
do `System`.

## Validação

Execute o teste nativo com:

```bash
bash test/run-system-tests.sh
```

O teste nativo usa uma pequena implementação substituta das funções de
plataforma para validar delegação, valores em diferentes instantes (inclusive
próximo ao rollover de `uint32_t`) e leituras repetidas. O build PlatformIO
valida a compilação contra as APIs do framework ESP32. O teste nativo não
substitui validação de uptime/heap em hardware, que permanece pendente.
