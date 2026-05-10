#pragma once
#include <vector>
#include "Constants.h"
#include "Character.h"

class Room;
class Character;

struct CellPos
{
    int r = 0, c = 0;
    CellPos() = default;
    CellPos(int rr, int cc) : r(rr), c(cc) {}
};

class World
{
private:
    World() = default;

    int homeRoom[2] = { -1, -1 };

public:
    static World& I();

    Room** rooms = nullptr;
    int numRooms = 0;

    std::vector<Character*> actors;

    void Bind(Room** r, int n);
    void AddActor(Character* a);

    void SetHomeRoom(int team, int room) { if (team >= 0 && team < 2) homeRoom[team] = room; }
    int  GetHomeRoom(int team) const { return (team >= 0 && team < 2) ? homeRoom[team] : -1; }

    bool InBounds(int r, int c) const;
    bool Passable(int r, int c) const;

    int GetRoomIndexByCell(int r, int c) const;
    bool CellInRoom(int roomIndex, int r, int c) const;
    void RoomBounds(int roomIndex, int& sr, int& er, int& sc, int& ec) const;

    bool FindNearestTile(int tileType, int fromR, int fromC, CellPos& out, bool restrictToRoom, int roomIndex) const;
    bool FindRandomSpaceInRoom(int roomIndex, CellPos& out) const;

    std::vector<Character*> EnemiesInSameRoom(const Character* me) const;
    std::vector<Character*> AlliesInSameRoom(const Character* me) const;

    std::vector<Character*> EnemiesAnywhere(Character* me);
    std::vector<Character*> AlliesAnywhere(const Character* me) const;

    Character* FindNearestAllyByRole(const Character* me, Role role) const;

    Character* FindNearestEnemyPreferFighter(const Character* me, bool sameRoomOnly) const;

    void TickCombatRoomSecurity();
    void RemoveDeadActors();
    bool TeamDead(int teamId) const;

    Character* FindNearestEnemyFighter(const Character* me, bool sameRoomOnly) const;

    int GetMidRoomIndex() const;
};
