#include "system/input/input.hpp"

class Boot {
public:
    Boot();
    bool begin();
private:
    Input input;
    void initializeHardware();
};