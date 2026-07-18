#pragma once
void encoderBegin();
int  encoderTakeDelta();
bool encoderTakePress();

enum EncoderGesture { GESTURE_NONE = 0, GESTURE_SINGLE = 1, GESTURE_DOUBLE = 2, GESTURE_LONG = 3 };
EncoderGesture encoderGesture();
