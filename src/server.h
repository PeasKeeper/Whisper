#ifndef SERVER_H

#include <iostream>
#include <wchar.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "IPv4Addr.h"
#include <locale>
#include <codecvt>

const int BUFFER_SIZE = 1024;

std::wstring getHelpMsg();

#endif