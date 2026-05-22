#include "UiManager.h"

#include <ftxui/component/app.hpp>
#include <ftxui/screen/color.hpp>

namespace ftxui {

namespace {

int MaxInputWidth() {
    return std::max(10, Terminal::Size().dimx - 5);
}

} // namespace

UiManager::UiManager(Client& newClient) : clientBackend(newClient) {
    messageHistory = {};
    scrollY = 1.0F;
    screen = nullptr;
}

void UiManager::run() {
    std::string draft;

    auto messageInput = getInput(draft);
    auto chatHistory = getHistory(scrollY);

    messageHistory.push_back({
        .author = "",
        .body = "You can list existing groups by typing /LSGRP\n"
        "You can make a new group by typing /NEWGRP group_name password\n"
        "You can join an existing group by typing /JOINGRP group_name password\n"
        "You can leave a group by typing /LEAVEGRP",
        .type = MessageType::System
    });

    auto app = Renderer(messageInput, [&] {
        return vbox({
            chatHistory->Render() | flex,
            separator(),
            hbox({
                text("> "),
                messageInput->Render() | flex,
            }),
        }) | border;
    });

    app = CatchEvent(app, [&](Event event) {
        if (event == Event::Custom) {
            return true;
        }

        if (event == Event::Return) {
            if (draft.empty()) {
                return true;
            }

            if (draft.find_first_not_of(" \n\t\r") == std::string::npos) {
                return true;
            }

            messageHistory.push_back({
                .author = "Me",
                .body = draft,
                .type = MessageType::Own
            });
            clientBackend.queueMessage(draft);
            draft.clear();
            scrollY = 1.0F;
            return true;
        }

        if (event == Event::PageUp) {
            scrollY += (-0.25F);
            return true;
        }

        if (event == Event::PageDown) {
            scrollY += (0.25F);
            return true;
        }
        return false;
    });

    auto localScreen = ScreenInteractive::Fullscreen();
    screen = &localScreen;

    localScreen.TrackMouse(true);
    localScreen.Loop(app);

    screen = nullptr;
}

void UiManager::stop() {
    if (screen != nullptr) {
        screen->Exit();
    }
}

bool UiManager::addUserMessage(const std::string& author, const std::string& body) {
    if (author.empty() || body.empty()) {
        return false;
    }

    if (author.find_first_not_of(" \n\t\r") == std::string::npos ||
        body.find_first_not_of(" \n\t\r") == std::string::npos) {
        return false;
    }

    messageHistory.push_back({
        .author = author,
        .body = body,
        .type = MessageType::Incoming
    });
    scrollY = 1.0F;
    return true;
}

bool UiManager::addSystemMessage(const std::string& body) {
    if (body.empty()) {
        return false;
    }

    if (body.find_first_not_of(" \n\t\r") == std::string::npos) {
        return false;
    }

    messageHistory.push_back({
        .author = "",
        .body = body,
        .type = MessageType::System
    });
    scrollY = 1.0F;
    return true;
}

void UiManager::postUserMessage(std::string author, std::string body) {
    addUserMessage(author, body);
    if (screen != nullptr) {
        screen->PostEvent(Event::Custom);
    }
}

void UiManager::postSystemMessage(std::string body) {
    addSystemMessage(body);
    if (screen != nullptr) {
        screen->PostEvent(Event::Custom);
    }
}

Element UiManager::Bubble(const Message& message, int maxWidth) {
    Color borderColor;
    if (message.type == MessageType::Own) {
        borderColor = Color::Blue;
    }
    else if (message.type == MessageType::Incoming) {
        borderColor = Color::DarkBlue;
    }
    else {
        borderColor = Color::GrayDark;
    }

    if (message.type == MessageType::System) {
        return hbox({
            filler(),
            text(message.body) | color(Color::GrayDark),
            filler()
        });
    }

    Element content = vbox({
        text(message.author) | bold | flex,
        separator() | flex,
        paragraph(message.body) | flex,
    });

    Element bubble = content | size(WIDTH, LESS_THAN, maxWidth) |
                               borderStyled(LIGHT, borderColor);

    if (message.type == MessageType::Own) {
        return hbox({filler(), bubble});
    }

    return hbox({bubble, filler()});
}

Component UiManager::getInput(std::string& draft) {
    InputOption inputOptions;
    inputOptions.multiline = true;
    inputOptions.transform = [](InputState state) {
      if (state.is_placeholder) {
        state.element |= dim;
      }

      return state.element;
    };

    auto input = Input(&draft, "Type a message", inputOptions);
    return input;
}

Component UiManager::getHistory(float& scrollY) {
    return Renderer([&] {
        Elements rows;

        for (const Message& message : messageHistory) {
            rows.push_back(
                Bubble(message, MaxInputWidth())
            );
        }

        return vbox(rows) |
               focusPositionRelative(0.0F, scrollY) |
               vscroll_indicator |
               frame;
    });
}

} // namespace ftxui
