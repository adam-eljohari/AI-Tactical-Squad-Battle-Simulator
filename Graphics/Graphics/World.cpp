#include "World.h"
#include "Room.h"
#include "Character.h"
#include <cstdlib>
#include <cmath>
#include <limits>
#include <algorithm>
#include <queue>

extern int maze[MSZ][MSZ];
extern double securityMap[MSZ][MSZ];

World& World::I()
{
    static World w;
    return w;
}

void World::Bind(Room** r, int n)
{
    rooms = r;
    numRooms = n;
}

void World::AddActor(Character* a)
{
    actors.push_back(a);
}

bool World::InBounds(int r, int c) const { return r >= 0 && r < MSZ && c >= 0 && c < MSZ; }

bool World::Passable(int r, int c) const
{
    if (!InBounds(r, c)) return false;
    int t = maze[r][c];
    return (t == SPACE || t == AMMO_DEPOT || t == MED_DEPOT);
}

void World::RoomBounds(int roomIndex, int& sr, int& er, int& sc, int& ec) const
{
    sr = rooms[roomIndex]->getCenterRow() - rooms[roomIndex]->getHeight() / 2;
    er = rooms[roomIndex]->getCenterRow() + rooms[roomIndex]->getHeight() / 2;
    sc = rooms[roomIndex]->getCenterCol() - rooms[roomIndex]->getWidth() / 2;
    ec = rooms[roomIndex]->getCenterCol() + rooms[roomIndex]->getWidth() / 2;
}

bool World::CellInRoom(int roomIndex, int r, int c) const
{
    int sr, er, sc, ec;
    RoomBounds(roomIndex, sr, er, sc, ec);
    return (r >= sr && r <= er && c >= sc && c <= ec);
}

int World::GetRoomIndexByCell(int r, int c) const
{
    for (int i = 0; i < numRooms; i++)
        if (CellInRoom(i, r, c)) return i;
    return -1;
}

bool World::FindNearestTile(int tileType, int fromR, int fromC, CellPos& out, bool restrictToRoom, int roomIndex) const
{
    double best = std::numeric_limits<double>::infinity();
    bool found = false;

    for (int r = 0; r < MSZ; r++)
        for (int c = 0; c < MSZ; c++)
        {
            if (maze[r][c] != tileType) continue;
            if (restrictToRoom && roomIndex >= 0 && !CellInRoom(roomIndex, r, c)) continue;

            double dx = double(c - fromC);
            double dy = double(r - fromR);
            double d2 = dx * dx + dy * dy;

            if (d2 < best)
            {
                best = d2;
                out = CellPos(r, c);
                found = true;
            }
        }
    return found;
}

bool World::FindRandomSpaceInRoom(int roomIndex, CellPos& out) const
{
    if (roomIndex < 0) return false;

    int sr, er, sc, ec;
    RoomBounds(roomIndex, sr, er, sc, ec);
    sr++; er--; sc++; ec--;

    for (int k = 0; k < 2000; k++)
    {
        int r = sr + rand() % std::max(1, (er - sr + 1));
        int c = sc + rand() % std::max(1, (ec - sc + 1));
        if (Passable(r, c))
        {
            out = CellPos(r, c);
            return true;
        }
    }
    return false;
}

std::vector<Character*> World::EnemiesInSameRoom(const Character* me) const
{
    std::vector<Character*> res;
    int myR = (int)std::round(me->getY());
    int myC = (int)std::round(me->getX());
    int myRoom = GetRoomIndexByCell(myR, myC);
    if (myRoom < 0) return res;

    for (auto* a : actors)
    {
        if (!a || a == me || a->IsDead()) continue;
        if (a->GetTeamId() == me->GetTeamId()) continue;

        int r = (int)std::round(a->getY());
        int c = (int)std::round(a->getX());
        if (GetRoomIndexByCell(r, c) == myRoom)
            res.push_back(a);
    }
    return res;
}

std::vector<Character*> World::AlliesInSameRoom(const Character* me) const
{
    std::vector<Character*> res;
    int myR = (int)std::round(me->getY());
    int myC = (int)std::round(me->getX());
    int myRoom = GetRoomIndexByCell(myR, myC);
    if (myRoom < 0) return res;

    for (auto* a : actors)
    {
        if (!a || a == me || a->IsDead()) continue;
        if (a->GetTeamId() != me->GetTeamId()) continue;

        int r = (int)std::round(a->getY());
        int c = (int)std::round(a->getX());
        if (GetRoomIndexByCell(r, c) == myRoom)
            res.push_back(a);
    }
    return res;
}

std::vector<Character*> World::EnemiesAnywhere(Character* me)
{
    std::vector<Character*> result;
    for (auto* a : actors)
    {
        if (!a) continue;
        if (a->IsDead()) continue;
        if (a->GetTeamId() == me->GetTeamId()) continue;
        result.push_back(a);
    }
    return result;
}

std::vector<Character*> World::AlliesAnywhere(const Character* me) const
{
    std::vector<Character*> res;
    for (auto* a : actors)
    {
        if (!a || a == me || a->IsDead()) continue;
        if (a->GetTeamId() != me->GetTeamId()) continue;
        res.push_back(a);
    }
    return res;
}

Character* World::FindNearestAllyByRole(const Character* me, Role role) const
{
    Character* best = nullptr;
    double bestD2 = 1e18;

    for (auto* a : actors)
    {
        if (!a || a == me || a->IsDead()) continue;
        if (a->GetTeamId() != me->GetTeamId()) continue;
        if (a->GetRole() != role) continue;

        double dx = a->getX() - me->getX();
        double dy = a->getY() - me->getY();
        double d2 = dx * dx + dy * dy;
        if (d2 < bestD2)
        {
            bestD2 = d2;
            best = a;
        }
    }
    return best;
}

