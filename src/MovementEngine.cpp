#include "../include/MovementEngine.h"
#include "../include/Util.h"
#include "../config.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace six_seven {

const char* MovementEngine::DirectionSprite(int dx, int dy)
{
    const bool up = dy < 0;
    const bool down = dy > 0;
    const bool left = dx < 0;
    const bool right = dx > 0;
    if (up && right)
        return "movingRU";
    if (up && left)
        return "movingLU";
    if (down && right)
        return "movingRD";
    if (down && left)
        return "movingLD";
    if (up)
        return "movingU";
    if (down)
        return "movingD";
    if (left)
        return "movingL";
    if (right)
        return "movingR";
    return "movingR";
}

void MovementEngine::PickNewTarget(int fromX, int fromY)
{
    int tries = 0;
    do {
        RandomSpriteWindowPos(targetX_, targetY_, SIX_SEVEN_MOVE_MARGIN_PX);
        ++tries;
    } while (tries < 8 && std::abs(targetX_ - fromX) < 48 && std::abs(targetY_ - fromY) < 48);
}

void MovementEngine::BounceFromEdge(int winX, int winY, bool hitX, bool hitY)
{
    POINT anchor = { winX + SIX_SEVEN_SPRITE_DRAW_X + SIX_SEVEN_SPRITE_WIDTH / 2,
                     winY + SIX_SEVEN_SPRITE_DRAW_Y + SIX_SEVEN_SPRITE_HEIGHT / 2 };
    RECT wr = {};
    if (!GetWorkAreaAtPoint(anchor, wr))
        wr = GetCombinedWorkArea();

    const int cx = anchor.x;
    const int cy = anchor.y;
    double angle = 0.0;

    if (hitX && !hitY) {
        const bool towardTop = (cy - wr.top) > (wr.bottom - cy);
        angle = towardTop ? -M_PI / 2.0 : M_PI / 2.0;
    } else if (hitY && !hitX) {
        const bool towardLeft = (cx - wr.left) > (wr.right - cx);
        angle = towardLeft ? M_PI : 0.0;
    } else {
        const int dxClamp = hitX ? 1 : 0;
        const int dyClamp = hitY ? 1 : 0;
        if (dxClamp >= dyClamp) {
            const bool towardTop = (cy - wr.top) > (wr.bottom - cy);
            angle = towardTop ? -M_PI / 2.0 : M_PI / 2.0;
        } else {
            const bool towardLeft = (cx - wr.left) > (wr.right - cx);
            angle = towardLeft ? M_PI : 0.0;
        }
    }

    const int tap = RandomInt(-SIX_SEVEN_MOVE_MAX_TAP_DEG, SIX_SEVEN_MOVE_MAX_TAP_DEG);
    angle += static_cast<double>(tap) * M_PI / 180.0;

    const double dist = static_cast<double>(SIX_SEVEN_MOVE_BOUNCE_DIST);
    targetX_ = winX + static_cast<int>(std::cos(angle) * dist);
    targetY_ = winY + static_cast<int>(std::sin(angle) * dist);

    ClampWindowToSpriteWorkArea(targetX_, targetY_, SIX_SEVEN_SPRITE_DRAW_X,
                                SIX_SEVEN_SPRITE_DRAW_Y, SIX_SEVEN_SPRITE_WIDTH,
                                SIX_SEVEN_SPRITE_HEIGHT, &anchor);
    spritePath_ = DirectionSprite(targetX_ - winX, targetY_ - winY);
}

void MovementEngine::Start(HWND hwnd, int targetX, int targetY, bool wanderUntilStop)
{
    hwnd_ = hwnd;
    targetX_ = targetX;
    targetY_ = targetY;
    wanderUntilStop_ = wanderUntilStop;
    active_ = true;
    RECT rc = {};
    if (hwnd_)
        GetWindowRect(hwnd_, &rc);
    const int dx = targetX_ - rc.left;
    const int dy = targetY_ - rc.top;
    spritePath_ = DirectionSprite(dx, dy);
}

void MovementEngine::Stop()
{
    active_ = false;
    wanderUntilStop_ = false;
}

bool MovementEngine::Tick()
{
    if (!active_ || !hwnd_)
        return false;
    RECT rc = {};
    GetWindowRect(hwnd_, &rc);
    const int x = rc.left;
    const int y = rc.top;
    const int dx = targetX_ - x;
    const int dy = targetY_ - y;
    const int step = SIX_SEVEN_MOVE_SPEED_PX;

    if (std::abs(dx) < step && std::abs(dy) < step) {
#if SIX_SEVEN_MOVE_BOUNCE_EDGE
        if (wanderUntilStop_) {
            PickNewTarget(x, y);
            spritePath_ = DirectionSprite(targetX_ - x, targetY_ - y);
            return true;
        }
#endif
        int nx = targetX_;
        int ny = targetY_;
        POINT anchor = { nx + SIX_SEVEN_SPRITE_DRAW_X + SIX_SEVEN_SPRITE_WIDTH / 2,
                         ny + SIX_SEVEN_SPRITE_DRAW_Y + SIX_SEVEN_SPRITE_HEIGHT / 2 };
        ClampWindowToSpriteWorkArea(nx, ny, SIX_SEVEN_SPRITE_DRAW_X, SIX_SEVEN_SPRITE_DRAW_Y,
                                    SIX_SEVEN_SPRITE_WIDTH, SIX_SEVEN_SPRITE_HEIGHT, &anchor);
        SetWindowPos(hwnd_, nullptr, nx, ny, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        active_ = false;
        return false;
    }

    const int stepX = dx > 0 ? step : dx < 0 ? -step : 0;
    const int stepY = dy > 0 ? step : dy < 0 ? -step : 0;
    int nx = x + stepX;
    int ny = y + stepY;
    const int rawNx = nx;
    const int rawNy = ny;
    POINT anchor = { nx + SIX_SEVEN_SPRITE_DRAW_X + SIX_SEVEN_SPRITE_WIDTH / 2,
                     ny + SIX_SEVEN_SPRITE_DRAW_Y + SIX_SEVEN_SPRITE_HEIGHT / 2 };
    ClampWindowToSpriteWorkArea(nx, ny, SIX_SEVEN_SPRITE_DRAW_X, SIX_SEVEN_SPRITE_DRAW_Y,
                                SIX_SEVEN_SPRITE_WIDTH, SIX_SEVEN_SPRITE_HEIGHT, &anchor);

#if SIX_SEVEN_MOVE_BOUNCE_EDGE
    const bool hitEdge = (nx != rawNx) || (ny != rawNy);
    const bool stuck = (nx == x && ny == y && (stepX != 0 || stepY != 0));
    if (hitEdge || stuck) {
        const bool hitX = nx != rawNx;
        const bool hitY = ny != rawNy;
        BounceFromEdge(nx, ny, hitX, hitY);
        SetWindowPos(hwnd_, nullptr, nx, ny, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        return true;
    }
#endif

    if (stepX != 0 || stepY != 0)
        spritePath_ = DirectionSprite(stepX, stepY);
    else
        spritePath_ = DirectionSprite(dx, dy);
    SetWindowPos(hwnd_, nullptr, nx, ny, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    return true;
}

} /* namespace six_seven */
