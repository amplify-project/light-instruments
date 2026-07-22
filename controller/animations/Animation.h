#ifndef ANIMATION_H
#define ANIMATION_H

#include <FastLED.h>

class Animation {
public:
    virtual ~Animation() = default;
    virtual bool update(int stripIndex) = 0;
    virtual bool isFinished() = 0;
    virtual void setColor(CRGB color) {}
};

#endif // ANIMATION_H
