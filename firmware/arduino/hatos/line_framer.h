#pragma once
#include <Arduino.h>

class LineFramer {
 public:
  void push(const uint8_t* data, size_t len, void (*onLine)(const char*)) {
    for (size_t i = 0; i < len; i++) {
      char c = (char)data[i];
      if (c == '\n') {
        buf_[pos_] = 0;
        if (pos_ > 0) onLine(buf_);
        pos_ = 0;
      } else if (pos_ < sizeof(buf_) - 1) {
        buf_[pos_++] = c;
      } else {
        pos_ = 0;  // overflow -> drop the oversized line
      }
    }
  }
 private:
  char buf_[241];
  size_t pos_ = 0;
};
