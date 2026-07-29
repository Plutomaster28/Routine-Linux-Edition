#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <curl/curl.h>

namespace discord {

struct HttpResponse {
    int status_code = 0;
    std::string body;
    std::map<std::string, std::string> headers;
    bool success = false;
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    HttpResponse get(const std::string& url, 
                     const std::map<std::string, std::string>& headers = {});
    
    HttpResponse post(const std::string& url, 
                      const std::string& body,
                      const std::map<std::string, std::string>& headers = {});
    
    HttpResponse patch(const std::string& url,
                       const std::string& body,
                       const std::map<std::string, std::string>& headers = {});

    HttpResponse put(const std::string& url,
                     const std::string& body,
                     const std::map<std::string, std::string>& headers = {});
    
    HttpResponse del(const std::string& url,
                     const std::map<std::string, std::string>& headers = {});

    void set_base_url(const std::string& base_url);
    void set_authorization(const std::string& token);

private:
    HttpResponse perform_request(const std::string& method,
                                 const std::string& url,
                                 const std::string& body,
                                 const std::map<std::string, std::string>& headers);
    
    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata);

    CURL* curl_;
    std::string base_url_;
    std::string auth_token_;
    std::mutex mutex_;
};

} // namespace discord

#endif // HTTP_CLIENT_HPP
