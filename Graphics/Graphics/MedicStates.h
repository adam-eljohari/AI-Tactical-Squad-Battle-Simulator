#pragma once
#include "State.h"

class MedicDecideState : public State
{
public:
    void OnEnter(NPC* npc) override;
    void Update(NPC* npc) override;
};

class MedicMoveState : public State
{
public:
    void Transition(NPC* npc) override;
};

class MedicWaitState : public State
{
public:
    void OnEnter(NPC* npc) override {}
    void Update(NPC* npc) override;
};
