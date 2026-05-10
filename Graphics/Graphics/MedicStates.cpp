#include "MedicStates.h"
#include "Character.h"
#include "World.h"
#include <cmath>
#include <cstdio>

extern int maze[MSZ][MSZ];

static bool AnyInjuredInRoomForTeam(int roomIndex, int teamId)
{
    if (roomIndex < 0) return false;
    for (auto* a : World::I().actors)
    {
        if (!a || a->IsDead()) continue;
        if (a->GetTeamId() != teamId) continue;
        if (a->GetRole() != Role::Fighter) continue;
        int ar = (int)std::round(a->getY());
        int ac = (int)std::round(a->getX());
        if (World::I().GetRoomIndexByCell(ar, ac) != roomIndex) continue;
        if (a->GetHP() < a->GetMaxHP()) return true;
    }
    return false;
}

void MedicDecideState::OnEnter(NPC* npc)
{
    auto* me = dynamic_cast<Character*>(npc);
    if (!me || me->IsDead()) return;

    int myr = (int)std::round(me->getY());
    int myc = (int)std::round(me->getX());
    int team = me->GetTeamId();
    std::printf("[MedicDecide] OnEnter medic at (%d,%d) team=%d medStock=%d\n", myr, myc, team, me->GetMedStock());

    int reqCount = 0;
    for (auto* a : World::I().actors)
    {
        if (!a || a->IsDead()) continue;
        if (a->GetTeamId() != team) continue;
        if (a->GetRole() != Role::Fighter) continue;
        if (a->NeedsMedic()) reqCount++;
    }
    std::printf("[MedicDecide] Team %d has %d active medic request(s)\n", team, reqCount);

    Character* requested = nullptr;
    double bestD2 = 1e18;
    for (auto* a : World::I().actors)
    {
        if (!a || a->IsDead()) continue;
        if (a->GetTeamId() != me->GetTeamId()) continue;
        if (a->GetRole() != Role::Fighter) continue;
        if (!a->NeedsMedic()) continue;

        double dx = a->getX() - me->getX();
        double dy = a->getY() - me->getY();
        double d2 = dx * dx + dy * dy;
        if (d2 < bestD2) { bestD2 = d2; requested = a; }
    }

    if (requested)
    {
        int tr = (int)std::round(requested->getY());
        int tc = (int)std::round(requested->getX());
        std::printf("[MedicDecide] Prioritizing requested fighter at (%d,%d)\n", tr, tc);
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
            {
                int rr = tr + dy;
                int cc = tc + dx;
                if (!World::I().InBounds(rr, cc)) continue;
                if (!World::I().Passable(rr, cc)) continue;
                me->MoveToCell(rr, cc);
                me->SetState(new MedicMoveState());
                return;
            }
        me->MoveToCell(tr, tc);
        me->SetState(new MedicMoveState());
        return;
    }

    int myHome = World::I().GetHomeRoom(me->GetTeamId());
    if (myHome >= 0)
    {
        Character* bestHomeInjured = nullptr;
        double bestD2 = 1e18;
        for (auto* a : World::I().actors)
        {
            if (!a || a->IsDead()) continue;
            if (a->GetTeamId() != me->GetTeamId()) continue;
            if (a->GetRole() != Role::Fighter) continue;

            int ar = (int)std::round(a->getY());
            int ac = (int)std::round(a->getX());
            if (World::I().GetRoomIndexByCell(ar, ac) != myHome) continue;
            if (a->GetHP() >= a->GetMaxHP()) continue;

            double dx = a->getX() - me->getX();
            double dy = a->getY() - me->getY();
            double d2 = dx * dx + dy * dy;
            if (d2 < bestD2) { bestD2 = d2; bestHomeInjured = a; }
        }

        if (bestHomeInjured)
        {
            int tr = (int)std::round(bestHomeInjured->getY());
            int tc = (int)std::round(bestHomeInjured->getX());
            std::printf("[MedicDecide] Home-room injured fighter at (%d,%d) -> moving near\n", tr, tc);
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                {
                    int rr = tr + dy;
                    int cc = tc + dx;
                    if (!World::I().InBounds(rr, cc)) continue;
                    if (!World::I().Passable(rr, cc)) continue;
                    me->MoveToCell(rr, cc);
                    me->SetState(new MedicMoveState());
                    return;
                }
            me->MoveToCell(tr, tc);
            me->SetState(new MedicMoveState());
            return;
        }
    }

    Character* urgent = nullptr;
    bestD2 = 1e18;
    for (auto* a : World::I().actors)
    {
        if (!a || a->IsDead()) continue;
        if (a->GetTeamId() != me->GetTeamId()) continue;
        if (a->GetRole() != Role::Fighter) continue;
        double hpRatio = (a->GetMaxHP() > 0.0) ? (a->GetHP() / a->GetMaxHP()) : 1.0;
        if (hpRatio > 0.5) continue;
        double dx = a->getX() - me->getX();
        double dy = a->getY() - me->getY();
        double d2 = dx * dx + dy * dy;
        if (d2 < bestD2) { bestD2 = d2; urgent = a; }
    }

    if (urgent)
    {
        int tr = (int)std::round(urgent->getY());
        int tc = (int)std::round(urgent->getX());
        std::printf("[MedicDecide] Urgent fighter <=50%% HP at (%d,%d) -> moving\n", tr, tc);
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
            {
                int rr = tr + dy;
                int cc = tc + dx;
                if (!World::I().InBounds(rr, cc)) continue;
                if (!World::I().Passable(rr, cc)) continue;
                me->MoveToCell(rr, cc);
                me->SetState(new MedicMoveState());
                return;
            }
        me->MoveToCell(tr, tc);
        me->SetState(new MedicMoveState());
        return;
    }

    if (me->GetMedStock() <= 0)
    {
        int r = (int)std::round(me->getY());
        int c = (int)std::round(me->getX());
        if (maze[r][c] == MED_DEPOT) me->RefillMedStock();
        else
        {
            CellPos pos;
            if (World::I().FindNearestTile(MED_DEPOT, r, c, pos, false, -1))
                me->MoveToCell(pos.r, pos.c);
        }
        me->SetState(new MedicMoveState());
        return;
    }

    Character* best = nullptr;
    double bestNeed = 0.0;
    for (auto* a : World::I().actors)
    {
        if (!a || a->IsDead()) continue;
        if (a->GetTeamId() != me->GetTeamId()) continue;
        double need = a->GetMaxHP() - a->GetHP();
        double w = (a->GetRole() == Role::Fighter) ? 1.25 : 1.0;
        if (need * w > bestNeed) { bestNeed = need * w; best = a; }
    }
    if (best && bestNeed > 6.0)
        me->MoveToCell((int)std::round(best->getY()), (int)std::round(best->getX()));

    me->SetState(new MedicMoveState());
}

