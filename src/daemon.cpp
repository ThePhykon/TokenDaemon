#include "imap_handler.hpp"
#include "os.hpp"
#include "utils.hpp"
#include "logger.hpp"
#include "defines.h"

#include <string>
#include <vector>
#include <stdexcept>
#include <regex>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <optional>

IMAPHandler* handler; // Global IMAP handler object


bool check_timestamp(std::string uid){
    Response res = handler->fetch_internaldate(uid); // Fetch the internal date of the email
    Logger::logger().debug("Checking timestamp for UID: " + uid); // Log the UID being checked

    std::regex code_regex(R"(\bINTERNALDATE\s\"(\d{2}-[A-Za-z]{3}-\d{4}\s\d{2}:\d{2}:\d{2}\s[+-]\d{4})\")"); // Regex to match the date and time
    std::smatch match; // Match object to store the result of regex search

    if (!std::regex_search(res.data, match, code_regex)) { // Search for the regex pattern in the response data
        Logger::logger().error("Failed to find timestamp in response data."); // Log error if timestamp is not found
        return false; // Return false if timestamp is not found
    }

    Logger::logger().debug("Timestamp found: " + match.str(1)); // Log the found timestamp
    std::string timestamp = match.str(1); // Extract the timestamp from the match

    std::tm tm = {}; // Initialize a tm structure to hold the parsed time
    std::istringstream ss(timestamp); // Create a string stream from the timestamp string
    ss >> std::get_time(&tm, "%d-%b-%Y %H:%M:%S"); // Parse the timestamp into the tm structure

    // Check if parsing failed
    if (ss.fail()) {
        Logger::logger().error("Failed to parse timestamp: " + timestamp); // Log error if parsing fails
        return false; // Return false if parsing fails
    }

    std::time_t email_time = std::mktime(&tm); // Convert the tm structure to time_t
    Logger::logger().debug("Email time (UTC): " + std::to_string(email_time)); // Log the email time in UTC

    // Get the current UTC time as a time_t object
    std::time_t now = std::time(nullptr); // Get the current time
    Logger::logger().debug("Current time (UTC): " + std::to_string(now)); // Log the current time in UTC

    // Calculate the time difference between now and the email time
    double diff = std::difftime(now, email_time); // Calculate the difference in seconds
    Logger::logger().debug("Time difference: " + std::to_string(diff)); // Log the time difference

    // Check if the email is within the last TIME_DIFFERENCE seconds
    if (diff <= TIME_DIFFERENCE && diff >= 0) { // Check if the email is recent
        Logger::logger().info("Email is recent (within the last " + std::to_string(TIME_DIFFERENCE) + " seconds)."); // Log if the email is recent
        return true; // Return true if the email is recent
    } else {
        Logger::logger().info("Email is not recent."); // Log if the email is not recent
    }
    return false; // Return false if the email is not recent
}

std::optional<std::string> get_token(std::string uid){
    Response res = handler->fetch_body(uid, 1); // Fetch the body of the email
    Logger::logger().debug("Checking email with UID: " + uid); // Log the UID being checked

    // Decoding email body from base64
    std::string base64_body = base64::extract_base64_from_email(res.header); // Extract the base64 encoded body from the email
    Logger::logger().debug("Base64 encoded email body: " + base64_body); // Log the base64 encoded email body
    std::string decoded_body = base64::decode(base64_body); // Decode the base64 body
    Logger::logger().debug("Decoded email body: " + decoded_body); // Log the decoded email body

    std::regex code_regex(R"(<p><b>(\d{6})</b></p>)"); // Regex to match a 6-digit number enclosed with * *
    std::smatch match;

    if (std::regex_search(decoded_body, match, code_regex)) {
        Logger::logger().info("Token found: " + match.str(1)); // Log the found token
        std::string token = match.str(1); // Extract the token from the match
        return token; // Return the token if found
    }

    return std::nullopt; // Return nullopt if no token is found
}   

