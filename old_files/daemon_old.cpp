#include <iostream>
#include <string>
#include <stdexcept>
#include <array>
#include <vector>
#include <sstream>
#include <regex>
#include <iomanip>
#include <optional>

#include "windows.h"
#include "daemon.hpp"
#include "curl/curl.h"
#include "defines.h"

// Global variables
std::string userdata; // This will hold the data received from the server
std::string headerdata; // This will hold the header data received from the server

size_t write_callback_string(char *ptr, size_t size, size_t nmemb, std::string *data);
size_t header_callback(char* buffer, size_t size, size_t nitems, std::string* data);

std::optional<std::string> copy_to_clipboard(const std::string text); // Function to copy text to clipboard
std::optional<std::string> restore_clipboard(const std::string old_clipboard); // Function to restore the clipboard content

Response perform_request(CURL* curl, const std::string& data = "") {
    // Set the data to be sent in the request
    if(data != ""){
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, data.c_str());
    }

    // Reset the userdata string to avoid appending to old data
    userdata.clear();
    headerdata.clear(); // Clear the header data as well

    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to perform request: " + std::string(curl_easy_strerror(res)));
    }

    Response response;
    response.code = res; // Store the response code
    response.header = headerdata; // Store the header data
    response.data = userdata; // Store the userdata

    return response; // Return the response data
}

void sanity_check() {
    // Check if the required defines are set
    if (IMAP_SERVER == nullptr || IMAP_PORT == 0 || IMAP_URL == nullptr || IMAP_USERNAME == nullptr || IMAP_PASSWORD == nullptr) {
        throw std::runtime_error("One or more required defines are not set.");
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL.");
    }

    // Ping test to check if the server is reachable
    CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, TEST_URL);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to set CURLOPT_URL: " + std::string(curl_easy_strerror(res)));
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nullptr); // Set a null write function to ignore the response
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // Set to HEAD request to avoid downloading the body

    res = curl_easy_perform(curl); // Perform the request to check connectivity
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to perform request: " + std::string(curl_easy_strerror(res)));
    }
    std::cout << "Sanity check passed. Server is reachable." << std::endl;
}

std::vector<std::string> fetch_new_emails(CURL* curl) {
    // Set the request type to SEARCH for unseen emails
    std::string cmd = "UID SEARCH FROM \"" TARGET_MAIL_ADDRESS "\"";
    Response response = perform_request(curl, cmd);
    
    std::istringstream iss(response.data); // Create a string stream from the response data
    std::string wort;

    // Skip the first part of the response until we find the word "SEARCH"
    while (iss >> wort) {
        if(wort == "SEARCH"){
            break;
        }
    }

    // Now we can read the UIDs of unseen emails
    std::vector<std::string> uids;
    std::string line;
    if (std::getline(iss, line)) { // Read only the first line
        std::istringstream line_stream(line);
        while (line_stream >> wort) {
            uids.push_back(wort); // Store the UID in the vector
        }
    }

    std::cout << "Found " << uids.size() << " emails." << std::endl;
    for (const auto& uid : uids) {
        std::cout << "UID: " << uid << std::endl; // Print the UID of each unseen email
    }

    return uids; // Return the vector of UIDs
}