void MedicDecideState::Update(NPC* npc)
{
    auto* me = dynamic_cast<Character*>(npc);
    if (!me || me->IsDead()) return;
    me->MedicHealAlliesSameCell();
}

void MedicMoveState::Transition(NPC* npc)
{
    auto* me = dynamic_cast<Character*>(npc);
    if (!me || me->IsDead()) return;

    int r = (int)std::round(me->getY());
    int c = (int)std::round(me->getX());
    if (maze[r][c] == MED_DEPOT && me->GetMedStock() < 8)
        me->RefillMedStock();

    std::printf("[MedicMove] Arrived at (%d,%d) medStock=%d -> attempt heal\n", r, c, me->GetMedStock());

    int room = World::I().GetRoomIndexByCell(r, c);
    Character* nearestInjured = nullptr;
    double bestD2 = 1e18;
    for (auto* a : World::I().actors)
    {
        if (!a || a->IsDead()) continue;
        if (a->GetTeamId() != me->GetTeamId()) continue;
        if (a->GetRole() != Role::Fighter) continue;

        int ar = (int)std::round(a->getY());
        int ac = (int)std::round(a->getX());
        if (World::I().GetRoomIndexByCell(ar, ac) != room) continue;
        if (a->GetHP() >= a->GetMaxHP()) continue;

        double dx = a->getX() - me->getX();
        double dy = a->getY() - me->getY();
        double d2 = dx*dx + dy*dy;
        if (d2 < bestD2) { bestD2 = d2; nearestInjured = a; }
    }

    if (nearestInjured)
    {
        int tr = (int)std::round(nearestInjured->getY());
        int tc = (int)std::round(nearestInjured->getX());

        int dr = std::abs(tr - r);
        int dc = std::abs(tc - c);
        if (std::max(dr, dc) <= 1)
        {
            me->MedicHealAlliesSameCell();
            if (me->GetMedStock() > 0 && AnyInjuredInRoomForTeam(room, me->GetTeamId()))
            {
                std::printf("[MedicMove] Injured remain in room %d — entering wait/heal state\n", room);
                me->SetState(new MedicWaitState());
                return;
            }
            me->SetState(new MedicDecideState());
            return;
        }

        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                int rr = tr + dy;
                int cc = tc + dx;
                if (!World::I().InBounds(rr, cc)) continue;
                if (!World::I().Passable(rr, cc)) continue;
                if (rr == r && cc == c) continue;
                std::printf("[MedicMove] Moving to adjacent tile (%d,%d) near injured (%d,%d)\n", rr, cc, tr, tc);
                me->MoveToCell(rr, cc);
                me->SetState(new MedicMoveState());
                return;
            }
        }

        std::printf("[MedicMove] No adjacent passable tile found, moving to exact cell (%d,%d)\n", tr, tc);
        me->MoveToCell(tr, tc);
        me->SetState(new MedicMoveState());
        return;
    }

    me->SetState(new MedicDecideState());
}

void MedicWaitState::Update(NPC* npc)
{
    auto* me = dynamic_cast<Character*>(npc);
    if (!me || me->IsDead()) return;

    me->MedicHealAlliesSameCell();

    int r = (int)std::round(me->getY());
    int room = World::I().GetRoomIndexByCell(r, (int)std::round(me->getX()));
    if (me->GetMedStock() <= 0 || !AnyInjuredInRoomForTeam(room, me->GetTeamId()))
    {
        std::printf("[MedicWait] Done healing or out of stock -> deciding\n");
        me->SetState(new MedicDecideState());
    }
}
