#ifndef UTILS_H
#define UTILS_H
#include <string>

class Utils {
public:
   static std::string url_decode(const std::string &value);
   static bool replace(std::string &str, const std::string &from, const std::string &to);
   static std::string random_string(size_t length);
};

#endif // UTILS_H