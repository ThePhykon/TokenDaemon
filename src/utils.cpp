#include "logger.hpp"
#include "utils.hpp"

#include <string>
#include <vector>
#include <sstream>
#include <optional>
#include <stdexcept>
#include <openssl/bio.h>
#include <openssl/evp.h>


std::string base64::decode(const std::string &encoded_string) {
    BIO *bio, *b64;
    std::vector<char> buffer(encoded_string.size());

    // Base64-Dekodierung vorbereiten
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // Keine Zeilenumbrüche beachten
    bio = BIO_new_mem_buf(encoded_string.data(), encoded_string.size());
    bio = BIO_push(b64, bio);

    // Dekodierung durchführen
    int decoded_length = BIO_read(bio, buffer.data(), encoded_string.size());
    if (decoded_length < 0) {
        BIO_free_all(bio);
        throw std::runtime_error("Fehler bei der Base64-Dekodierung");
    }

    std::string result(buffer.data(), decoded_length);
    BIO_free_all(bio);
    return result;
}


std::string base64::extract_base64_from_email(const std::string &input){
    // Assuming the input is the email body containing the base64 string
    std::istringstream stream(input);
    std::string line;
    std::string res;
    std::optional<std::string> temp = std::nullopt;

    // Copy everything except the first and last line of the email body
    while(std::getline(stream, line)){
        if(!temp.has_value()){
            // Skip the first line
            std::getline(stream, line);
            temp = line; // Store the first line for later use
        }
        else {
            res += temp.value() + "\n"; // Append the previous line to the result
            temp = line;
        }
    }

    return res; // Return the result string
}