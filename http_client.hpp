#pragma once

#include "net_connect.hpp"
#include "http_message.hpp"
#include <string>

HttpResponse http_get(const std::string& host, 
                     const std::string& path);
