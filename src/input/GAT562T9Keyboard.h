#pragma once

#include "TCA8418KeyboardBase.h"

class GAT562T9Keyboard : public TCA8418KeyboardBase
{
  public:
    GAT562T9Keyboard();

  protected:
    void pressed(uint8_t key) override;
    void released() override;

  private:
    uint8_t lastKey = UINT8_MAX;
    uint8_t charIndex = 0;
    uint32_t lastTap = 0;
    uint32_t pressedAt = 0;
    bool replacePrevious = false;
};
