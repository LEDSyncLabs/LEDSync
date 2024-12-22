#include "utils.h"
#include <iomanip>
#include <sstream>

std::string Utils::url_decode(const std::string &value) {
  std::ostringstream decoded;
  size_t length = value.length();

  for (size_t i = 0; i < length; i++) {
    if (value[i] == '%' && i + 2 < length) {
      // Decode %xx
      std::istringstream hex(value.substr(i + 1, 2));
      int hex_value;
      if (hex >> std::hex >> hex_value) {
        decoded << static_cast<char>(hex_value);
        i += 2;
      }
    } else if (value[i] == '+') {
      // Replace '+' with space
      decoded << ' ';
    } else {
      decoded << value[i];
    }
  }

  return decoded.str();
}

bool Utils::replace(std::string &str, const std::string &from,
                    const std::string &to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;
}
