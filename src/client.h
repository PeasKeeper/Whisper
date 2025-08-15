#ifndef CLIENT_H

#include <iostream>
#include <wchar.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <locale>
#include <codecvt>

const int BUFFER_SIZE = 1024;

std::wstring getHelpMsg();

#endif