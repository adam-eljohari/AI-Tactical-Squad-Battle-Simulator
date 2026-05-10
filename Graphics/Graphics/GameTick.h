#pragma once
#include "World.h"
#include "Character.h"
#include "ProjectileManager.h"

static inline bool GameTick()
{
    World::I().TickCombatRoomSecurity();

    for (auto* a : World::I().actors)
    {
        if (!a || a->IsDead()) continue;
        a->TickCooldowns();
        a->DoSomeWork();
    }

    ProjectileManager::I().Tick();

    World::I().RemoveDeadActors();

    if (World::I().TeamDead(0) || World::I().TeamDead(1))
        return false;

    return true;
}
