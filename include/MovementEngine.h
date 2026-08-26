#ifndef SIX_SEVEN_MOVEMENT_ENGINE_H
#define SIX_SEVEN_MOVEMENT_ENGINE_H

#include <string>
#include <windows.h>

namespace six_seven {

class MovementEngine {
public:
    void Start(HWND hwnd, int targetX, int targetY, bool wanderUntilStop = false);
    void Stop();
    bool Tick();
    bool IsActive() const { return active_; }
    const char* CurrentSpritePath() const { return spritePath_.c_str(); }

private:
    static const char* DirectionSprite(int dx, int dy);
    void PickNewTarget(int fromX, int fromY);
    void BounceFromEdge(int winX, int winY, bool hitX, bool hitY);

    HWND hwnd_ = nullptr;
    int targetX_ = 0;
    int targetY_ = 0;
    bool active_ = false;
    bool wanderUntilStop_ = false;
    std::string spritePath_ = "movingR";
};

} /* namespace six_seven */

#endif
