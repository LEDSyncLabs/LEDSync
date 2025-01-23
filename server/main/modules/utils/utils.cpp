#include "utils.h"
#include "esp_random.h"
#include "math.h"
#include <cstdlib>
#include <ctime>
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

std::string Utils::random_string(size_t length) {
  const std::string characters = "abcdefghijklmnopqrstuvwxyz0123456789";
  std::string random_str;
  random_str.reserve(length);

  for (size_t i = 0; i < length; ++i) {
    uint32_t random_value = esp_random(); // Generowanie liczby losowej
    size_t index = random_value % characters.size();
    random_str += characters[index];
  }

  return random_str;
}

void RGBtoHSV(float r, float g, float b, float *h, float *s, float *v) {
  float min, max, delta;
  min = MIN(MIN(r, g), b);
  max = MAX(MAX(r, g), b);
  *v = max; // v
  delta = max - min;
  if (max != 0)
    *s = delta / max; // s
  else {
    // r = g = b = 0		// s = 0, v is undefined
    *s = 0;
    *h = -1;
    return;
  }
  if (r == max)
    *h = (g - b) / delta; // between yellow & magenta
  else if (g == max)
    *h = 2 + (b - r) / delta; // between cyan & yellow
  else
    *h = 4 + (r - g) / delta; // between magenta & cyan
  *h *= 60;                   // degrees
  if (*h < 0)
    *h += 360;
}
void HSVtoRGB(float *r, float *g, float *b, float h, float s, float v) {
  int i;
  float f, p, q, t;
  if (s == 0) {
    // achromatic (grey)
    *r = *g = *b = v;
    return;
  }
  h /= 60; // sector 0 to 5
  i = floor(h);
  f = h - i; // factorial part of h
  p = v * (1 - s);
  q = v * (1 - s * f);
  t = v * (1 - s * (1 - f));
  switch (i) {
  case 0:
    *r = v;
    *g = t;
    *b = p;
    break;
  case 1:
    *r = q;
    *g = v;
    *b = p;
    break;
  case 2:
    *r = p;
    *g = v;
    *b = t;
    break;
  case 3:
    *r = p;
    *g = q;
    *b = v;
    break;
  case 4:
    *r = t;
    *g = p;
    *b = v;
    break;
  default: // case 5:
    *r = v;
    *g = p;
    *b = q;
    break;
  }
}