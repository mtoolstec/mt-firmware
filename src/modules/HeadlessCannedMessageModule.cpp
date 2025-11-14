#include "HeadlessCannedMessageModule.h"
#if !HAS_SCREEN && !MESHTASTIC_EXCLUDE_CANNEDMESSAGES

#include "Channels.h"
#include "Led.h"
#include "MeshService.h"
#include "NodeDB.h"
#include "SPILock.h"
#include "buzz.h"
#include "configuration.h"
#include "mesh/generated/meshtastic/cannedmessages.pb.h"
#include "mesh/generated/meshtastic/mesh.pb.h"

#if !MESHTASTIC_EXCLUDE_EXTERNALNOTIFICATION
#include "modules/ExternalNotificationModule.h"
#endif

#include <algorithm>

static constexpr const char *kCannedConfigPath = "/prefs/cannedConf.proto";
static meshtastic_CannedMessageModuleConfig cannedMessageModuleConfig;

HeadlessCannedMessageModule *headlessCannedMessageModule = nullptr;

HeadlessCannedMessageModule::HeadlessCannedMessageModule()
    : SinglePortModule("canned-headless", meshtastic_PortNum_TEXT_MESSAGE_APP)
{
    moduleConfig.canned_message.enabled = true;
    moduleConfig.has_canned_message = true;

    loadConfig();
    splitConfiguredMessages();

    if (inputBroker)
        inputObserver.observe(inputBroker);
}

AdminMessageHandleResult HeadlessCannedMessageModule::handleAdminMessageForModule(const meshtastic_MeshPacket &mp,
                                                                                  meshtastic_AdminMessage *request,
                                                                                  meshtastic_AdminMessage *response)
{
    switch (request->which_payload_variant) {
    case meshtastic_AdminMessage_get_canned_message_module_messages_request_tag:
        if (mp.decoded.want_response) {
            response->which_payload_variant = meshtastic_AdminMessage_get_canned_message_module_messages_response_tag;
            strncpy(response->get_canned_message_module_messages_response, cannedMessageModuleConfig.messages,
                    sizeof(response->get_canned_message_module_messages_response));
            return AdminMessageHandleResult::HANDLED_WITH_RESPONSE;
        }
        break;
    case meshtastic_AdminMessage_set_canned_message_module_messages_tag:
        if (*request->set_canned_message_module_messages) {
            strncpy(cannedMessageModuleConfig.messages, request->set_canned_message_module_messages,
                    sizeof(cannedMessageModuleConfig.messages));
            splitConfiguredMessages();
            saveConfig();
        }
        return AdminMessageHandleResult::HANDLED;
    default:
        break;
    }

    return AdminMessageHandleResult::NOT_HANDLED;
}

void HeadlessCannedMessageModule::loadConfig()
{
    if (nodeDB->loadProto(kCannedConfigPath, meshtastic_CannedMessageModuleConfig_size,
                          sizeof(meshtastic_CannedMessageModuleConfig), &meshtastic_CannedMessageModuleConfig_msg,
                          &cannedMessageModuleConfig) != LoadFileResult::LOAD_SUCCESS) {
        installDefaultConfig();
    }
}

void HeadlessCannedMessageModule::saveConfig()
{
#ifdef FSCom
    spiLock->lock();
    FSCom.mkdir("/prefs");
    spiLock->unlock();
#endif

    nodeDB->saveProto(kCannedConfigPath, meshtastic_CannedMessageModuleConfig_size, &meshtastic_CannedMessageModuleConfig_msg,
                      &cannedMessageModuleConfig);
}

void HeadlessCannedMessageModule::installDefaultConfig()
{
    strncpy(cannedMessageModuleConfig.messages, "Hi|Bye|Yes|No|Ok|Help|On my way|Need assistance",
            sizeof(cannedMessageModuleConfig.messages));
}

