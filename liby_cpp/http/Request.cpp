#include "Request.h"
#include <cstring>

using namespace Liby;
using namespace Liby::http;

std::unordered_map<std::string, Request::Method> methods = {
    {"GET", Request::Method::GET},
    {"POST", Request::Method::POST},
    {"PUT", Request::Method::PUT},
    {"OPTIONS", Request::Method::OPTIONS}};

static inline char parse_hex(const char *s) {
    int n1, n2;
    if (*s >= '0' && *s <= '9')
        n1 = *s - '0';
    else {
        char temp = *s | 32;
        n1 = temp - 'a' + 10;
    }
    if (*(s + 1) >= '0' && *(s + 1) <= '9')
        n2 = *(s + 1) - '0';
    else {
        char temp = *(s + 1) | 32;
        n2 = temp - 'a' + 10;
    }
    return (n1 * 16 + n2);
}
static inline const char *findFirstOf(const char *begin, const char *end,
                                      int c) {
    for (; begin < end && *begin != c; begin++)
        ;
    if (begin < end) {
        return begin;
    } else {
        return nullptr;
    }
}

void Request::parse(const char *begin, const char *end) {
    bool flag;
    const char *finish = nullptr;
    switch (status_) {
    case Request::Status::Good:
        // debug("Parsing Status is Good, do not need parse!");
        break;
    case Request::Status::ParsingMethod:
        finish = findFirstOf(begin, end, ' ');
        if (finish == nullptr)
            return;
        flag = ParseMethod(begin, finish);
        if (!flag)
            return;
        bytes_ += ++finish - begin;
        begin = finish;
    case Request::Status::ParsingURI:
        finish = findFirstOf(begin, end, ' ');
        if (finish == nullptr)
            return;
        flag = ParseURI(begin, finish);
        if (!flag)
            return;
        bytes_ += ++finish - begin;
        begin = finish;
    case Request::Status::ParsingVersion:
        finish = findFirstOf(begin, end, '\n');
        if (finish == nullptr)
            return;
        flag = ParseHttpVersion(begin,
                                *(finish - 1) == '\r' ? finish - 1 : finish);
        if (!flag)
            return;
        bytes_ += ++finish - begin;
        begin = finish;
    case Request::Status::ParsingHeaders: {
        while (1) {
            finish = findFirstOf(begin, end, '\n');
            if (finish == nullptr)
                return;
            if (begin == finish || begin + 1 == finish) {
                status_ = Request::Status::Good;
                bytes_ += ++finish - begin;
                return;
            }
            flag =
                ParseHeader(begin, *(finish - 1) == '\r' ? finish - 1 : finish);
            if (!flag)
                return;
            bytes_ += ++finish - begin;
            begin = finish;
        }
    }
    default:
        break;
        // debug("Parsing Status is BadRequest, can not parse");
    }
}

bool Request::ParseMethod(const char *begin, const char *end) noexcept {
    assert(begin <= end && begin && end);

    std::string method((begin), (end));
    auto it = methods.find(method);
    if (it == methods.end()) {
        status_ = Request::Status::InvalidMethod;
        return false;
    } else {
        method_ = it->second;
        status_ = Request::Status::ParsingURI;
        return true;
    }
}

bool Request::ParseHttpVersion(const char *begin, const char *end) noexcept {
    assert(begin <= end && begin && end);

    int length = end - begin;
    if (length != 8) {
        status_ = Request::Status::InvalidVersion;
        return false;
    }
    if (!std::strncmp("HTTP/1.1", begin, 8)) {
        http_version_major_ = 1;
        http_version_minor_ = 1;
    } else if (!std::strncmp("HTTP/1.0", begin, 8)) {
        http_version_major_ = 1;
        http_version_minor_ = 0;
    } else {
        status_ = Request::Status::InvalidVersion;
        return false;
    }
    status_ = Request::Status::ParsingHeaders;
    return true;
}

bool Request::ParseHeader(const char *begin, const char *end) noexcept {
    assert(begin <= end && begin && end);

    const char *mid = findFirstOf(begin, end, ':');
    if (mid == nullptr) {
        status_ = Request::Status::InvalidHeader;
        return false;
    }

    const char *left = mid - 1;
    const char *right = mid + 1;
    while (std::isspace(*left))
        left--;
    while (std::isspace(*right))
        right++;
    std::string key(begin, left + 1);
    std::string value(right, end);
    headers_[key] = value;
    return true;
}

bool Request::ParseURI(const char *begin, const char *end) noexcept {
    assert(begin <= end && begin && end);

    do {
        int query_index = -1;
        int n = end - begin;
        const char *src = begin;
        char *temp = new (std::nothrow) char[n + 1];
        if (temp == nullptr)
            break;
        int i = 0, j = 0;
        for (; i < n; i++, j++) {
            if (src[i] == '%') {
                temp[j] = parse_hex(src + i + 1);
                i += 2;
            } else {
                temp[j] = src[i];
            }
            if (temp[j] == '?') {
                query_index = j;
                temp[j] = '\0';
            }
        }
        temp[j] = '\0';

        if (query_index != -1)
            query_ = &temp[query_index + 1];
        uri_ = temp;
        status_ = Request::Status::ParsingVersion;
        return true;
    } while (0);

    status_ = Request::Status::InvalidURI;
    return false;
}

void Request::clear() {
    status_ = Request::Status::ParsingMethod;
    bytes_ = 0;
    method_ = Request::Method::INVALID_METHOD;
    uri_ = query_ = "";
    http_version_major_ = http_version_minor_ = 0;
    headers_.clear();
}

void Request::print() {
    info("bytes: %d\n", bytes_);
    info("uri: %s", uri_.data());
    info("query: %s", query_.data());
    for (auto &x : headers_) {
        info("%s: %s", x.first.data(), x.second.data());
    }
}