Character* World::FindNearestEnemyPreferFighter(const Character* me, bool sameRoomOnly) const
{
    Character* bestFighter = nullptr;
    Character* bestAny = nullptr;
    double bestF2 = 1e18;
    double bestA2 = 1e18;

    int myR = (int)std::round(me->getY());
    int myC = (int)std::round(me->getX());
    int myRoom = GetRoomIndexByCell(myR, myC);

    for (auto* a : actors)
    {
        if (!a || a == me || a->IsDead()) continue;
        if (a->GetTeamId() == me->GetTeamId()) continue;

        int r = (int)std::round(a->getY());
        int c = (int)std::round(a->getX());
        if (sameRoomOnly && myRoom >= 0 && GetRoomIndexByCell(r, c) != myRoom)
            continue;

        double dx = a->getX() - me->getX();
        double dy = a->getY() - me->getY();
        double d2 = dx * dx + dy * dy;

        if (a->GetRole() == Role::Fighter)
        {
            if (d2 < bestF2) { bestF2 = d2; bestFighter = a; }
        }
        else
        {
            if (d2 < bestA2) { bestA2 = d2; bestAny = a; }
        }
    }

    return bestFighter ? bestFighter : bestAny;
}

void World::TickCombatRoomSecurity()
{
    const double MAX_DARK = 0.65;

    std::vector<bool> combat(numRooms, false);
    for (auto* a : actors)
    {
        if (!a || a->IsDead()) continue;
        int r = (int)std::round(a->getY());
        int c = (int)std::round(a->getX());
        int room = GetRoomIndexByCell(r, c);
        if (room < 0) continue;
        if (!EnemiesInSameRoom(a).empty())
            combat[room] = true;
    }

    for (int i = 0; i < numRooms; i++)
    {
        if (!combat[i]) continue;

        int sr, er, sc, ec;
        RoomBounds(i, sr, er, sc, ec);

        for (int r = sr; r <= er; r++)
            for (int c = sc; c <= ec; c++)
                securityMap[r][c] *= 0.985;
    }

    for (auto* a : actors)
    {
        if (!a || a->IsDead()) continue;

        int ar = (int)std::round(a->getY());
        int ac = (int)std::round(a->getX());
        int room = GetRoomIndexByCell(ar, ac);
        if (room < 0) continue;
        if (!combat[room]) continue;

        int sr, er, sc, ec;
        RoomBounds(room, sr, er, sc, ec);

        int R = 14;
        int r0 = std::max(sr, ar - R), r1 = std::min(er, ar + R);
        int c0 = std::max(sc, ac - R), c1 = std::min(ec, ac + R);

        for (int rr = r0; rr <= r1; rr++)
            for (int cc = c0; cc <= c1; cc++)
            {
                if (!Passable(rr, cc)) continue;

                double dx = double(cc - ac);
                double dy = double(rr - ar);
                double d = std::sqrt(dx * dx + dy * dy);
                if (d < 0.5) d = 0.5;

                double add = 0.65 / d;
                securityMap[rr][cc] = std::min(MAX_DARK, securityMap[rr][cc] + add);
            }
    }
}

void World::RemoveDeadActors()
{
    actors.erase(std::remove_if(actors.begin(), actors.end(),
        [](Character* a)
        {
            if (!a) return true;
            if (a->IsDead())
            {
                delete a;
                return true;
            }
            return false;
        }), actors.end());
}

bool World::TeamDead(int teamId) const
{
    for (auto* a : actors)
        if (a && !a->IsDead() && a->GetTeamId() == teamId)
            return false;
    return true;
}


Character* World::FindNearestEnemyFighter(const Character* me, bool sameRoomOnly) const
{
    Character* best = nullptr;
    double bestD2 = 1e18;

    int myR = (int)std::round(me->getY());
    int myC = (int)std::round(me->getX());
    int myRoom = GetRoomIndexByCell(myR, myC);

    for (auto* a : actors)
    {
        if (!a || a == me || a->IsDead()) continue;
        if (a->GetTeamId() == me->GetTeamId()) continue;
        if (a->GetRole() != Role::Fighter) continue;

        int r = (int)std::round(a->getY());
        int c = (int)std::round(a->getX());

        if (sameRoomOnly && myRoom >= 0 && GetRoomIndexByCell(r, c) != myRoom)
            continue;

        double dx = a->getX() - me->getX();
        double dy = a->getY() - me->getY();
        double d2 = dx * dx + dy * dy;

        if (d2 < bestD2)
        {
            bestD2 = d2;
            best = a;
        }
    }

    return best;
}

int World::GetMidRoomIndex() const
{
    if (homeRoom[0] < 0 || homeRoom[1] < 0) return -1;
    if (!rooms) return -1;

    Room* r0 = rooms[homeRoom[0]];
    Room* r1 = rooms[homeRoom[1]];
    if (!r0 || !r1) return -1;

    double r0r = r0->getCenterRow();
    double r0c = r0->getCenterCol();
    double r1r = r1->getCenterRow();
    double r1c = r1->getCenterCol();

    double midr = (r0r + r1r) * 0.5;
    double midc = (r0c + r1c) * 0.5;

    double bestD = std::numeric_limits<double>::infinity();
    int bestIdx = -1;

    for (int i = 0; i < numRooms; ++i)
    {
        if (!rooms[i]) continue;
        double rr = rooms[i]->getCenterRow();
        double cc = rooms[i]->getCenterCol();
        double dx = rr - midr;
        double dy = cc - midc;
        double d2 = dx * dx + dy * dy;
        if (d2 < bestD)
        {
            bestD = d2;
            bestIdx = i;
        }
    }

    return bestIdx;
}
