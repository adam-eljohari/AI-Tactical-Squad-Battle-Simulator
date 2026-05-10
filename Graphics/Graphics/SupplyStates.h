#pragma once
#include "State.h"

class SupplyDecideState : public State { public: void OnEnter(NPC* npc) override; void Update(NPC* npc) override; };
class SupplyMoveState : public State { public: void Transition(NPC* npc) override; };
