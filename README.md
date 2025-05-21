# Token Daemon

This is a C++ application, that polls your email inbox for one-time tokens from a specified email-address and automatically retrieves and copies them to your clipboard (if possible).

## Features

- Polls an email inbox for one-time tokens
- Filters by sender address
- Automatically copies tokens to clipboard (if supported)
- Configurable via source/header files

## Requirements

- CMake (version 3.15 or higher recommended)
- UCRT MinGW (or another C++ toolchain)
- g++ compiler (C++17 or newer)
- Git (to clone the repository and dependencies)

## Dependencies

- libcurl (for IMAP/email access)
- [Conan](https://conan.io/) (recommended for dependency management)
- Windows: `libcurl.dll` and its dependencies must be available in your PATH or next to the executable

## Installation

1. **Clone the repository:**
   ```powershell
   git clone <repository-url>
   cd Email-Daemon
   ```


2. **Install dependencies (optional):**
   - If you have [Conan](https://conan.io/) installed (optional, may not work in all environments):
     ```powershell
     conan install . --output-folder=build --build=missing
     ```
   - If Conan does not work or you prefer not to use it, manually download and place `libcurl.dll` and its dependencies in your build or system path.


3. **Configure the application:**
   - Copy `include/define.h.template` to `include/define.h` and fill in your email credentials and settings.

4. **Build the project:**
   ```powershell
   mkdir build
   cd build
   cmake ..
   cmake --build .
   ```


5. **Run the application:**
   - The executables will be in the `build` directory (e.g., `build\bin\TokenDaemon_daemon.exe`).
   - Two executables are created: one with console output `TokenDaemon_console.exe` and one without `TokenDaemon_daemon.exe`(for background use).
   - Make sure `libcurl.dll` is accessible.
   - To have the daemon start automatically with your PC, configure the desired executable to autostart (e.g., add a shortcut to the Windows Startup folder or use Task Scheduler).

## Usage

- The application will poll your inbox for emails from the specified address and extract one-time tokens.
- Tokens are copied to your clipboard if possible (Windows clipboard support is included).


## Notes

- This project is intended for personal use. Be careful with your credentials.
- Do not commit your filled `define.h` with sensitive data to version control.
- For Linux or macOS, you may need to adapt the clipboard and build logic.
- The token regex pattern in the code may need to be adjusted to match the format of your one-time tokens.
- Different email providers and clients may handle email formatting differently (tested primarily with web.de). You may need to adapt the code or configuration for your specific provider.

## Todo

- [ ] Add support for OAuth2 authentication
- [ ] Add cross-platform clipboard support

## License

MIT License (see `LICENSE` file for details)