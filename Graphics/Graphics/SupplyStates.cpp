#include "SupplyStates.h"
#include "Character.h"
#include "World.h"
#include <cmath>

extern int maze[MSZ][MSZ];

void SupplyDecideState::OnEnter(NPC* npc)
{
    auto* me = dynamic_cast<Character*>(npc);
    if (!me || me->IsDead()) return;

    if (me->GetSupplyStock() <= 0)
    {
        int r = (int)std::round(me->getY());
        int c = (int)std::round(me->getX());
        if (maze[r][c] == AMMO_DEPOT)
            me->RefillSupplyStock();
        else
        {
            CellPos pos;
            if (World::I().FindNearestTile(AMMO_DEPOT, r, c, pos, false, -1))
                me->MoveToCell(pos.r, pos.c);
        }
        me->SetState(new SupplyMoveState());
        return;
    }

    Character* best = nullptr;
    double bestNeed = 0.0;

    for (auto* a : World::I().actors)
    {
        if (!a || a->IsDead()) continue;
        if (a->GetTeamId() != me->GetTeamId()) continue;
        if (a->GetRole() != Role::Fighter) continue;

        double need = double(a->GetMaxAmmo() - a->GetAmmo());
        if (need > bestNeed)
        {
            bestNeed = need;
            best = a;
        }
    }

    if (best && bestNeed >= 2.0)
        me->MoveToCell((int)std::round(best->getY()), (int)std::round(best->getX()));

    me->SetState(new SupplyMoveState());
}

void SupplyDecideState::Update(NPC* npc)
{
    auto* me = dynamic_cast<Character*>(npc);
    if (!me || me->IsDead()) return;
    me->SupplyServeAlliesSameCell();
}

void SupplyMoveState::Transition(NPC* npc)
{
    auto* me = dynamic_cast<Character*>(npc);
    if (!me || me->IsDead()) return;

    int r = (int)std::round(me->getY());
    int c = (int)std::round(me->getX());
    if (maze[r][c] == AMMO_DEPOT && me->GetSupplyStock() <= 0)
        me->RefillSupplyStock();

    me->SetState(new SupplyDecideState());
}
