// Thorough I2C scan on the AX22 shared bus, both pin orientations.
#include <Wire.h>

void scan(int sda, int scl) {
  Wire.end();
  Wire.begin(sda, scl);
  Wire.setClock(100000);
  delay(50);
  Serial.printf("\n-- scan SDA=%d SCL=%d --\n", sda, scl);
  int n = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  found 0x%02X\n", a);
      n++;
    }
  }
  Serial.printf("  total: %d\n", n);
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("\n== I2C scan ==");
}

void loop() {
  scan(10, 11);   // schematic: SDA=GPIO10, SCL=GPIO11
  scan(11, 10);   // swapped, just in case
  delay(2000);
}
