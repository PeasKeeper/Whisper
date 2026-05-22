# pragma once

#include "Client.h"

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <string>
#include <vector>

namespace ftxui {

class UiManager {
    private:
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

        Element Bubble(const Message& message, int maxWidth);

        Component getInput(std::string& draft);
        Component getHistory(float& scrollY);

        bool addUserMessage(const std::string& author, const std::string& body);
        bool addSystemMessage(const std::string& body);

        float scrollY;

        Client &clientBackend;

        ScreenInteractive *screen;

    public:
        UiManager(Client& newClient);
        ~UiManager() = default;

        void run();
        void stop();

        void postUserMessage(std::string author, std::string body);
        void postSystemMessage(std::string body);
};

} // namespace ftxui
