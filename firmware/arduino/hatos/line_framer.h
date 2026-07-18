#pragma once
#include <Arduino.h>

class LineFramer {
 public:
  void push(const uint8_t* data, size_t len, void (*onLine)(const char*)) {
    for (size_t i = 0; i < len; i++) {
      char c = (char)data[i];
      if (c == '\n') {
        if (overflow_) {
          // Drop the oversized line entirely: no phantom tail fragment.
          overflow_ = false;
          pos_ = 0;
        } else {
          buf_[pos_] = 0;
          if (pos_ > 0) onLine(buf_);
          pos_ = 0;
        }
      } else if (overflow_) {
        // Still inside an oversized line: discard bytes until the newline.
        continue;
      } else if (pos_ < sizeof(buf_) - 1) {
        buf_[pos_++] = c;
      } else {
        overflow_ = true;  // overflow -> drop the oversized line
      }
    }
  }
 private:
  char buf_[241];
  size_t pos_ = 0;
  bool overflow_ = false;
};
