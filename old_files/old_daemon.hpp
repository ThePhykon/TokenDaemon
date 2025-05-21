#pragma once

#include <curl/curl.h>
#include <string>

class Response {
public:
    Response() : code(CURLE_OK), header(""), data("") {}
    Response(CURLcode code, const std::string& headerdata, const std::string& userdata)
        : code(code), header(headerdata), data(userdata) {}
    CURLcode code;
    std::string header;
    std::string data;
};