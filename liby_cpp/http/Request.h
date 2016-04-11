#ifndef POLLERTEST_REQUEST_H
#define POLLERTEST_REQUEST_H

#include "../util.h"
#include <string>
#include <unordered_map>

namespace Liby {
namespace http {

struct Request final {
    enum class Status {
        ParsingMethod = 1,  // 刚开始分析
        ParsingURI = 2,     // 正在分析URI
        ParsingVersion = 3, // 正在分析http版本
        ParsingHeaders = 4, // 正在分析headers但数据不完整
        Good = 5,           // 分析完成,有效请求
        InvalidMethod = 6,
        InvalidURI = 7,
        InvalidVersion = 8,
        InvalidHeader = 9
    };

    enum class Method {
        INVALID_METHOD,
        GET,
        POST,
        PUT,
        OPTIONS,
    };

    Status status_ = Status::ParsingMethod;
    int bytes_ = 0; // Request部分(不包含body)的长度
    Method method_ = Method::INVALID_METHOD;
    std::string uri_;
    std::string query_;
    int http_version_major_ = 0;
    int http_version_minor_ = 0;
    std::unordered_map<std::string, std::string> headers_;

    void parse(const char *begin, const char *end);
    void clear();

    bool isError() const {
        return status_ == Status::InvalidURI ||
               status_ == Status::InvalidHeader ||
               status_ == Status::InvalidVersion ||
               status_ == Status::InvalidMethod;
    }

    bool isGood() const { return status_ == Status::Good; }

    bool ParseHttpVersion(const char *begin, const char *end) noexcept;
    bool ParseURI(const char *begin, const char *end) noexcept;
    bool ParseMethod(const char *begin, const char *end) noexcept;
    bool ParseHeader(const char *begin, const char *end) noexcept;

    void print();
};
}
}

#endif // POLLERTEST_REQUEST_H
