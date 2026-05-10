#pragma once
#include <vector>
#include <memory>
#include "Projectile.h"

class Character;

class ProjectileManager
{
private:
    ProjectileManager() = default;

public:
    static ProjectileManager& I();

    std::vector<std::unique_ptr<Bullet>> bullets;
    std::vector<std::unique_ptr<Grenade>> grenades;

    void FireBullet(Character* shooter, Character* target);
    void ThrowGrenade(Character* shooter, Character* target);

    void Tick();
    void Draw();
};
