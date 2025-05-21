#include "os.hpp"
#include "logger.hpp"

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <optional>

std::optional<std::string> os::copy_to_clipboard(const std::string& data){
    // Create a hidden window to act as the clipboard owner
    HWND hWnd = CreateWindowEx(
        0, "STATIC", "HiddenWindow", 0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, nullptr, nullptr);
    if (!hWnd) {
        Logger::logger().error("Failed to create hidden window for clipboard operations.");
        return std::nullopt;
    }

    while(!OpenClipboard(hWnd)) Sleep(1);
    Logger::logger().debug("Opened clipboard.");

    HANDLE cmem = GetClipboardData(CF_TEXT);
    if (cmem == nullptr) {
        CloseClipboard();
        DestroyWindow(hWnd);
        return std::nullopt;
    }

    char* pchData = static_cast<char*>(GlobalLock(cmem));
    if (pchData == nullptr) {
        CloseClipboard();
        DestroyWindow(hWnd);
        return std::nullopt;
    }

    std::string clip_text(pchData);
    GlobalUnlock(cmem);

    HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, data.length() + 1);
    if (hMem == nullptr) {
        CloseClipboard();
        DestroyWindow(hWnd);
        return std::nullopt;
    }

    void* pMem = GlobalLock(hMem);
    if (pMem == nullptr) {
        GlobalFree(hMem);
        CloseClipboard();
        DestroyWindow(hWnd);
        return std::nullopt;
    }

    memcpy(pMem, data.c_str(), data.length() + 1);
    GlobalUnlock(hMem);
    EmptyClipboard();
    if(!SetClipboardData(CF_TEXT, hMem)) {
        GlobalFree(hMem);
        CloseClipboard();
        DestroyWindow(hWnd);
        return std::nullopt;
    }

    // SetClipboardData will take ownership of hMem, so we don't free it here
    CloseClipboard();
    DestroyWindow(hWnd);

    Logger::logger().info("Data copied to clipboard.");
    return std::optional<std::string>(clip_text);
}

bool os::init(){
    return true;
}

void os::notify(const std::string& message, int delay) {
    MessageBox(nullptr, message.c_str(), "Notification", MB_OK);
}