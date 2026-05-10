#include "Character.h"
#include "World.h"
#include "PathPlanner.h"
#include "glut.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstdio>

extern int maze[MSZ][MSZ];

static Personality RandomPersonality()
{
    Personality p;
    int r = rand() % 3;
    if (r == 0)
    {
        p.lowHpThreshold = 25.0;
        p.lowAmmoThreshold = 2;
        p.attackRange = 22.0;
        p.grenadeRange = 30.0;
        p.braveFactor = 0.85;
    }
    else if (r == 1)
    {
        p.lowHpThreshold = 55.0;
        p.lowAmmoThreshold = 6;
        p.attackRange = 14.0;
        p.grenadeRange = 22.0;
        p.braveFactor = 0.25;
    }
    else
    {
        p.lowHpThreshold = 35.0;
        p.lowAmmoThreshold = 4;
        p.attackRange = 18.0;
        p.grenadeRange = 26.0;
        p.braveFactor = 0.55;
    }
    return p;
}

static inline double Dist(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1, dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

Character::Character(int team, Role r)
{
    teamId = team;
    role = r;

    maxHp = 100.0;
    hp = maxHp;

    if (role == Role::Fighter) { maxAmmo = 12; ammo = maxAmmo; }
    else { maxAmmo = 0; ammo = 0; }

    if (role == Role::Supply) supplyStock = 8;
    if (role == Role::Medic)  medStock = 8;

    p = RandomPersonality();
}

void Character::Damage(double amount) 
{ 
    hp = std::max(0.0, hp - amount);
    if (role == Role::Fighter)
    {
        const double thr = GetPersonality().lowHpThreshold;
        if (hp <= thr)
        {
            RequestMedic();
            std::printf("[Character] Fighter (team %d) requested medic: HP=%.1f / %.1f threshold=%.1f\n", 
                GetTeamId(), hp, GetMaxHP(), thr);
        }
    }
}

void Character::Heal(double amount) 
{
    while (hp < maxHp)
    {
        hp += 0.00001;
    }
    
        
}

void Character::UseAmmo(int amount) { ammo = std::max(0, ammo - amount); }
void Character::RefillAmmo() { ammo = maxAmmo; }

void Character::ConsumeSupplyPack() { if (supplyStock > 0) supplyStock--; }
void Character::ConsumeMedPack() { if (medStock > 0) medStock--; }

void Character::RefillSupplyStock() { supplyStock = 8; }
void Character::RefillMedStock() 
{ 
    medStock = 8; 
    std::printf("[Medic] Refilled med stock -> %d\n", medStock); 
}

void Character::TickCooldowns()
{
    if (shootCD > 0) shootCD--;
    if (grenadeCD > 0) grenadeCD--;
}

void Character::MoveToCell(int r, int c)
{
    int sr = (int)std::round(getY());
    int sc = (int)std::round(getX());
        
    std::vector<GridPos> pth;

    if (PathPlanner::FindPathTeam(sr, sc, r, c, GetTeamId(), pth))
    {
        SetPath(pth);
        std::printf("[MoveToCell] Path found from (%d,%d) -> (%d,%d)\n", sr, sc, r, c);
        return;
    }
    else
    {
        std::printf("[MoveToCell] PathPlanner failed for (%d,%d) -> (%d,%d)\n", sr, sc, r, c);
    }

    
    int targetRoom = World::I().GetRoomIndexByCell(r, c);
    CellPos nearPos;
    if (targetRoom >= 0 && World::I().FindNearestTile(SPACE, r, c, nearPos, true, targetRoom))
    {
        if (PathPlanner::FindPathTeam(sr, sc, nearPos.r, nearPos.c, GetTeamId(), pth))
        {
            SetPath(pth);
            std::printf("[MoveToCell] Fallback path to nearest SPACE in room %d: (%d,%d)\n", targetRoom, nearPos.r, nearPos.c);
            return;
        }
        else
        {
            std::printf("[MoveToCell] Fallback path failed to nearest SPACE in room %d: (%d,%d)\n", targetRoom, nearPos.r, nearPos.c);
        }
    }

    if (World::I().FindNearestTile(SPACE, r, c, nearPos, false, -1))
    {
        if (PathPlanner::FindPathTeam(sr, sc, nearPos.r, nearPos.c, GetTeamId(), pth))
        {
            SetPath(pth);
            std::printf("[MoveToCell] Fallback path to global nearest SPACE: (%d,%d)\n", nearPos.r, nearPos.c);
            return;
        }
        else
        {
            std::printf("[MoveToCell] Fallback global path failed to (%d,%d)\n", nearPos.r, nearPos.c);
        }
    }

    
    if (World::I().InBounds(r, c) && World::I().Passable(r, c))
    {
        if (PathPlanner::FindPathTeam(sr, sc, r, c, GetTeamId(), pth))
        {
            SetPath(pth);
            std::printf("[MoveToCell] Final A* succeeded to exact cell (%d,%d)\n", r, c);
            return;
        }
        else
        {
            bool losClear = true;
            int dr = r - sr;
            int dc = c - sc;
            int steps = std::max(std::abs(dr), std::abs(dc));
            if (steps > 0)
            {
                for (int k = 1; k <= steps; ++k)
                {
                    int rr = sr + (dr * k) / steps;
                    int cc = sc + (dc * k) / steps;
                    if (!World::I().InBounds(rr, cc) || !World::I().Passable(rr, cc))
                    {
                        losClear = false;
                        break;
                    }
                }
            }
            if (losClear)
            {
                setTarget((double)c, (double)r);
                std::printf("[MoveToCell] LOS direct move allowed to (%d,%d)\n", r, c);
                return;
            }
            else
            {
                std::printf("[MoveToCell] No safe movement found to (%d,%d) — staying at (%d,%d)\n", r, c, sr, sc);
            }
        }
    }
    else
    {
        std::printf("[MoveToCell] Target (%d,%d) out of bounds or not passable\n", r, c);
    }

    return;
}

void Character::SeekEnemyInRoomOrNearby()
{
    auto enemiesSame = World::I().EnemiesInSameRoom(this);
    if (!enemiesSame.empty())
    {
        Character* best = nullptr;
        double bestD = 1e18;
        for (auto* e : enemiesSame)
        {
            double d = Dist(getX(), getY(), e->getX(), e->getY());
            if (d < bestD) { bestD = d; best = e; }
        }
        if (best)
        {
            MoveToCell((int)std::round(best->getY()), (int)std::round(best->getX()));
            return;
        }
    }

    auto enemies = World::I().EnemiesAnywhere(this);
    if (enemies.empty()) return;

    Character* best = nullptr;
    double bestD2 = 1e18;

    int myr = (int)std::round(getY());
    int myc = (int)std::round(getX());

    for (auto* e : enemies)
    {
        int er = (int)std::round(e->getY());
        int ec = (int)std::round(e->getX());
        double d2 = double((er - myr) * (er - myr) + (ec - myc) * (ec - myc));
        if (d2 < bestD2) { bestD2 = d2; best = e; }
    }

    if (best)
        MoveToCell((int)std::round(best->getY()), (int)std::round(best->getX()));
}

void Character::SeekSupply()
{
    Character* s = World::I().FindNearestAllyByRole(this, Role::Supply);
    if (!s) return;
    MoveToCell((int)std::round(s->getY()), (int)std::round(s->getX()));
}

void Character::SeekMedic()
{
    Character* m = World::I().FindNearestAllyByRole(this, Role::Medic);
    if (!m) return;
    MoveToCell((int)std::round(m->getY()), (int)std::round(m->getX()));
}

void Character::SupplyServeAlliesSameCell()
{
    if (role != Role::Supply) return;
    if (supplyStock <= 0) return;

    int r = (int)std::round(getY());
    int c = (int)std::round(getX());

    for (auto* a : World::I().actors)
    {
        if (!a || a->IsDead()) continue;
        if (a->GetTeamId() != GetTeamId()) continue;
        if (a->GetRole() != Role::Fighter) continue;

        int ar = (int)std::round(a->getY());
        int ac = (int)std::round(a->getX());
        if (ar == r && ac == c)
        {
            if (a->GetAmmo() < a->GetMaxAmmo())
            {
                a->RefillAmmo();
                ConsumeSupplyPack();
                if (supplyStock <= 0) return;
            }
        }
    }
}

void Character::MedicHealAlliesSameCell()
{
    if (role != Role::Medic) return;
    if (medStock <= 0)
    {
        std::printf("[MedicHeal] Medic at (%.2f,%.2f) out of stock\n", getX(), getY());
        return;
    }

    double mx = getX();
    double my = getY();
    const double HEAL_RANGE = 25.0;
    const double HEAL_AMOUNT = 35.0;

    int mroom = World::I().GetRoomIndexByCell((int)std::round(my), (int)std::round(mx));
    int myHome = World::I().GetHomeRoom(GetTeamId());
    std::printf("[MedicHeal] Medic scanning from (%.2f,%.2f) room=%d home=%d range=%.1f medStock=%d\n",
        mx, my, mroom, myHome, HEAL_RANGE, medStock);

    for (auto* a : World::I().actors)
    {
        if (!a || a->IsDead()) continue;
        if (a->GetTeamId() != GetTeamId()) continue;
        if (a == this) continue;

        int ar = (int)std::round(a->getY());
        int ac = (int)std::round(a->getX());
        int aroom = World::I().GetRoomIndexByCell(ar, ac);

        double ax = a->getX();
        double ay = a->getY();
        double dx = ax - mx;
        double dy = ay - my;
        double dist = std::sqrt(dx * dx + dy * dy);

        bool sameRoom = (aroom == mroom);
        bool inHomeRoom = (aroom >= 0 && aroom == myHome);
        bool explicitRequest = a->NeedsMedic();

        std::printf("[MedicHeal] Candidate at (%.2f,%.2f) role=%d HP=%.1f/%.1f room=%d dist=%.2f req=%d\n",
            ax, ay, (int)a->GetRole(), a->GetHP(), a->GetMaxHP(), aroom, dist, (int)explicitRequest);

        if (dist <= HEAL_RANGE && (sameRoom || explicitRequest || inHomeRoom))
        {
            if (a->GetHP() < a->GetMaxHP() && medStock > 0)
            {
                double before = a->GetHP();
                a->Heal(HEAL_AMOUNT);
                a->ClearMedicRequest();
                ConsumeMedPack();
                std::printf("[Medic] Healed ally at (%.2f,%.2f) -> HP now %.1f / %.1f ; medStock=%d (before=%.1f)\n",
                    ax, ay, a->GetHP(), a->GetMaxHP(), medStock, before);

                if (medStock <= 0)
                {
                    std::printf("[MedicHeal] Medic out of stock\n");
                    break;
                }
            }
        }
    }

    while (medStock > 0 && GetHP() < GetMaxHP())
    {
        double before = GetHP();
        Heal(HEAL_AMOUNT);
        ConsumeMedPack();
        std::printf("[Medic] Self-healed -> HP now %.1f / %.1f ; medStock=%d (before=%.1f)\n",
            GetHP(), GetMaxHP(), medStock, before);

        if (GetHP() == before)
            break;
    }
}


void Character::show()
{
    if (IsDead()) return;

    double x = getX();
    double y = getY();

    // ================= BODY =================
    if (teamId == 0) glColor3d(0.10, 0.30, 1.00);
    else             glColor3d(1.00, 0.20, 0.20);

    glBegin(GL_POLYGON);
    glVertex2d(x - 1, y - 1);
    glVertex2d(x - 1, y + 1);
    glVertex2d(x + 1, y + 1);
    glVertex2d(x + 1, y - 1);
    glEnd();

    // ================= HEAD =================
    if (teamId == 0) glColor3d(0.55, 0.75, 1.00);
    else             glColor3d(1.00, 0.65, 0.65);

    glBegin(GL_POLYGON);
    glVertex2d(x - 0.5, y + 1);
    glVertex2d(x - 0.5, y + 2);
    glVertex2d(x + 0.5, y + 2);
    glVertex2d(x + 0.5, y + 1);
    glEnd();

    // ================= MEDIC HAT =================
    if (role == Role::Medic)
    {
        glColor3d(1.0, 1.0, 1.0);

        glBegin(GL_POLYGON);
        glVertex2d(x - 1.0, y + 2);
        glVertex2d(x - 1.0, y + 2.8);
        glVertex2d(x + 1.0, y + 2.8);
        glVertex2d(x + 1.0, y + 2);
        glEnd();

        glColor3d(0.6, 0.6, 0.6);
        glBegin(GL_LINE_LOOP);
        glVertex2d(x - 1.0, y + 2);
        glVertex2d(x - 1.0, y + 2.8);
        glVertex2d(x + 1.0, y + 2.8);
        glVertex2d(x + 1.0, y + 2);
        glEnd();

        glColor3d(1.0, 0.1, 0.1);
        glBegin(GL_POLYGON);
        glVertex2d(x - 0.15, y + 2.2);
        glVertex2d(x - 0.15, y + 2.6);
        glVertex2d(x + 0.15, y + 2.6);
        glVertex2d(x + 0.15, y + 2.2);
        glEnd();

        glBegin(GL_POLYGON);
        glVertex2d(x - 0.4, y + 2.35);
        glVertex2d(x - 0.4, y + 2.5);
        glVertex2d(x + 0.4, y + 2.5);
        glVertex2d(x + 0.4, y + 2.35);
        glEnd();

        glColor3d(0,0,0);

        
    }

    // ================= BIGGER WEAPON (FIGHTER ONLY) =================
    if (role == Role::Fighter)
    {
        double dir = (teamId == 0) ? 1.0 : -1.0;

        // ===== Main body (bigger) =====
        glColor3d(0.18, 0.18, 0.18);
        glBegin(GL_POLYGON);
        glVertex2d(x + dir * 0.2, y + 0.2);
        glVertex2d(x + dir * 0.2, y - 0.2);
        glVertex2d(x + dir * 1.6, y - 0.2);
        glVertex2d(x + dir * 1.6, y + 0.2);
        glEnd();

        // ===== Barrel (longer & thicker) =====
        glColor3d(0.05, 0.05, 0.05);
        glBegin(GL_POLYGON);
        glVertex2d(x + dir * 1.6, y + 0.12);
        glVertex2d(x + dir * 1.6, y - 0.12);
        glVertex2d(x + dir * 2.3, y - 0.12);
        glVertex2d(x + dir * 2.3, y + 0.12);
        glEnd();

        // ===== Handle (bigger grip) =====
        glColor3d(0.10, 0.10, 0.10);
        glBegin(GL_POLYGON);
        glVertex2d(x + dir * 0.8, y - 0.2);
        glVertex2d(x + dir * 0.6, y - 0.2);
        glVertex2d(x + dir * 0.4, y - 0.7);
        glVertex2d(x + dir * 0.6, y - 0.7);
        glEnd();

        // ===== Outline =====
        glColor3d(0, 0, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2d(x + dir * 0.2, y + 0.2);
        glVertex2d(x + dir * 0.2, y - 0.2);
        glVertex2d(x + dir * 1.6, y - 0.2);
        glVertex2d(x + dir * 1.6, y + 0.2);
        glEnd();
    }


    // ================= BIGGER BARS =================
    double barW = 3.4;
    double barH = 0.45;

    // ===== HP BAR =====
    double hpRatio = (maxHp > 0) ? hp / maxHp : 0;
    if (hpRatio < 0) hpRatio = 0;
    if (hpRatio > 1) hpRatio = 1;

    double hpLeft = x - barW / 2;
    double hpBottom = y + 3.4;

    glColor3d(0, 0, 0);
    glBegin(GL_LINE_LOOP);
    glVertex2d(hpLeft, hpBottom);
    glVertex2d(hpLeft + barW, hpBottom);
    glVertex2d(hpLeft + barW, hpBottom + barH);
    glVertex2d(hpLeft, hpBottom + barH);
    glEnd();

    if (hpRatio > 0.6) glColor3d(0.10, 0.85, 0.10);
    else if (hpRatio > 0.3) glColor3d(0.95, 0.80, 0.10);
    else glColor3d(0.95, 0.10, 0.10);

    glBegin(GL_POLYGON);
    glVertex2d(hpLeft, hpBottom);
    glVertex2d(hpLeft + barW * hpRatio, hpBottom);
    glVertex2d(hpLeft + barW * hpRatio, hpBottom + barH);
    glVertex2d(hpLeft, hpBottom + barH);
    glEnd();

    // ===== AMMO BAR (BLUE) =====
    if (maxAmmo > 0)
    {
        double ammoRatio = (double)ammo / (double)maxAmmo;
        if (ammoRatio < 0) ammoRatio = 0;
        if (ammoRatio > 1) ammoRatio = 1;

        double ammoLeft = x - barW / 2;
        double ammoBottom = y + 2.85;

        glColor3d(0, 0, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2d(ammoLeft, ammoBottom);
        glVertex2d(ammoLeft + barW, ammoBottom);
        glVertex2d(ammoLeft + barW, ammoBottom + barH);
        glVertex2d(ammoLeft, ammoBottom + barH);
        glEnd();

        glColor3d(0.15, 0.45, 1.00);
        glBegin(GL_POLYGON);
        glVertex2d(ammoLeft, ammoBottom);
        glVertex2d(ammoLeft + barW * ammoRatio, ammoBottom);
        glVertex2d(ammoLeft + barW * ammoRatio, ammoBottom + barH);
        glVertex2d(ammoLeft, ammoBottom + barH);
        glEnd();
    }
}
