#pragma once
#if !HAS_SCREEN && !MESHTASTIC_EXCLUDE_CANNEDMESSAGES

#include "Observer.h"
#include "SinglePortModule.h"
#include "input/InputBroker.h"

#include <Arduino.h>
#include <vector>

class HeadlessCannedMessageModule : public SinglePortModule
{
  public:
    HeadlessCannedMessageModule();

    bool isSelecting() const { return selecting; }
    bool hasMessages() const { return !messages.empty(); }

    AdminMessageHandleResult handleAdminMessageForModule(const meshtastic_MeshPacket &mp, meshtastic_AdminMessage *request,
                                                         meshtastic_AdminMessage *response) override;

  private:
    static constexpr uint8_t kMaxMessages = 8;
    static constexpr uint32_t kSelectionTimeoutMs = 10000;

    std::vector<String> messages;
    bool selecting = false;
    int currentIndex = -1;
    uint32_t lastInteraction = 0;

    void loadConfig();
    void saveConfig();
    void installDefaultConfig();
    void splitConfiguredMessages();

    void enterSelection();
    void exitSelection(bool playTone = true);
    void advanceSelection();
    bool sendCurrentMessage();
    bool sendText(const String &message);

    int handleInputEvent(const InputEvent *event);

    CallbackObserver<HeadlessCannedMessageModule, const InputEvent *> inputObserver =
        CallbackObserver<HeadlessCannedMessageModule, const InputEvent *>(this, &HeadlessCannedMessageModule::handleInputEvent);
};

extern HeadlessCannedMessageModule *headlessCannedMessageModule;

#endif