void HeadlessCannedMessageModule::splitConfiguredMessages()
{
    messages.clear();
    messages.reserve(kMaxMessages);

    String raw = cannedMessageModuleConfig.messages;

    // 如果配置为空，使用默认消息
    if (raw.length() == 0) {
        installDefaultConfig();
        raw = cannedMessageModuleConfig.messages;
    }

    int start = 0;

    // 严格限制最多解析 8 条消息
    while (messages.size() < kMaxMessages) {
        int sep = raw.indexOf('|', start);
        String token = (sep == -1) ? raw.substring(start) : raw.substring(start, sep);
        token.trim();

        // 只添加非空消息，并确保不超过 8 条
        if (token.length() > 0 && messages.size() < kMaxMessages) {
            messages.push_back(token);
        }

        if (sep == -1)
            break;
        start = sep + 1;
    }

    // 如果解析后还是空的（理论上不应该发生），再次尝试默认配置
    if (messages.empty()) {
        LOG_WARN("Canned messages empty after parsing, using defaults\n");
        installDefaultConfig();
        splitConfiguredMessages();
        return;
    }

    // 确保索引有效
    currentIndex = -1;

    LOG_INFO("Loaded %d canned messages (max %d)\n", messages.size(), kMaxMessages);
}

void HeadlessCannedMessageModule::enterSelection()
{
    if (!hasMessages())
        return;

    selecting = true;
    currentIndex = -1;
    lastInteraction = millis();

    // 进入选择模式：关闭心跳，常亮LED
    ledBlink.set(false);
    ledForceOn.set(true);

    playCannedModeStart();
}

void HeadlessCannedMessageModule::exitSelection(bool playTone)
{
    selecting = false;
    currentIndex = -1;

    // 退出选择模式：恢复心跳，取消强制常亮
    ledForceOn.set(false);
    // ledBlink会由系统的ledBlinker()自动恢复

    if (playTone) {
        playCannedModeExit();
    }
}

void HeadlessCannedMessageModule::advanceSelection()
{
    if (!selecting || messages.empty())
        return;

    // 循环到下一条消息，确保索引在有效范围内
    int maxIndex = std::min((int)messages.size(), (int)kMaxMessages);
    currentIndex = (currentIndex + 1) % maxIndex;
    lastInteraction = millis();

    // LED闪烁一次（关-开-关）
    ledForceOn.set(false);
    delay(50);
    ledForceOn.set(true);

    // 改为播放 3 位二进制（点=短，划=长）来表示 000..111 的索引
    if (currentIndex >= 0 && currentIndex < kMaxMessages) {
        playBinaryIndexTone(currentIndex);
    }
}

bool HeadlessCannedMessageModule::sendCurrentMessage()
{
    // 严格检查：必须在选择模式、索引有效、且消息存在
    if (!selecting || messages.empty())
        return false;

    if (currentIndex < 0 || currentIndex >= (int)messages.size() || currentIndex >= kMaxMessages)
        return false;

    if (sendText(messages[currentIndex])) {
        // 播放独立的发送反馈音效（酷炫上升序列）
        playCannedMessageSentTone();
        return true;
    }

    return false;
}

bool HeadlessCannedMessageModule::sendText(const String &message)
{
    if (!service || message.length() == 0)
        return false;

    meshtastic_MeshPacket *p = allocDataPacket();
    if (!p)
        return false;

    p->to = NODENUM_BROADCAST;
    p->channel = channels.getPrimaryIndex();
    p->want_ack = true;

    const bool bell = moduleConfig.canned_message.send_bell;
    const size_t maxLen = bell ? meshtastic_Constants_DATA_PAYLOAD_LEN - 1 : meshtastic_Constants_DATA_PAYLOAD_LEN;
    size_t len = std::min(message.length(), maxLen);
    memcpy(p->decoded.payload.bytes, message.c_str(), len);
    p->decoded.payload.size = len;

    if (bell && p->decoded.payload.size < meshtastic_Constants_DATA_PAYLOAD_LEN) {
        p->decoded.payload.bytes[p->decoded.payload.size++] = 7;
    }

    service->sendToMesh(p, RX_SRC_LOCAL, true);
    return true;
}

int HeadlessCannedMessageModule::handleInputEvent(const InputEvent *event)
{
    if (!hasMessages())
        return 0;

    if (selecting && (millis() - lastInteraction) > kSelectionTimeoutMs) {
        exitSelection();
    }

    switch (event->inputEvent) {
    case INPUT_BROKER_HEADLESS_CANNED_MODE:
        if (selecting)
            exitSelection();
        else
            enterSelection();
        return 1;
    case INPUT_BROKER_USER_PRESS:
        if (selecting) {
            advanceSelection();
            return 1;
        }
        break;
    case INPUT_BROKER_HEADLESS_CANNED_SEND:
        if (selecting && sendCurrentMessage()) {
            exitSelection(false);
            return 1;
        }
        break;
    default:
        break;
    }

    return 0;
}

#endif
