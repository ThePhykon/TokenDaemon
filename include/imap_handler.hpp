#pragma once

#include <string>
#include <vector>
#include <ctime> // For std::tm
#include "curl/curl.h"

class Response {
public:
    CURLcode code; // Response code
    std::string header; // Header data
    std::string data; // Response data

    // Constructor
    Response(CURLcode code = CURLE_OK, const std::string& header = "", const std::string& data = "") : code(code), header(header), data(data) {}
};

// IMAP handler
class IMAPHandler {
private:
    // Connection parameters
    CURL* curl; // CURL handle
    const std::string server; // IMAP server address
    const std::string port; // IMAP server port
    const std::string username; // Username for the IMAP server
    const std::string password; // Password for the IMAP server

    // Connection settings
    long timeout; // Timeout for the connection
    bool use_ssl; // Use SSL for the connection
    bool verbose; // Verbose output for debugging

    // Callback functions
    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* handler); // Callback for writing data
    static size_t header_callback(char* buffer, size_t size, size_t nitems, void* handler); // Callback for writing header data

    // Buffers
    std::string userdata; // Buffer for received data
    std::string headerdata; // Buffer for received header data
    Response last_response; // Last response from the server

public:
    // Constructor
    IMAPHandler(const std::string& server, const std::string& port, const std::string& username, const std::string& password, long timeout=36000L, bool verbose = false);

    // Destructor
    ~IMAPHandler();

    // Initialize the connection
    void initialize();
    void connect();
    void disconnect();

    // Request functions
    Response select(std::string mailbox);
    Response raw_search(std::string criteria);
    std::vector<std::string> search(std::string criteria);
    std::vector<std::string> search_from(std::string from);

    Response raw_fetch(std::string uid, std::string data);
    Response fetch_internaldate(std::string uid);
    Response fetch_body(std::string uid, int part = -1);

    Response delete_uids(std::vector<std::string> uids);

    // Perform a request to the IMAP server
    Response perform_custom_request(const std::string cmd);

    // Setter and getter functions
    void set_verbose(bool verbose);
    void set_debug(bool debug);
    std::string get_username() const;
    std::string get_password() const;
    bool get_use_ssl() const;
    bool get_verbose() const;
    bool get_debug() const;
    std::string get_server() const;
    std::string get_port() const;
};