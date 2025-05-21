#include "imap_handler.hpp"
#include <stdexcept> // For std::runtime_error
#include <iostream> // For std::cout
#include <sstream> // For std::istringstream

#include "logger.hpp"

// Constructor
IMAPHandler::IMAPHandler(const std::string& server, const std::string& port, const std::string& username, const std::string& password, long timeout, bool verbose)
    : curl(nullptr), server(server), port(port), username(username), password(password), verbose(verbose), timeout(timeout) {
}

// Destructor
IMAPHandler::~IMAPHandler() {
    disconnect(); // Disconnect from the server
    Logger::logger().info("IMAPHandler destroyed.");
}

// Initialize the connection
void IMAPHandler::initialize() {
    curl = curl_easy_init(); // Initialize CURL
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL.");
    }
    Logger::logger().debug("CURL initialized.");

    try {
        // Set connection parameters
        if (curl_easy_setopt(curl, CURLOPT_URL, ("imaps://" + server + ":" + port).c_str()) != CURLE_OK) {
            throw std::runtime_error("Failed to set CURL URL.");
        }
        if (curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str()) != CURLE_OK) {
            throw std::runtime_error("Failed to set CURL username.");
        }
        if (curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str()) != CURLE_OK) {
            throw std::runtime_error("Failed to set CURL password.");
        }
        if (curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL) != CURLE_OK) {
            throw std::runtime_error("Failed to set CURL SSL option.");
        }
        if (curl_easy_setopt(curl, CURLOPT_VERBOSE, verbose ? 1L : 0L) != CURLE_OK) {
            throw std::runtime_error("Failed to set CURL verbose option.");
        }

        // Set callback functions
        if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &IMAPHandler::write_callback) != CURLE_OK) {
            throw std::runtime_error("Failed to set CURL write callback.");
        }
        if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)this) != CURLE_OK) {
            throw std::runtime_error("Failed to set CURL write data.");
        }
        if (curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &IMAPHandler::header_callback) != CURLE_OK) {
            throw std::runtime_error("Failed to set CURL header callback.");
        }
        if (curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*)this) != CURLE_OK) {
            throw std::runtime_error("Failed to set CURL header data.");
        }

        // Set timeout options
        if (curl_easy_setopt(curl, CURLOPT_ACCEPTTIMEOUT_MS, timeout) != CURLE_OK) {
            throw std::runtime_error("Failed to set CURL timeout.");
        }
    } catch (const std::exception& e) {
        curl_easy_cleanup(curl); // Clean up CURL on error
        curl = nullptr; // Set CURL handle to null
        throw std::runtime_error("CURL initialization failed: " + std::string(e.what()));
    }
    
}

// Connect to the server
void IMAPHandler::connect() {
    CURLcode res = curl_easy_perform(curl); // Perform the connection
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to connect to server: " + std::string(curl_easy_strerror(res)));
    }
    Logger::logger().info("Connected to server.");
}

// Disconnect from the server
void IMAPHandler::disconnect() {
    if (curl) {
        Logger::logger().info("Disconnecting from server..."); // Log the disconnection
        curl_easy_cleanup(curl); // Clean up CURL
        curl = nullptr; // Set CURL handle to null
    }
}

// Perform a custom request to the IMAP server
Response IMAPHandler::perform_custom_request(const std::string cmd){
    Logger::logger().debug("Performing custom request: " + cmd); // Log the custom request

    // Set the command to be sent in the request
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, cmd.c_str()); // Set the custom request command

    // Reset the userdata and headerdata strings to avoid appending to old data
    userdata.clear();
    headerdata.clear();

    // Perform the request
    CURLcode res = curl_easy_perform(curl); // Perform the request
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to perform request: " + std::string(curl_easy_strerror(res)));
    }

    // Store the response data
    last_response.code = res; // Store the response code
    last_response.header = headerdata; // Store the header data
    last_response.data = userdata; // Store the userdata

    Logger::logger().debug("Response code: " + std::to_string(res)); // Log the response code
    Logger::logger().debug("Response header: " + headerdata); // Log the response header
    Logger::logger().debug("Response data: " + userdata); // Log the response data

    return last_response; // Return the response data
}

// ===================================
// Request functions
// ===================================
Response IMAPHandler::select(std::string mailbox){
    // Set the select command for the given mailbox
    std::string cmd = "SELECT " + mailbox; // Create the select command
    return perform_custom_request(cmd); // Perform the request and return the response
}

// Perform a raw search with the given criteria
Response IMAPHandler::raw_search(std::string criteria){
    // Set the search command
    std::string cmd = "UID SEARCH " + criteria; // Create the search command
    return perform_custom_request(cmd); // Perform the request and return the response
}

