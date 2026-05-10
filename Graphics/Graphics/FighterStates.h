#pragma once
#include "State.h"

class FighterDecideState : public State
{
public:
    void OnEnter(NPC* npc) override;
};

class FighterMoveState : public State
{
public:
    void Transition(NPC* npc) override;
};

class FighterAttackState : public State
{
    int ticks = 0;
public:
    void OnEnter(NPC* npc) override;
    void Update(NPC* npc) override;
};

class FighterRetreatState : public State
{
public:
    void OnEnter(NPC* npc) override;
    void Transition(NPC* npc) override;
};
