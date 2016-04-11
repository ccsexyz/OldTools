#ifndef POLLERTEST_REPLY_H
#define POLLERTEST_REPLY_H

#include "../util.h"
#include <unordered_map>
#include <vector>

namespace Liby {

class File;
namespace http {

struct Reply final {

    const static int OK;
    const static int CREATED;
    const static int ACCEPTED;
    const static int NO_CONTENT;
    const static int MULTIPLE_CHOICES;
    const static int MOVED_PERMANENTLY;
    const static int MOVED_TEMPORARILY;
    const static int NOT_MODIFIED;
    const static int BAD_REQUEST;
    const static int UNAUTHORIZED;
    const static int FORBIDDEN;
    const static int NOT_FOUND;
    const static int INTERNAL_SERVER_ERROR;
    const static int NOT_IMPLEMENTED;
    const static int BAD_GATEWAY;
    const static int SERVICE_UNAVAILABLE;

    std::shared_ptr<File> filefp_;
    long filesize_ = -1;
    int filefd_ = -1;
    int status_;
    std::unordered_map<std::string, std::string> headers_;
    std::string filepath_;
    std::string content_;
    std::string toString();
    std::string getMimeType();
    void genContent();
};

std::string e404(const std::string &filepath);
// std::string e500(std::string filepath);
}
}

#endif // POLLERTEST_REPLY_H
