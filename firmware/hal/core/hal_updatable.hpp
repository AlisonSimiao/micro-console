#pragma once

#include "hal/core/hal_module.hpp"

class hal_updatable : public hal_module {
public:
    virtual void update() = 0;
};
