#pragma once

#include <cstddef>

// Pinos declarados pelos HALs antes da inicialização do hardware.
// configKey deve ter duração estática (literal de string).
class PinClaims {
public:
    static constexpr size_t MAX_CLAIMS = 32;

    PinClaims();

    bool add(int pin, const char* configKey);

private:
    struct Claim {
        int pin;
        const char* configKey;
    };

    Claim claims[MAX_CLAIMS];
    size_t count;
};
