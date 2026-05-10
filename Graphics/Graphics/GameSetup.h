#pragma once
#include "World.h"
#include "Character.h"
#include "FighterStates.h"
#include "MedicStates.h"
#include "SupplyStates.h"
#include <cstdlib>
#include <cmath>

static inline double RoomDistance(Room** rooms, int i, int j)
{
    double dr = rooms[i]->getCenterRow() - rooms[j]->getCenterRow();
    double dc = rooms[i]->getCenterCol() - rooms[j]->getCenterCol();
    return std::sqrt(dr * dr + dc * dc);
}

static inline void SetupGame(Room** rooms, int numRooms)
{
    World::I().Bind(rooms, numRooms);
    World::I().actors.clear();

    // create teams (2 fighters + medic + supply)
    auto* f0a = new Character(0, Role::Fighter);
    auto* f0b = new Character(0, Role::Fighter);
    auto* m0 = new Character(0, Role::Medic);
    auto* s0 = new Character(0, Role::Supply);

    auto* f1a = new Character(1, Role::Fighter);
    auto* f1b = new Character(1, Role::Fighter);
    auto* m1 = new Character(1, Role::Medic);
    auto* s1 = new Character(1, Role::Supply);

    World::I().AddActor(f0a); World::I().AddActor(f0b); World::I().AddActor(m0); World::I().AddActor(s0);
    World::I().AddActor(f1a); World::I().AddActor(f1b); World::I().AddActor(m1); World::I().AddActor(s1);

    int roomA = 0, roomB = 1;
    double best = 0.0;
    for (int i = 0; i < numRooms; i++)
        for (int j = i + 1; j < numRooms; j++)
        {
            double d = RoomDistance(rooms, i, j);
            if (d > best) { best = d; roomA = i; roomB = j; }
        }

    World::I().SetHomeRoom(0, roomA);
    World::I().SetHomeRoom(1, roomB);

    // spawn team0 in roomA
    for (auto* a : World::I().actors)
        if (a && a->GetTeamId() == 0)
        {
            CellPos pos;
            if (World::I().FindRandomSpaceInRoom(roomA, pos))
                a->setPos((double)pos.c, (double)pos.r);
        }

    // spawn team1 in roomB
    for (auto* a : World::I().actors)
        if (a && a->GetTeamId() == 1)
        {
            CellPos pos;
            if (World::I().FindRandomSpaceInRoom(roomB, pos))
                a->setPos((double)pos.c, (double)pos.r);
        }

    f0a->SetState(new FighterDecideState());
    f0b->SetState(new FighterDecideState());
    m0->SetState(new MedicDecideState());
    s0->SetState(new SupplyDecideState());

    f1a->SetState(new FighterDecideState());
    f1b->SetState(new FighterDecideState());
    m1->SetState(new MedicDecideState());
    s1->SetState(new SupplyDecideState());
}
