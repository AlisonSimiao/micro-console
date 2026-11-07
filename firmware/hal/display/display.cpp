#include "hal/display/display.hpp"

#include <Arduino.h>
#include <LovyanGFX.hpp>

#include <cstddef>

namespace {

constexpr int PANEL_WIDTH = 240;
constexpr int PANEL_HEIGHT = 320;
constexpr uint32_t SPI_WRITE_FREQUENCY = 40000000;
constexpr uint8_t BACKLIGHT_CHANNEL = 0;
constexpr uint32_t BACKLIGHT_FREQUENCY = 5000;
constexpr uint8_t BACKLIGHT_RESOLUTION = 8;
constexpr int DISABLED_PIN = -1;

struct Settings {
    int sck;
    int mosi;
    int miso;
    int cs;
    int dc;
    int rst;
    int led;
};

struct PinDefinition {
    const char* configKey;
    int value;
    bool optional;
    bool requiresOutput;
};

class DisplayPanel : public lgfx::LGFX_Device {
public:
    DisplayPanel() : bus{}, panel{}
    {
        panel.setBus(&bus);
        setPanel(&panel);
    }

    bool begin(const Settings& settings)
    {
        auto busConfig = bus.config();
        // O SPI nativo (VSPI) usa GPIO 18/19, ocupados pelos botões UP e DOWN.
        busConfig.spi_host = SPI2_HOST;
        busConfig.spi_mode = 0;
        busConfig.freq_write = SPI_WRITE_FREQUENCY;
        // Painel 4-wire com DC dedicado; leitura não usada.
        busConfig.spi_3wire = false;
        busConfig.use_lock = true;
        busConfig.pin_sclk = static_cast<int16_t>(settings.sck);
        busConfig.pin_mosi = static_cast<int16_t>(settings.mosi);
        busConfig.pin_miso = static_cast<int16_t>(settings.miso);
        busConfig.pin_dc = static_cast<int16_t>(settings.dc);
        bus.config(busConfig);

        auto panelConfig = panel.config();
        panelConfig.pin_cs = static_cast<int16_t>(settings.cs);
        panelConfig.pin_rst = static_cast<int16_t>(settings.rst);
        panelConfig.pin_busy = DISABLED_PIN;
        panelConfig.memory_width = PANEL_WIDTH;
        panelConfig.memory_height = PANEL_HEIGHT;
        panelConfig.panel_width = PANEL_WIDTH;
        panelConfig.panel_height = PANEL_HEIGHT;
        panelConfig.readable = false;
        panel.config(panelConfig);

        return init();
    }

private:
    lgfx::Bus_SPI bus;
    lgfx::Panel_ILI9341 panel;
};

// Instância única: o objeto do painel vive somente neste arquivo.
DisplayPanel displayPanel;

Settings readSettings(const Config& config)
{
    Settings settings{};
    settings.sck = config.getInt("display.sck", 4);
    settings.mosi = config.getInt("display.mosi", 23);
    settings.miso = config.getInt("display.miso", DISABLED_PIN);
    settings.cs = config.getInt("display.cs", 27);
    settings.dc = config.getInt("display.dc", 32);
    settings.rst = config.getInt("display.rst", DISABLED_PIN);
    settings.led = config.getInt("display.led", 33);
    return settings;
}

bool isValidGpioPin(int pin)
{
    return (pin >= 0 && pin <= 5) ||
           (pin >= 12 && pin <= 19) ||
           (pin >= 21 && pin <= 23) ||
           (pin >= 25 && pin <= 27) ||
           (pin >= 32 && pin <= 39);
}

bool isOutputCapable(int pin)
{
    // GPIO 34–39 são somente entrada.
    return isValidGpioPin(pin) && pin < 34;
}

bool isPinInUse(const PinDefinition& pin)
{
    return !pin.optional || pin.value != DISABLED_PIN;
}

bool validatePins(const Settings& settings)
{
    // configKey, value, optional, requiresOutput
    const PinDefinition pins[] = {
        {"display.sck", settings.sck, false, true},
        {"display.mosi", settings.mosi, false, true},
        {"display.miso", settings.miso, true, false},
        {"display.cs", settings.cs, false, true},
        {"display.dc", settings.dc, false, true},
        {"display.rst", settings.rst, true, true},
        {"display.led", settings.led, true, true},
    };
    constexpr size_t PIN_COUNT = sizeof(pins) / sizeof(pins[0]);

    for (size_t i = 0; i < PIN_COUNT; ++i) {
        const PinDefinition& pin = pins[i];
        if (!isPinInUse(pin)) {
            continue;
        }

        if (!isValidGpioPin(pin.value)) {
            Serial.print("Display: invalid pin for ");
            Serial.print(pin.configKey);
            Serial.print(": ");
            Serial.println(pin.value);
            return false;
        }

        if (pin.requiresOutput && !isOutputCapable(pin.value)) {
            Serial.print("Display: pin requires output capability for ");
            Serial.print(pin.configKey);
            Serial.print(": ");
            Serial.println(pin.value);
            return false;
        }
    }

    for (size_t i = 0; i < PIN_COUNT; ++i) {
        const PinDefinition& first = pins[i];
        if (!isPinInUse(first)) {
            continue;
        }

        for (size_t j = i + 1; j < PIN_COUNT; ++j) {
            const PinDefinition& second = pins[j];
            if (!isPinInUse(second)) {
                continue;
            }

            if (first.value == second.value) {
                Serial.print("Display: pin conflict between ");
                Serial.print(first.configKey);
                Serial.print(" and ");
                Serial.println(second.configKey);
                return false;
            }
        }
    }

    return true;
}

uint8_t readRotation(const Config& config)
{
    const int value = config.getInt("display.rotation", 0);
    if (value >= 0 && value <= 3) {
        return static_cast<uint8_t>(value);
    }

    const int clamped = value < 0 ? 0 : 3;
    Serial.print("Display: rotation out of range (");
    Serial.print(value);
    Serial.print("), using ");
    Serial.println(clamped);
    return static_cast<uint8_t>(clamped);
}

uint8_t readBrightness(const Config& config)
{
    const int value = config.getInt("display.brightness", 255);
    if (value < 0) {
        return 0;
    }

    if (value > 255) {
        return 255;
    }

    return static_cast<uint8_t>(value);
}

void logPins(const Settings& settings)
{
    Serial.print("Display: pins SCK=");
    Serial.print(settings.sck);
    Serial.print(" MOSI=");
    Serial.print(settings.mosi);
    Serial.print(" MISO=");
    Serial.print(settings.miso);
    Serial.print(" CS=");
    Serial.print(settings.cs);
    Serial.print(" DC=");
    Serial.print(settings.dc);
    Serial.print(" RST=");
    Serial.print(settings.rst);
    Serial.print(" LED=");
    Serial.println(settings.led);
}

} // namespace

