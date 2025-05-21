#pragma once

#include <string>
#include <optional>

namespace os {
    const std::string os_name = "Windows"; // Operating system name

    std::optional<std::string> copy_to_clipboard(const std::string& data); // Function to copy data to clipboard
    void notify(const std::string& message, int delay = 10); // Function to send a notification
    bool init();
}