void main_loop() {
    handler->connect(); // Connect to the IMAP server
    Logger::logger().info("Connected to IMAP server."); // Log connection to the server

    // Selecting INBOX
    handler->select("INBOX");
    Logger::logger().info("Selected INBOX."); // Log selection of INBOX

    while(true) {
        Logger::logger().debug("Checking for new emails..."); // Log the start of email checking
        std::vector<std::string> uids = handler->search_from(TARGET_MAIL_ADDRESS); // Search for unseen emails from the target address
        std::vector<std::string> uids_copy = uids; // Copy the uids vector to avoid modifying it while iterating

        if(uids.empty()) {
            Logger::logger().debug("No new emails found."); // Log if no new emails are found
        } else {
            Logger::logger().debug("Found " + std::to_string(uids.size()) + " new emails."); // Log the number of new emails found
        }

        // Iterate through UIDs
        while(!uids_copy.empty()) {
            std::string uid = uids_copy.back();
            if(check_timestamp(uid)) {
                std::optional<std::string> token = get_token(uid); // Get the token from the email
                if(token.has_value()) {
                    std::optional<std::string> old_clipboard;

                    for(int i = 0; i < CLIPBOARD_RETRY; i++){
                        old_clipboard = os::copy_to_clipboard(token.value());
                        
                        if(old_clipboard.has_value()){
                            break;
                        }

                        Sleep(2);
                    }

                    if(!old_clipboard.has_value()) {
                        Logger::logger().error("Failed to copy token to clipboard."); // Log error if copying fails
                        
                        os::notify("Unable to copy token to clipboard! Token: " + token.value());
                        break; // Exit the loop if copying fails
                    }

                    Logger::logger().warning("Token copied to clipboard: " + token.value()); // Log success if token is copied
                    os::notify("Token copied!");

                    Sleep(10000); // Give user 10 seconds to paste token
                    os::copy_to_clipboard(old_clipboard.value()); // Restore the old clipboard content
                    Logger::logger().warning("Clipboard restored."); // Log restoration of clipboard
                    break; // Exit the loop after copying the token

                } else {
                    Logger::logger().error("No token found in email."); // Log error if no token is found
                }
            } else {
                Logger::logger().info("Email is not recent."); // Log if the email is not recent
            }

            uids_copy.pop_back(); // Remove the processed UID from the vector
        }

        if(!uids.empty()) {
            handler->delete_uids(uids); // Delete the processed emails
            Logger::logger().warning("Deleted processed emails."); // Log deletion of processed emails
        }
        
        Logger::logger().debug("Waiting for " + std::to_string(POLLING_INTERVAL / 1000) + " seconds before checking again..."); // Log the wait time
        Sleep(POLLING_INTERVAL); // Wait for the polling interval before checking again
    }
}

void init() {
    bool verbose = false; // Set verbose mode to false

    // Set verbose mode based on DEBUG_ACTIVATE
    #if DEBUG_ACTIVATE
        verbose = true; // Set verbose mode to true if DEBUG is activated
    #endif

    handler = new IMAPHandler(IMAP_SERVER, std::to_string(IMAP_PORT), IMAP_USERNAME, IMAP_PASSWORD, 36000L, verbose); // Initialize the IMAP handler
    while(true) {
        try {
            handler->initialize(); // Initialize the connection
            break; // Break the loop if connection is successful
        } catch (const std::exception& e) {
            Logger::logger().error("Initialization failed: " + std::string(e.what())); // Log connection failure
            Sleep(1000); // Wait before retrying
        }
    }

    os::init();
}

int run(){
    // Set log level to DEBUG if DEBUG is defined
    #if DEBUG_ACTIVATE
        Logger::logger().set_log_level(DEBUG);
    #else
        Logger::logger().set_log_level(INFO);
    #endif

    // Set env variable for timezone
    _putenv("TZ=UTC"); // Set the timezone to UTC

    Logger::logger().info("Starting daemon..."); // Log the start of the daemon
    Logger::logger().info("IMAP_SERVER: " + std::string(IMAP_SERVER)); // Log the IMAP server
    Logger::logger().info("IMAP_PORT: " + std::to_string(IMAP_PORT)); // Log the IMAP port
    Logger::logger().info("IMAP_URL: " + std::string(IMAP_URL)); // Log the IMAP URL
    Logger::logger().info("IMAP_USERNAME: " + std::string(IMAP_USERNAME)); // Log the IMAP username
    Logger::logger().info("IMAP_PASSWORD: " + std::string(IMAP_PASSWORD)); // Log the IMAP password
    // Log the current UTC time
    time_t now = time(nullptr);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S UTC", gmtime(&now));
    Logger::logger().info("Current UTC time: " + std::string(buffer));

    // Running the main loop in a try-catch block to handle exceptions
    while(true) {
        init(); // Initialize the IMAP handler
        Logger::logger().info("IMAP handler initialized."); // Log the initialization of the IMAP handler

        try {
            main_loop(); // Enter the main loop
        }

        catch (const std::exception& e) {
            Logger::logger().error("Error: " + std::string(e.what())); // Log any errors that occur
        }
        catch (...) {
            Logger::logger().error("Unknown error occurred."); // Log unknown errors
        }

        Sleep(1000); // Wait before retrying the main loop

        // Resetting handler
        delete handler;
    }

    delete handler; // Clean up the IMAP handler
    return 0; // Return success
}