#pragma once
class NPC;

class State
{
public:
    virtual ~State() = default;
    virtual void OnEnter(NPC* npc) {}
    virtual void Transition(NPC* npc) {}
    virtual void Update(NPC* npc) {}
};
