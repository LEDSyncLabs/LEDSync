#include "utils.h"
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include "esp_random.h"

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



std::string Utils::random_string(size_t length) {
    const std::string characters = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::string random_str;
    random_str.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        uint32_t random_value = esp_random();  // Generowanie liczby losowej
        size_t index = random_value % characters.size();
        random_str += characters[index];
    }

    return random_str;
}
