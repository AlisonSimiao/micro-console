#include <Arduino.h>

#include <cassert>
#include <cstdint>

#include "system/system.hpp"

namespace {

uint32_t fakeMillis = 0;
uint32_t fakeFreeHeap = 0;

} // namespace

EspClass ESP;

uint32_t millis()
{
    return fakeMillis;
}

uint32_t EspClass::getFreeHeap() const
{
    return fakeFreeHeap;
}

int main()
{
    const System system;

    fakeMillis = 1234;
    assert(system.getUptimeMs() == 1234);

    fakeMillis = UINT32_MAX - 2;
    assert(system.getUptimeMs() == UINT32_MAX - 2);

    fakeMillis = 1;
    assert(system.getUptimeMs() == 1);
    assert(system.getUptimeMs() == 1);

    fakeFreeHeap = 32768;
    assert(system.getFreeHeap() == 32768);
    fakeFreeHeap = 16384;
    assert(system.getFreeHeap() == 16384);
}
