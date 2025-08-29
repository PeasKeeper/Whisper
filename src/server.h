#pragma once

#include <iostream>
#include <set>

const int BUFFER_SIZE = 1024;

std::wstring getHelpMsg();

void getClientMessage (const int client_fd, const std::set<int>* clients);