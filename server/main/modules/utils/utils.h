#pragma once
#include <string>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
class Utils {
public:
  static std::string url_decode(const std::string &value);
  static bool replace(std::string &str, const std::string &from,
                      const std::string &to);
  static std::string random_string(size_t length);
};
void RGBtoHSV(float r, float g, float b, float *h, float *s, float *v);
void HSVtoRGB(float *r, float *g, float *b, float h, float s, float v);