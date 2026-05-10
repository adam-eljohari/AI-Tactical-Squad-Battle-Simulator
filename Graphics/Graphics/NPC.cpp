#include "NPC.h"
#include "State.h"
#include "glut.h"
#include <cmath>

NPC::NPC() {}

NPC::~NPC()
{
    if (pCurrentState) delete pCurrentState;
}

void NPC::setPos(double nx, double ny)
{
    x = nx;
    y = ny;
    targetX = nx;
    targetY = ny;

    isMoving = false;
    path.clear();
    pathIndex = 0;
}

void NPC::show()
{
    glColor3d(0, 0, 0);
    glBegin(GL_POLYGON);
    glVertex2d(x - 1, y - 1);
    glVertex2d(x - 1, y + 1);
    glVertex2d(x + 1, y + 1);
    glVertex2d(x + 1, y - 1);
    glEnd();

    glColor3d(0.8, 0.6, 0.4);
    glBegin(GL_POLYGON);
    glVertex2d(x - 0.5, y + 1);
    glVertex2d(x - 0.5, y + 2);
    glVertex2d(x + 0.5, y + 2);
    glVertex2d(x + 0.5, y + 1);
    glEnd();
}

void NPC::setTarget(double tx, double ty)
{
    targetX = tx;
    targetY = ty;

    double dx = targetX - x;
    double dy = targetY - y;
    double dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 1e-6)
    {
        isMoving = false;
        return;
    }

    setDirection();
    isMoving = true;
}

void NPC::setDirection()
{
    double dx = targetX - x;
    double dy = targetY - y;

    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-9)
    {
        dirX = 0;
        dirY = 0;
        return;
    }

    dirX = dx / len;
    dirY = dy / len;
}

void NPC::SetPath(const std::vector<GridPos>& newPath)
{
    path = newPath;
    pathIndex = 0;

    if (path.size() >= 2)
    {
        pathIndex = 1;
        setTarget((double)path[pathIndex].c,
            (double)path[pathIndex].r);
    }
    else
    {
        path.clear();
        pathIndex = 0;
        isMoving = false;
    }
}

void NPC::SetState(State* pNewState)
{
    if (pCurrentState) delete pCurrentState;
    pCurrentState = pNewState;

    if (pCurrentState)
        pCurrentState->OnEnter(this);
}

void NPC::stepMove()
{
    x += SPEED * dirX;
    y += SPEED * dirY;
}

void NPC::DoSomeWork()
{
    if (pCurrentState)
        pCurrentState->Update(this);

    if (!isMoving)
        return;

    stepMove();

    double dx = targetX - x;
    double dy = targetY - y;
    double dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 0.8)
    {
        x = targetX;
        y = targetY;

        if (!path.empty() && pathIndex + 1 < (int)path.size())
        {
            pathIndex++;
            setTarget((double)path[pathIndex].c,
                (double)path[pathIndex].r);
            return;
        }

        isMoving = false;
        path.clear();
        pathIndex = 0;

        if (pCurrentState)
            pCurrentState->Transition(this);
    }
}

void NPC::StopMoving()
{
    SetPath(std::vector<GridPos>());
}