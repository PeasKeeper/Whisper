#pragma once

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <functional>
#include <string>
#include <vector>
#include <mutex>
#include <queue>

class AppManager;

namespace ftxui {

class UiManager {
    private:
        using SendMessageCallback = std::function<void(std::string)>;

        enum class MessageType {
            Incoming,
            Own,
            System
        };

        struct Message {
            std::string author;
            std::string body;
            MessageType type;
        };

        std::vector<Message> messageHistory;

        Element renderMessageRow(const Message& message, int maxWidth);
        Element renderEmojiPicker(const std::vector<std::string>& emojiList, int selectedEmoji);

        Component getInput(std::string& draft, int& draftCursor);
        Component getHistory(float& scrollY);

        bool addUserMessage(const std::string& author, const std::string& body);
        bool addSystemMessage(const std::string& body);

        std::mutex pendingMessagesMutex;
        std::queue<Message> pendingMessages;
        void processPendingMessages();

        float historyScrollY;

        AppManager &appManager;
        SendMessageCallback onSendMessage;

        ScreenInteractive screen;

    public:
        UiManager(AppManager &newAppManager);
        ~UiManager() = default;

        void run();
        void stop();

        void setSendMessageCallback(SendMessageCallback callback);

        void postUserMessage(std::string author, std::string body);
        void postSystemMessage(std::string body);
};

} // namespace ftxui
