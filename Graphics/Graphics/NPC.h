#pragma once
#include <vector>

class State;

struct GridPos
{
    int r = 0, c = 0;
    GridPos() = default;
    GridPos(int rr, int cc) : r(rr), c(cc) {}
};

class NPC
{
private:
    double x = 50.0;
    double y = 50.0;

    double targetX = 50.0;
    double targetY = 50.0;

    double dirX = 0.0;
    double dirY = 0.0;

    bool isMoving = false;

protected:
    State* pCurrentState = nullptr;

    std::vector<GridPos> path;
    int pathIndex = 0;

public:
    static constexpr double SPEED = 0.08;

    NPC();
    virtual ~NPC();

    void show();

    void setPos(double nx, double ny);

    void setTarget(double tx, double ty);
    void setDirection();

    bool getIsMoving() const { return isMoving; }

    void SetPath(const std::vector<GridPos>& newPath);

    void SetState(State* pNewState);
    State* GetState() const { return pCurrentState; }

    void DoSomeWork();

    double getX() const { return x; }
    double getY() const { return y; }

    void StopMoving();


private:
    void stepMove();
};
