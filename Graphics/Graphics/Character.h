#pragma once
#include "NPC.h"
#include "Constants.h"

struct Personality
{
    double lowHpThreshold = 35.0;
    int lowAmmoThreshold = 4;
    double attackRange = 18.0;
    double grenadeRange = 26.0;
    double braveFactor = 0.55;
};

class Character : public NPC
{
private:
    int teamId = 0;
    Role role = Role::Fighter;

    double hp = 100.0;
    double maxHp = 100.0;

    int ammo = 12;
    int maxAmmo = 12;

    int supplyStock = 6; // for Supply
    int medStock = 6;    // for Medic

    Personality p;

    int shootCD = 0;
    int grenadeCD = 0;

    // Request flag used to call medic explicitly
    bool needsMedic = false;

public:
    Character(int team, Role r);

    int GetTeamId() const { return teamId; }
    Role GetRole() const { return role; }

    double GetHP() const { return hp; }
    double GetMaxHP() const { return maxHp; }
    int GetAmmo() const { return ammo; }
    int GetMaxAmmo() const { return maxAmmo; }

    int GetSupplyStock() const { return supplyStock; }
    int GetMedStock() const { return medStock; }

    void ConsumeSupplyPack();
    void ConsumeMedPack();
    void RefillSupplyStock();
    void RefillMedStock();

    const Personality& GetPersonality() const { return p; }
    bool IsDead() const { return hp <= 0.0; }

    void Damage(double amount);
    void Heal(double amount);

    void UseAmmo(int amount);
    void RefillAmmo();

    void TickCooldowns();

    void MoveToCell(int r, int c);

    void SeekEnemyInRoomOrNearby();
    void SeekSupply();
    void SeekMedic();

    void SupplyServeAlliesSameCell();
    void MedicHealAlliesSameCell();

    bool CanShoot() const { return role == Role::Fighter && ammo > 0 && shootCD == 0; }
    bool CanGrenade() const { return role == Role::Fighter && grenadeCD == 0; }

    void OnShootFired() { UseAmmo(1); shootCD = 18; }
    void OnGrenadeFired() { grenadeCD = 90; }

    void RequestMedic() { needsMedic = true; }
    void ClearMedicRequest() { needsMedic = false; }
    bool NeedsMedic() const { return needsMedic; }

    void show();
};
