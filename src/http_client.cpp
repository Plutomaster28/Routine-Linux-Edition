#include "http_client.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace discord {

namespace {

struct CurlGlobalState {
    CurlGlobalState() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    // Intentionally process-lifetime. HttpClient instances can be owned by
    // globals in other translation units, whose destruction order is not
    // defined relative to this guard.
    ~CurlGlobalState() = default;
};

CurlGlobalState curl_global_state;

} // namespace

HttpClient::HttpClient() : curl_(nullptr) {
    curl_ = curl_easy_init();
}

HttpClient::~HttpClient() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
}

void HttpClient::set_base_url(const std::string& base_url) {
    base_url_ = base_url;
}

void HttpClient::set_authorization(const std::string& token) {
    auth_token_ = "Bot " + token;
}

HttpResponse HttpClient::get(const std::string& url,
                             const std::map<std::string, std::string>& headers) {
    return perform_request("GET", url, "", headers);
}

HttpResponse HttpClient::post(const std::string& url,
                              const std::string& body,
                              const std::map<std::string, std::string>& headers) {
    return perform_request("POST", url, body, headers);
}

HttpResponse HttpClient::patch(const std::string& url,
                               const std::string& body,
                               const std::map<std::string, std::string>& headers) {
    return perform_request("PATCH", url, body, headers);
}

HttpResponse HttpClient::put(const std::string& url,
                             const std::string& body,
                             const std::map<std::string, std::string>& headers) {
    return perform_request("PUT", url, body, headers);
}

HttpResponse HttpClient::del(const std::string& url,
                             const std::map<std::string, std::string>& headers) {
    return perform_request("DELETE", url, "", headers);
}

HttpResponse HttpClient::perform_request(const std::string& method,
                                        const std::string& url,
                                        const std::string& body,
                                        const std::map<std::string, std::string>& headers) {
    // Each client owns one easy handle. Serialize only users of this instance;
    // independent clients can service time-critical interactions concurrently.
    std::lock_guard<std::mutex> lock(mutex_);
    
    HttpResponse response;
    response.success = false;
    
    if (!curl_) {
        response.body = "CURL not initialized";
        std::cerr << "CURL error: CURL handle is null" << std::endl;
        return response;
    }
    
    // Reset CURL handle to clean state
    curl_easy_reset(curl_);

    std::string full_url = base_url_ + url;
    std::string response_body;
    std::map<std::string, std::string> response_headers;
    
    curl_easy_setopt(curl_, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &response_headers);
    curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, 10000L);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, 30000L);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl_, CURLOPT_ACCEPT_ENCODING, "");
#if LIBCURL_VERSION_NUM >= 0x075500
    curl_easy_setopt(curl_, CURLOPT_PROTOCOLS_STR, "https");
#else
    curl_easy_setopt(curl_, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
#endif
    
    // Set custom method
    if (method == "POST") {
        curl_easy_setopt(curl_, CURLOPT_POST, 1L);
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE,
                         static_cast<long>(body.size()));
    } else if (method == "PATCH" || method == "PUT") {
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, method.c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE,
                         static_cast<long>(body.size()));
    } else if (method == "DELETE") {
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    
    // Build headers
    struct curl_slist* header_list = nullptr;
    
    if (!auth_token_.empty()) {
        std::string auth_header = "Authorization: " + auth_token_;
        header_list = curl_slist_append(header_list, auth_header.c_str());
    }
    
    header_list = curl_slist_append(header_list, "Content-Type: application/json");
    header_list = curl_slist_append(header_list, "User-Agent: DiscordBot (C++/1.0)");
    
    for (const auto& [key, value] : headers) {
        std::string header = key + ": " + value;
        header_list = curl_slist_append(header_list, header.c_str());
    }
    
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, header_list);
    
    // Perform request
    CURLcode res = curl_easy_perform(curl_);
    
    if (res == CURLE_OK) {
        long status_code;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &status_code);
        
        response.status_code = static_cast<int>(status_code);
        response.body = response_body;
        response.headers = response_headers;
        response.success = (status_code >= 200 && status_code < 300);
    } else {
        response.body = curl_easy_strerror(res);
        std::cerr << "CURL error: " << response.body << std::endl;
    }
    
    curl_slist_free_all(header_list);
    
    return response;
}

size_t HttpClient::write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), total_size);
    return total_size;
}

size_t HttpClient::header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t total_size = size * nitems;
    std::string header(buffer, total_size);
    
    auto* headers = static_cast<std::map<std::string, std::string>*>(userdata);
    
    size_t colon_pos = header.find(':');
    if (colon_pos != std::string::npos) {
        std::string key = header.substr(0, colon_pos);
        std::string value = header.substr(colon_pos + 1);
        
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t\n\r"));
        key.erase(key.find_last_not_of(" \t\n\r") + 1);
        value.erase(0, value.find_first_not_of(" \t\n\r"));
        value.erase(value.find_last_not_of(" \t\n\r") + 1);
        std::transform(key.begin(), key.end(), key.begin(),
            [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });
        
        (*headers)[key] = value;
    }
    
    return total_size;
}

} // namespace discord
