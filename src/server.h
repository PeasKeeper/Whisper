#pragma once

#include <iostream>
#include <set>

const int BUFFER_SIZE = 1024;

std::wstring getHelpMsg();

void handleClient (const int clientFd);

void stopSignalHandler(int signum);