CURL* setup_connection(){
    // Print all given defines
    std::cout << "IMAP_SERVER: " << IMAP_SERVER << std::endl;
    std::cout << "IMAP_PORT: " << IMAP_PORT << std::endl;
    std::cout << "IMAP_URL: " << IMAP_URL << std::endl;
    std::cout << "IMAP_USERNAME: " << IMAP_USERNAME << std::endl;
    std::cout << "IMAP_PASSWORD: " << IMAP_PASSWORD << std::endl;

    CURL* curl = curl_easy_init();
    CURLcode res;

    res = curl_easy_setopt(curl, CURLOPT_URL, IMAP_URL);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to set CURLOPT_URL: " + std::string(curl_easy_strerror(res)));
    }

    res = curl_easy_setopt(curl, CURLOPT_USERNAME, IMAP_USERNAME);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to set CURLOPT_USERNAME: " + std::string(curl_easy_strerror(res)));
    }

    res = curl_easy_setopt(curl, CURLOPT_PASSWORD, IMAP_PASSWORD);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to set CURLOPT_PASSWORD: " + std::string(curl_easy_strerror(res)));
    }

    res = curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to set CURLOPT_USE_SSL: " + std::string(curl_easy_strerror(res)));
    }

    res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to set CURLOPT_VERBOSE: " + std::string(curl_easy_strerror(res)));
    }

    res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_string);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to set CURLOPT_WRITEFUNCTION: " + std::string(curl_easy_strerror(res)));
    }

    res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&userdata);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to set CURLOPT_WRITEDATA: " + std::string(curl_easy_strerror(res)));
    }

    res = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to set CURLOPT_HEADERFUNCTION: " + std::string(curl_easy_strerror(res)));
    }

    res = curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&headerdata);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to set CURLOPT_HEADERDATA: " + std::string(curl_easy_strerror(res)));
    }

    res = curl_easy_setopt(curl, CURLOPT_ACCEPTTIMEOUT_MS, 360000L); // 3 minutes timeout
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to set CURLOPT_ACCEPTTIMEOUT_MS: " + std::string(curl_easy_strerror(res)));
    }

    return curl;
}

bool check_timestamp(CURL* curl, std::string uid){
    Response response = perform_request(curl, "UID FETCH " + uid + " INTERNALDATE");
    std::cout << "Checking timestamp for UID: " << uid << std::endl;
    std::cout << "Response: " << response.data << std::endl;
    std::cout << "Response (Header): " << response.header << std::endl;

    std::regex code_regex(R"(\bINTERNALDATE\s\"(\d{2}-[A-Za-z]{3}-\d{4}\s\d{2}:\d{2}:\d{2}\s[+-]\d{4})\")"); // Regex to match the date and time
    std::smatch match;

    if (!std::regex_search(response.data, match, code_regex)) {
        return false; // No match found, return false
    }

    std::string timestamp = match.str(1); // Extract the timestamp from the match
    std::string timezone = match.str(2);  // Zeitzone (z.B. +0000)
    std::cout << "Timestamp found: " << timestamp << std::endl;

    // Convert the timestamp to a time_t object
    std::tm tm = {};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm, "%d-%b-%Y %H:%M:%S"); // Since mingw does not support %z, we will not parse the timezone => assume it's UTC
    if (ss.fail()) {
        std::cerr << "Failed to parse timestamp: " << timestamp << std::endl;
        return false; // Failed to parse the timestamp, return false
    }

    std::time_t email_time = std::mktime(&tm); // Convert to time_t
    std::cout << "Email time (UTC): " << std::put_time(std::gmtime(&email_time), "%Y-%m-%d %H:%M:%S") << std::endl;
    
    // Get the current UTC time as a time_t object
    std::time_t now = std::time(nullptr);

    // Convert email_time to UTC if necessary (assuming it is already in UTC)
    // Compare the email timestamp with the current time
    double diff = std::difftime(now, email_time);

    // Check if the email is within the last 5 minutes
    if (diff <= TIME_DIFFERENCE && diff >= 0) {
        std::cout << "Email is recent (within the last 5 minutes)." << std::endl;
        return true;
    } else {
        std::cout << "Email is not recent." << std::endl;
    }


    return false; // Placeholder for timestamp check logic
}

std::optional<std::string> check_email(const std::string& uid, CURL* curl) {
    std::cout << "Checking email with UID: " << uid << std::endl;

    std::string cmd = "UID FETCH " + uid + " BODY.PEEK[]";
    Response response = perform_request(curl, cmd);
    std::cout << "Checked email successfully." << std::endl;

    // Parse the response to find the token
    std::regex code_regex(R"(\*\b[0-9]{6}\b\*)"); // Regex to match a 6-digit number enclosed with * *
    std::smatch match;

    if (std::regex_search(response.header, match, code_regex)) {
        std::string token = match.str(0); // Extract the token from the match
        std::cout << "Token found: " << token << std::endl;
        // Remove the * from the token
        token.erase(std::remove(token.begin(), token.end(), '*'), token.end()); // Remove * from the token
        std::cout << "Token without *: " << token << std::endl;
        return token; // Return the token if found
    }

    return std::nullopt; // Return nullopt if no token is found
}

