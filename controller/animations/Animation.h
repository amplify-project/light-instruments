#ifndef ANIMATION_H
#define ANIMATION_H

class Animation {
public:
    virtual ~Animation() = default;
    virtual bool update(int stripIndex) = 0;
    virtual bool isFinished() = 0;
};

#endif // ANIMATION_H
