#include "ProjectileManager.h"
#include "Character.h"
#include <cmath>
#include <algorithm>

ProjectileManager& ProjectileManager::I()
{
    static ProjectileManager pm;
    return pm;
}

void ProjectileManager::FireBullet(Character* shooter, Character* target)
{
    if (!shooter || !target) return;
    bullets.emplace_back(std::make_unique<Bullet>(
        shooter->getX(), shooter->getY(),
        target->getX() - shooter->getX(),
        target->getY() - shooter->getY(),
        shooter->GetTeamId()
    ));
    shooter->OnShootFired();
}

void ProjectileManager::ThrowGrenade(Character* shooter, Character* target)
{
    if (!shooter || !target) return;
    grenades.emplace_back(std::make_unique<Grenade>(
        target->getX(), target->getY(), shooter->GetTeamId()
    ));
    shooter->OnGrenadeFired();
}

void ProjectileManager::Tick()
{
    for (auto& b : bullets)
        if (b) b->Move();

    for (auto& g : grenades)
        if (g) g->Tick();

    bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
        [](const std::unique_ptr<Bullet>& b) { return !b || !b->IsMoving(); }), bullets.end());

    grenades.erase(std::remove_if(grenades.begin(), grenades.end(),
        [](const std::unique_ptr<Grenade>& g) { return !g || !g->IsAlive(); }), grenades.end());
}

void ProjectileManager::Draw()
{
    for (auto& b : bullets)
        if (b) b->show();

    for (auto& g : grenades)
        if (g) g->show();
}