std::optional<std::string> copy_to_clipboard(const std::string data){
    while(!OpenClipboard(nullptr)) Sleep(1);

    HANDLE cmem = GetClipboardData(CF_TEXT);
    if (cmem == nullptr) {
        CloseClipboard();
        return std::nullopt;
    }

    char* pchData = static_cast<char*>(GlobalLock(cmem));
    if (pchData == nullptr) {
        CloseClipboard();
        return std::nullopt;
    }

    std::string clip_text(pchData);
    GlobalUnlock(cmem);

    HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, data.length() + 1);
    memcpy(GlobalLock(hMem), data.c_str(), data.length() + 1);
    GlobalUnlock(hMem);
    OpenClipboard(nullptr);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();

    return std::optional<std::string>(clip_text);
}

void cleanup(CURL* curl, std::vector<std::string> uids){

    // Mark emails as DELETED
    for(std::string uid : uids){
        std::cout << "MARKING " + uid + " AS DELETED!";
        std::string cmd = "UID STORE " + uid + " +FLAGS (\\Deleted)";
        perform_request(curl, cmd);
    }

    // Perform EXPUNGE
    std::string cmd = "EXPUNGE";
    perform_request(curl, cmd);
}

int main_loop() {
    _putenv("TZ=UTC"); // Set the timezone to UTC

    std::cout << "Starting daemon..." << std::endl;
    std::cout << curl_version() << std::endl;

    sanity_check();
    
    // Setting up connection
    CURL* curl = setup_connection();
    
    // Check connection
    perform_request(curl);

    // Updating url to inbox
    std::string url = IMAP_URL "INBOX/"; // Fetch all emails from INBOX
    CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // Entering loop
    while (true) {
        // Check emails for token
        std::vector<std::string> uids = fetch_new_emails(curl);
        std::vector<std::string> uids_copy = uids; // Copy the uids vector to avoid modifying it while iterating
        
        while(!uids_copy.empty()) {
            // Find latest email and check for token
            std::string uid = uids_copy.back(); // Get the last UID in the vector

            if (check_timestamp(curl, uid)) {
                std::optional<std::string> token = check_email(uid, curl);
                if (token.has_value()) {
                    std::cout << "Token found: " << *token << std::endl;
                    // Copy the token to clipboard
                    std::optional<std::string> old_clipboard = copy_to_clipboard(token.value());

                    //TODO: Notify user that token has been copied!
                    
                    if(!old_clipboard.has_value()){
                        std::cerr << "Unable to copy toke to clipboard!" << std::endl;
                    }

                    Sleep(10000); // Give user 10 seconds to paste token
                    copy_to_clipboard(old_clipboard.value()); // Restore the old clipboard content
                    break; // Exit the loop after copying the token
                } else {
                    std::cout << "No token found in email." << std::endl;
                }
            }

            uids_copy.pop_back();
            std::cout << "UIDS-LEN: " << uids_copy.size() << std::endl;
        }

        cleanup(curl, uids);
            
        Sleep(30000); // Sleep before checking again
    }

    return 0;
}

size_t write_callback_string(char *ptr, size_t size, size_t nmemb, std::string *data){
    // This function will be called by libcurl as soon as there is data received that needs to be saved.
    // The size of the data is size * nmemb, and the data itself is in ptr.
    // You can save it to a file or process it as needed.
    data->append(std::string(ptr, size * nmemb)); // Save the data to the string

    return size * nmemb; // Return the number of bytes processed
}

size_t header_callback(char* buffer, size_t size, size_t nitems, std::string* data) {
    // This function will be called by libcurl to process header data.
    // The size of the data is size * nitems, and the data itself is in buffer.
    // Append the header data to the userdata string for further processing if needed.
    data->append(std::string(buffer, size * nitems));

    return size * nitems; // Return the number of bytes processed
}