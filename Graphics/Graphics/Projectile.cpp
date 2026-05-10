#include "Projectile.h"
#include "World.h"
#include "Character.h"
#include "glut.h"
#include <cmath>
#include <algorithm>

extern int maze[MSZ][MSZ];

static inline bool InBounds(int r, int c) { return r >= 0 && r < MSZ && c >= 0 && c < MSZ; }

Bullet::Bullet(double sx, double sy, double dx, double dy, int team)
{
    x = sx; y = sy; teamId = team;
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-9) { dirX = 1; dirY = 0; }
    else { dirX = dx / len; dirY = dy / len; }

    x += 1.2 * dirX;
    y += 1.2 * dirY;
}


void Bullet::Move()
{
    if (!moving) return;

    x += speed * dirX;
    y += speed * dirY;

    int r = (int)std::round(y);
    int c = (int)std::round(x);
    if (!InBounds(r, c) || maze[r][c] == WALL || maze[r][c] == COVER)
    {
        moving = false;
        return;
    }

    CheckHit();
}

void Bullet::CheckHit()
{
    int r = (int)std::round(y);
    int c = (int)std::round(x);
    int room = World::I().GetRoomIndexByCell(r, c);
    if (room < 0) return;

    for (auto* a : World::I().actors)
    {
        if (!a || a->IsDead()) continue;
        if (a->GetTeamId() == teamId) continue;

        int ar = (int)std::round(a->getY());
        int ac = (int)std::round(a->getX());
        if (World::I().GetRoomIndexByCell(ar, ac) != room) continue;

        double dx = a->getX() - x;
        double dy = a->getY() - y;
        if (dx * dx + dy * dy <= 1.2 * 1.2)
        {
            a->Damage(12.0);
            moving = false;
            return;
        }
    }
}


void Bullet::show() const
{
    if (!moving) return;

    glColor3d(1, 0, 0);
    glBegin(GL_POLYGON);
    glVertex2d(x - 0.5, y);
    glVertex2d(x, y + 0.5);
    glVertex2d(x + 0.5, y);
    glVertex2d(x, y - 0.5);
    glEnd();

    glColor3d(0, 0, 0);
    glBegin(GL_LINE_LOOP);
    glVertex2d(x - 0.5, y);
    glVertex2d(x, y + 0.5);
    glVertex2d(x + 0.5, y);
    glVertex2d(x, y - 0.5);
    glEnd();
}


Grenade::Grenade(double sx, double sy, int team)
{
    x = sx; y = sy; teamId = team;
}

void Grenade::Tick()
{
    if (timer > 0)
    {
        timer--;
        if (timer == 0) Explode();
    }
}

void Grenade::Explode()
{
    exploding = true;

    int gr = (int)std::round(y);
    int gc = (int)std::round(x);
    int room = World::I().GetRoomIndexByCell(gr, gc);
    if (room < 0) { exploding = false; return; }

    for (auto* a : World::I().actors)
    {
        if (!a || a->IsDead()) continue;
        if (a->GetTeamId() == teamId) continue;

        int ar = (int)std::round(a->getY());
        int ac = (int)std::round(a->getX());
        if (World::I().GetRoomIndexByCell(ar, ac) != room) continue;

        double dx = a->getX() - x;
        double dy = a->getY() - y;
        if (dx * dx + dy * dy <= 8.0 * 8.0)
            a->Damage(10.0);
    }


    exploding = false;
}

void Grenade::show() const
{
    if (timer <= 0 && !exploding) return;

    glColor3d(0.2, 0.2, 0.2);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 18; i++)
    {
        double ang = 2.0 * 3.1415926 * i / 18.0;
        glVertex2d(x + 0.55 * std::cos(ang), y + 0.55 * std::sin(ang));
    }
    glEnd();

}
