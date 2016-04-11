#include "Reply.h"
#include <string.h>

using namespace Liby;
using namespace Liby::http;

const int Reply:: OK = 200;
const int Reply:: CREATED = 201;
const int Reply:: ACCEPTED = 202;
const int Reply:: NO_CONTENT = 204;
const int Reply:: MULTIPLE_CHOICES = 300;
const int Reply:: MOVED_PERMANENTLY = 301;
const int Reply:: MOVED_TEMPORARILY = 302;
const int Reply:: NOT_MODIFIED = 304;
const int Reply:: BAD_REQUEST = 400;
const int Reply:: UNAUTHORIZED = 401;
const int Reply:: FORBIDDEN = 403;
const int Reply:: NOT_FOUND = 404;
const int Reply:: INTERNAL_SERVER_ERROR = 500;
const int Reply:: NOT_IMPLEMENTED = 501;
const int Reply:: BAD_GATEWAY = 502;
const int Reply:: SERVICE_UNAVAILABLE = 503;

std::unordered_map<int, std::string> status_string = {
        {Reply::OK, "Ok" },
        {Reply::CREATED, "Created" },
        {Reply::ACCEPTED, "Accepted" },
        {Reply::NO_CONTENT, "No Content" },
        {Reply::MULTIPLE_CHOICES, "Multiple Choices" },
        {Reply::MOVED_PERMANENTLY, "Moved Permanently" },
        {Reply::MOVED_TEMPORARILY, "Moved Temporarily" },
        {Reply::NOT_MODIFIED, "Not Modified" },
        {Reply::BAD_REQUEST, "Bad Request" },
        {Reply::UNAUTHORIZED, "Unauthorized" },
        {Reply::FORBIDDEN, "Forbidden" },
        {Reply::NOT_FOUND, "Not Found" },
        {Reply::INTERNAL_SERVER_ERROR, "Internal Server Error" },
        {Reply::NOT_IMPLEMENTED, "Not Implemented" },
        {Reply::BAD_GATEWAY, "Bad Gateway" },
        {Reply::SERVICE_UNAVAILABLE, "Service Unavailable" }

};
std::unordered_map<int, std::string> status_number = {
        {Reply::OK, "200" },
        {Reply::CREATED, "201" },
        {Reply::ACCEPTED, "202" },
        {Reply::NO_CONTENT, "204" },
        {Reply::MULTIPLE_CHOICES, "300" },
        {Reply::MOVED_PERMANENTLY, "301" },
        {Reply::MOVED_TEMPORARILY, "302" },
        {Reply::NOT_MODIFIED, "304" },
        {Reply::BAD_REQUEST, "400" },
        {Reply::UNAUTHORIZED, "401" },
        {Reply::FORBIDDEN, "403" },
        {Reply::NOT_FOUND, "404" },
        {Reply::INTERNAL_SERVER_ERROR, "500" },
        {Reply::NOT_IMPLEMENTED, "501" },
        {Reply::BAD_GATEWAY, "502" },
        {Reply::SERVICE_UNAVAILABLE, "503" }
};

std::unordered_map<std::string, std::string> file_mime = {{".html", "text/html"},
                                                          {".xml", "text/xml"},
                                                          {".xhtml", "application/xhtml+xml"},
                                                          {".txt", "text/plain"},
                                                          {".rtf", "application/rtf"},
                                                          {".pdf", "application/pdf"},
                                                          {".word", "application/msword"},
                                                          {".png", "image/png"},
                                                          {".gif", "image/gif"},
                                                          {".jpg", "image/jpeg"},
                                                          {".jpeg", "image/jpeg"},
                                                          {".au", "audio/basic"},
                                                          {".mpeg", "video/mpeg"},
                                                          {".mpg", "video/mpeg"},
                                                          {".avi", "video/x-msvideo"},
                                                          {".gz", "application/x-gzip"},
                                                          {".tar", "application/x-tar"},
                                                          {".css", "text/css"},
                                                          {".cgi", "text/html"},
                                                          {"", "text/plain"}};

std::string Reply::toString() {
    std::string ret = "HTTP/1.1 ";
    ret += status_number[status_];
    ret += " ";
    ret += status_string[status_];
    ret += "\r\n";
    // headers_["Date"] = Timestamp::now().toString();
    if(!content_.empty()) {
        headers_["Content-Length"] = std::to_string(content_.size());
    }
    for(auto &x : headers_) {
        ret += x.first;
        ret += ": ";
        ret += x.second;
        ret += "\r\n";
    }
    ret += "\r\n";
    return ret + content_;
}

std::string Reply::getMimeType() {
    std::string ret;
    unsigned long ops;
    ops = filepath_.rfind('.');
    if(ops == std::string::npos) {
        return "text/plain";
    } else {
        std::string type(&filepath_[ops], &filepath_[filepath_.size()]);
        auto it = file_mime.find(type);
        if(it == file_mime.end()) {
            return "text/plain";
        } else {
            return it->second;
        }
    }
}

std::string e404(const std::string &filepath) {
    static Reply rep;
    std::string ret = "<h1>404 - Not Found</h1>";
    rep.headers_["Date"] = Timestamp::now().toString();
    rep.headers_["Connection"] = "Close";
    rep.headers_["Content-Length"] = ret.size();
    rep.headers_["Content-Type"] = "text/plain";
    return rep.toString() + ret;
}