// Perform a search with the given criteria and return a vector of UIDs
std::vector<std::string> IMAPHandler::search(std::string criteria){
    // Perform the search request
    Response response = raw_search(criteria); // Perform the search request

    std::istringstream iss(response.data); // Create a string stream from the response data
    std::string word;

    // Skip the first part of the response until we find the word "SEARCH"
    while (iss >> word) {
        if(word == "SEARCH"){
            break;
        }
    }

    // Now we can read the UIDs of unseen emails
    std::vector<std::string> uids;
    std::string line;

    if (std::getline(iss, line)) { // Read only the first line
        std::istringstream line_stream(line);
        while (line_stream >> word) {
            uids.push_back(word); // Store the UID in the vector
        }
    }

    Logger::logger().debug("Found " + std::to_string(uids.size()) + " emails."); // Log the number of found emails
    for (const auto& uid : uids) {
        Logger::logger().debug("UID: " + uid); // Log the UID of each unseen email
    }

    return uids; // Return the vector of UIDs
}

// Perform a search for emails from the given sender and return a vector of UIDs
std::vector<std::string> IMAPHandler::search_from(std::string from){
    // Set the search criteria for unseen emails from the given sender
    std::string criteria = "FROM \"" + from + "\""; // Create the search criteria
    return search(criteria); // Perform the search and return the UIDs
}

// Perform a raw fetch with the given UID and data
Response IMAPHandler::raw_fetch(std::string uid, std::string data){
    // Set the fetch command
    std::string cmd = "UID FETCH " + uid + " " + data; // Create the fetch command
    return perform_custom_request(cmd); // Perform the request and return the response
}

Response IMAPHandler::fetch_internaldate(std::string uid){
    // Set the fetch command for internal date
    std::string cmd = "UID FETCH " + uid + " INTERNALDATE"; // Create the fetch command for internal date
    return perform_custom_request(cmd); // Perform the request and return the response
}


Response IMAPHandler::fetch_body(std::string uid, int part){
    if(part < 0){
        // Set the fetch command for body
        std::string cmd = "UID FETCH " + uid + " BODY[]"; // Create the fetch command for body
        return perform_custom_request(cmd); // Perform the request and return the response
    }
    else{
        std::string cmd = "UID FETCH " + uid + " BODY[" + std::to_string(part) + "]"; // Create the fetch command for body part
        return perform_custom_request(cmd); // Perform the request and return the response
    }
}


Response IMAPHandler::delete_uids(std::vector<std::string> uids){
    // Build string out of UIDs
    std::string uid_string = ""; // Initialize an empty string for UIDs
    for(int i = 0; i < uids.size(); i++){
        uid_string += uids[i]; // Append each UID to the string
        if(i != uids.size() - 1){
            uid_string += ","; // Add a comma between UIDs
        }
    }

    Logger::logger().debug("Deleting UIDs: " + uid_string); // Log the UIDs to be deleted

    // Set the delete command for the given UIDs
    std::string cmd = "UID STORE " + uid_string + " +FLAGS (\\Deleted)"; // Create the delete command
    perform_custom_request(cmd); // Perform the request and return the response

    // Expunge the deleted emails
    cmd = "EXPUNGE"; // Create the expunge command
    Logger::logger().info("Deleted emails and performed expunge.");
    return perform_custom_request(cmd); // Perform the request and return the response
}


// Callback function for writing data
size_t IMAPHandler::write_callback(char* ptr, size_t size, size_t nmemb, void* data) {
    if (ptr == nullptr || data == nullptr) {
        Logger::logger().error("Pointer or data pointer is null."); // Log error if pointer or data pointer is null
        return 0; // Return 0 if the pointer is null
    }
    size_t total_size = size * nmemb; // Calculate the total size of the data
    IMAPHandler* handler = static_cast<IMAPHandler*>(data); // Cast the data pointer to IMAPHandler
    handler->userdata.append(ptr, total_size); // Append the data to the userdata buffer

    return total_size; // Return the total size of the data
}

// Callback function for writing header data
size_t IMAPHandler::header_callback(char* buffer, size_t size, size_t nitems, void* data) {
    if (buffer == nullptr || data == nullptr) {
        Logger::logger().error("Buffer or data pointer is null."); // Log error if buffer or data pointer is null
        return 0; // Return 0 if the pointer is null
    }
    size_t total_size = size * nitems; // Calculate the total size of the header data
    IMAPHandler* handler = static_cast<IMAPHandler*>(data); // Cast the data pointer to IMAPHandler
    handler->headerdata.append(buffer, total_size); // Append the header data to the headerdata buffer
    
    return total_size; // Return the total size of the header data
}

// Setter and getter implementations
void IMAPHandler::set_verbose(bool verbose) {
    this->verbose = verbose;
}

std::string IMAPHandler::get_username() const {
    return username;
}

std::string IMAPHandler::get_password() const {
    return password;
}

std::string IMAPHandler::get_server() const {
    return server;
}

std::string IMAPHandler::get_port() const {
    return port;
}

bool IMAPHandler::get_use_ssl() const {
    return use_ssl;
}

bool IMAPHandler::get_verbose() const {
    return verbose;
}
