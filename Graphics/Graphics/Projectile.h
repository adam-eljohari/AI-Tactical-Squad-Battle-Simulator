#pragma once
#include "Constants.h"

class Character;

class Bullet
{
    double x = 0, y = 0;
    double dirX = 0, dirY = 0;
    int teamId = 0;
    bool moving = true;
    double speed = 0.7;

public:
    Bullet(double sx, double sy, double dx, double dy, int team);
    bool IsMoving() const { return moving; }
    void Move();
    void show() const;

private:
    void CheckHit();
};

class Grenade
{
    double x = 0, y = 0;
    int teamId = 0;
    bool exploding = false;
    int timer = 18;

public:
    Grenade(double sx, double sy, int team);
    bool IsAlive() const { return timer > 0 || exploding; }
    void Tick();
    void show() const;

private:
    void Explode();
};
