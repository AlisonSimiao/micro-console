#pragma once

class hal_module {
public:
    virtual ~hal_module() = default;

    virtual bool begin() = 0;

    bool isInstantiated() const {
        return instantiated;
    }

    void setInstantiated(bool value) {
        instantiated = value;
    }

private:
    bool instantiated = false;
};
