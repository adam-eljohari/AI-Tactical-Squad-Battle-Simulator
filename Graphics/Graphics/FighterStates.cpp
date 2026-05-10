#include "FighterStates.h"
#include "Character.h"
#include "World.h"
#include "ProjectileManager.h"
#include <cmath>
#include <cstdlib>

static inline double Dist(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

static inline bool LowHP(Character* me)
{
    return me->GetHP() <= 0.5 * me->GetMaxHP();
}

void FighterRetreatState::OnEnter(NPC* npc)
{
    auto* me = dynamic_cast<Character*>(npc);
    if (!me || me->IsDead()) return;

    int home = World::I().GetHomeRoom(me->GetTeamId());
    if (home < 0)
    {
        me->SetState(new FighterDecideState());
        return;
    }

    int myR = (int)std::round(me->getY());
    int myC = (int)std::round(me->getX());
    int curRoom = World::I().GetRoomIndexByCell(myR, myC);
    if (curRoom == home)
    {
        Character* medic = World::I().FindNearestAllyByRole(me, Role::Medic);
        if (medic)
        {
            int mr = (int)std::round(medic->getY());
            int mc = (int)std::round(medic->getX());

            for (int dy = -1; dy <= 1; ++dy)
            {
                for (int dx = -1; dx <= 1; ++dx)
                {
                    int rr = mr + dy;
                    int cc = mc + dx;
                    if (!World::I().InBounds(rr, cc)) continue;
                    if (!World::I().Passable(rr, cc)) continue;
                    if (rr == myR && cc == myC) continue;
                    me->MoveToCell(rr, cc);
                    me->RequestMedic();
                    return;
                }
            }

            me->MoveToCell(mr, mc);
            me->RequestMedic();
            return;
        }
    }

    CellPos pos;
    if (World::I().FindRandomSpaceInRoom(home, pos))
    {
        me->MoveToCell(pos.r, pos.c);

        me->RequestMedic();
    }
}

void FighterRetreatState::Transition(NPC* npc)
{
    auto* me = dynamic_cast<Character*>(npc);
    if (!me || me->IsDead()) return;

    me->RequestMedic();
    me->Heal((me->GetHP()) - 100);

    if (me->GetHP() >= std::min(me->GetMaxHP(), me->GetPersonality().lowHpThreshold * 1.8))
    {
        me->ClearMedicRequest();
        me->SetState(new FighterDecideState());
    }
    else
        me->setTarget(me->getX(), me->getY());
}

void FighterDecideState::OnEnter(NPC* npc)
{
    auto* me = dynamic_cast<Character*>(npc);
    if (!me || me->IsDead()) return;

    if (!World::I().EnemiesInSameRoom(me).empty())
    {
        me->StopMoving();
        me->SetState(new FighterAttackState());
        return;
    }

    if (LowHP(me))
    {
        me->SetState(new FighterRetreatState());
        return;
    }

    int mid = World::I().GetMidRoomIndex();
    if (mid >= 0)
    {
        CellPos pos;
        if (World::I().FindRandomSpaceInRoom(mid, pos))
        {
            me->MoveToCell(pos.r, pos.c);
            me->SetState(new FighterMoveState());
            return;
        }
    }

    int myR = (int)std::round(me->getY());
    int myC = (int)std::round(me->getX());
    int room = World::I().GetRoomIndexByCell(myR, myC);

    CellPos pos;
    if (World::I().FindRandomSpaceInRoom(room, pos))
    {
        me->MoveToCell(pos.r, pos.c);
        me->SetState(new FighterMoveState());
    }
}

// -------------------- MOVE --------------------
void FighterMoveState::Transition(NPC* npc)
{
    auto* me = dynamic_cast<Character*>(npc);
    if (!me || me->IsDead()) return;

    me->SetState(new FighterDecideState());
}

// -------------------- ATTACK --------------------
void FighterAttackState::OnEnter(NPC* npc)
{
    ticks = 0;
}

void FighterAttackState::Update(NPC* npc)
{
    auto* me = dynamic_cast<Character*>(npc);
    if (!me || me->IsDead()) return;

    if (LowHP(me))
    {
        me->SetState(new FighterRetreatState());
        return;
    }

    ticks++;
    if (ticks > 120)
    {
        me->SetState(new FighterDecideState());
        return;
    }

    Character* target = World::I().FindNearestEnemyPreferFighter(me, true);
    if (!target)
    {
        me->SetState(new FighterDecideState());
        return;
    }

    double d = Dist(me->getX(), me->getY(), target->getX(), target->getY());

    if (d > me->GetPersonality().attackRange && d > me->GetPersonality().grenadeRange)
    {
        me->MoveToCell((int)std::round(target->getY()),
            (int)std::round(target->getX()));
        return;
    }

    me->StopMoving();

    if (me->CanGrenade() && d <= me->GetPersonality().grenadeRange && (rand() % 100) < 22)
        ProjectileManager::I().ThrowGrenade(me, target);
    else if (me->CanShoot() && d <= me->GetPersonality().attackRange)
        ProjectileManager::I().FireBullet(me, target);
}
