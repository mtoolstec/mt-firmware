#include "GAT562T9Keyboard.h"

#include <Arduino.h>

namespace
{
constexpr uint8_t rows = 5;
constexpr uint8_t columns = 3;
constexpr uint32_t multiTapMs = 750;
constexpr uint32_t longPressMs = 2000;

constexpr char keyMap[12][9] = {{',', '.', '!', '?', '<', '>', '1'},
                                {'a', 'b', 'c', 'A', 'B', 'C', '2'},
                                {'d', 'e', 'f', 'D', 'E', 'F', '3'},
                                {'g', 'h', 'i', 'G', 'H', 'I', '4'},
                                {'j', 'k', 'l', 'J', 'K', 'L', '5'},
                                {'m', 'n', 'o', 'M', 'N', 'O', '6'},
                                {'p', 'q', 'r', 's', 'P', 'Q', 'R', 'S', '7'},
                                {'t', 'u', 'v', 'T', 'U', 'V', '8'},
                                {'w', 'x', 'y', 'z', 'W', 'X', 'Y', 'Z', '9'},
                                {'*', '+'},
                                {' ', '0'},
                                {'#', '^'}};
constexpr uint8_t keyLengths[12] = {7, 7, 7, 7, 7, 7, 9, 7, 9, 2, 2, 2};
} // namespace

GAT562T9Keyboard::GAT562T9Keyboard() : TCA8418KeyboardBase(rows, columns) {}

void GAT562T9Keyboard::pressed(uint8_t key)
{
    if (state != Idle || key == 0) {
        return;
    }

    const uint8_t row = (key - 1) / 10;
    const uint8_t column = (key - 1) % 10;
    if (row >= rows || row == 3 || column >= columns) {
        return;
    }

    const uint8_t index = (row < 3 ? row : 3) * columns + column;
    const uint32_t now = millis();
    replacePrevious = index == lastKey && now - lastTap < multiTapMs;
    charIndex = replacePrevious ? charIndex + 1 : 0;
    lastKey = index;
    pressedAt = now;
    state = Held;
}

void GAT562T9Keyboard::released()
{
    if (state != Held) {
        return;
    }

    const uint32_t now = millis();
    if (replacePrevious) {
        queueEvent(BSP);
    }
    queueEvent(keyMap[lastKey][now - pressedAt >= longPressMs ? 0 : charIndex % keyLengths[lastKey]]);
    lastTap = now;
    state = Idle;
}
