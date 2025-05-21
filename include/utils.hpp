#pragma once

#include <string>
#include <vector>

namespace base64
{
    // Encodes a string to base64 format.
    std::string decode(const std::string &input);
    std::string extract_base64_from_email(const std::string &input);
} // namespace base64