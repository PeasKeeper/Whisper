#include "UiManager.h"

#include <StringUtils.h>

#include <algorithm>

#include <ftxui/component/app.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <mutex>

namespace ftxui {

namespace {

int maxBubbleWidth() {
    const int terminalWidth = Terminal::Size().dimx;
    // outer app border: 2
    // history frame: 2
    // scroll indicator: ~ 1
    // some additional space: 2
    const int availableWidth = std::max(1, terminalWidth - 7);

    if (availableWidth < 16) {
        return availableWidth;
    }

    return std::clamp(availableWidth / 2, 16, 64);
}

int maxSystemMessageWidth() {
    return std::max(1, Terminal::Size().dimx - 10);
}

} // namespace

UiManager::UiManager(Client& newClient) : clientBackend(newClient) {
    messageHistory = {};
    historyScrollY = 1.0F;
    screen = nullptr;
}

void UiManager::run() {
    std::string draft;
    int draftCursor = 0;

    auto messageInput = getInput(draft, draftCursor);
    auto chatHistory = getHistory(historyScrollY);

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
                text("> ") | color(Color::GrayLight),
                messageInput->Render() |
                    yframe |
                    size(HEIGHT, LESS_THAN, 5) |
                    flex,
            }),
        }) | border;
    });

    app = CatchEvent(app, [&](Event event) {
        if (event == Event::Custom) {
            processPendingMessages();
            return true;
        }

        if (event == Event::CtrlN) {
            draftCursor = std::clamp(
                draftCursor,
                0,
                static_cast<int>(draft.size())
            );

            draft.insert(static_cast<std::size_t>(draftCursor), "\n");
            ++draftCursor;
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
            draftCursor = 0;
            historyScrollY = 1.0F;
            return true;
        }

        if (event == Event::PageUp) {
            historyScrollY += (-0.25F);
            return true;
        }

        if (event == Event::PageDown) {
            historyScrollY += (0.25F);
            return true;
        }

        if (event == Event::ArrowUp) {
            historyScrollY += (-0.05F);
            return true;
        }

        if (event == Event::ArrowDown) {
            historyScrollY += (0.05F);
            return true;
        }

        if (event.is_mouse() && event.mouse().button == Mouse::WheelUp) {
            historyScrollY += (-0.05F);
            return true;
        }

        if (event.is_mouse() && event.mouse().button == Mouse::WheelDown) {
            historyScrollY += (0.05F);
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

void UiManager::postUserMessage(std::string author, std::string body) {
    std::unique_lock<std::mutex> lock(pendingMessagesMutex);
    pendingMessages.push({
        .author = std::move(author),
        .body = std::move(body),
        .type = MessageType::Incoming
    });
    lock.unlock();
    if (screen != nullptr) {
        screen->PostEvent(Event::Custom);
    }
}

void UiManager::postSystemMessage(std::string body) {
    std::unique_lock<std::mutex> lock(pendingMessagesMutex);
    pendingMessages.push({
        .author = "",
        .body = std::move(body),
        .type = MessageType::System
    });
    lock.unlock();
    if (screen != nullptr) {
        screen->PostEvent(Event::Custom);
    }
}

Element UiManager::renderMessageRow(const Message& message, int maxWidth) {
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
        const int systemWidth = maxSystemMessageWidth();

        auto widthFn = [](const std::string& text) {
            return string_width(text);
        };

        auto splitFn = [](const std::string& text) {
            return Utf8ToGlyphs(text);
        };

        Element systemMessage =
            paragraph(StringUtils::wrapText(message.body, systemWidth, widthFn, splitFn)) |
            color(Color::GrayDark) |
            size(WIDTH, LESS_THAN, systemWidth);

        return hbox({
            filler(),
            systemMessage,
            filler()
        });
    }

    auto widthFn = [](const std::string& text) {
        return string_width(text);
    };

    auto splitFn = [](const std::string& text) {
        return Utf8ToGlyphs(text);
    };

    const int bodyWidth = std::max(1, maxWidth - 2);
    Element content = vbox({
        text(message.author) | bold,
        separator(),
        paragraph(StringUtils::wrapText(message.body, bodyWidth, widthFn, splitFn)),
    });

    const int minBubbleWidth = std::min(8, maxWidth);

    Element bubble = content |
                     size(WIDTH, GREATER_THAN, minBubbleWidth) |
                     size(WIDTH, LESS_THAN, maxWidth) |
                     borderStyled(LIGHT, borderColor);

    if (message.type == MessageType::Own) {
        return hbox({filler(), bubble});
    }

    return hbox({bubble, filler()});
}

Component UiManager::getInput(std::string& draft, int& draftCursor) {
    InputOption inputOptions;
    inputOptions.multiline = true;
    inputOptions.cursor_position = &draftCursor;
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
                renderMessageRow(message, maxBubbleWidth()) | xflex
            );
        }

        return vbox(rows) |
               focusPositionRelative(0.0F, scrollY) |
               vscroll_indicator |
               frame;
    });
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
    historyScrollY = 1.0F;
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
    historyScrollY = 1.0F;
    return true;
}

void UiManager::processPendingMessages() {
    std::queue<Message> messages;

    std::unique_lock<std::mutex> lock(pendingMessagesMutex);
    std::swap(messages, pendingMessages);
    lock.unlock();

    while (!messages.empty()) {
        if (messages.front().type == MessageType::Incoming) {
            addUserMessage(messages.front().author, messages.front().body);
        }
        else if (messages.front().type == MessageType::System) {
            addSystemMessage(messages.front().body);
        }
        messages.pop();
    }
}

} // namespace ftxui
