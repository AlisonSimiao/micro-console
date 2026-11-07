#pragma once

#include "hal/core/pin_claims.hpp"

class hal_module {
public:
    virtual ~hal_module() = default;

    virtual bool begin() = 0;

    // Declara os GPIOs do módulo antes de qualquer begin(); não toca no hardware.
    virtual bool claimPins(PinClaims& claims) const
    {
        (void)claims;
        return true;
    }

    bool isInstantiated() const {
        return instantiated;
    }

    void setInstantiated(bool value) {
        instantiated = value;
    }

private:
    bool instantiated = false;
};