Display::Display(Config& config)
    : config(config), backlightPin(DISABLED_PIN), rotation(0), brightness(255) {}

bool Display::begin()
{
    Serial.println("Display: initializing");

    const Settings settings = readSettings(config);
    if (!validatePins(settings)) {
        Serial.println("Display: invalid configuration");
        return false;
    }

    rotation = readRotation(config);
    brightness = readBrightness(config);
    backlightPin = settings.led;

    logPins(settings);
    Serial.print("Display: panel ILI9341 ");
    Serial.print(PANEL_WIDTH);
    Serial.print("x");
    Serial.print(PANEL_HEIGHT);
    Serial.print(", rotation=");
    Serial.print(rotation);
    Serial.print(", brightness=");
    Serial.println(brightness);

    if (!displayPanel.begin(settings)) {
        Serial.println("Display: panel initialization failed");
        return false;
    }

    displayPanel.setRotation(rotation);

    if (backlightPin == DISABLED_PIN) {
        Serial.println("Display: backlight control disabled, using fixed 3V3");
    } else {
        const uint8_t backlightGpio = static_cast<uint8_t>(backlightPin);
        if (!ledcAttachChannel(backlightGpio, BACKLIGHT_FREQUENCY, BACKLIGHT_RESOLUTION, BACKLIGHT_CHANNEL)) {
            Serial.println("Display: failed to attach backlight PWM; backlight left at fixed 3V3");
            backlightPin = DISABLED_PIN;
        } else {
            ledcWrite(backlightGpio, brightness);
        }
    }

    setInstantiated(true);
    Serial.print("Display: initialized ");
    Serial.print(width());
    Serial.print("x");
    Serial.println(height());
    return true;
}

int Display::width() const
{
    return (rotation % 2 == 0) ? PANEL_WIDTH : PANEL_HEIGHT;
}

int Display::height() const
{
    return (rotation % 2 == 0) ? PANEL_HEIGHT : PANEL_WIDTH;
}

void Display::clear(uint16_t color)
{
    if (!isInstantiated()) {
        return;
    }

    displayPanel.fillScreen(color);
}

void Display::fillRect(int x, int y, int w, int h, uint16_t color)
{
    if (!isInstantiated()) {
        return;
    }

    displayPanel.fillRect(x, y, w, h, color);
}

void Display::drawText(int x, int y, const char* text, uint16_t color, uint8_t size)
{
    if (!isInstantiated() || text == nullptr) {
        return;
    }

    displayPanel.setTextDatum(lgfx::textdatum_t::top_left);
    displayPanel.setTextColor(color);
    displayPanel.setTextSize(size);
    displayPanel.drawString(text, x, y);
}

void Display::setBrightness(uint8_t level)
{
    brightness = level;

    if (backlightPin == DISABLED_PIN || !isInstantiated()) {
        return;
    }

    ledcWrite(static_cast<uint8_t>(backlightPin), level);
}

uint8_t Display::getRotation() const
{
    return rotation;
}

uint8_t Display::getBrightness() const
{
    return brightness;
}
