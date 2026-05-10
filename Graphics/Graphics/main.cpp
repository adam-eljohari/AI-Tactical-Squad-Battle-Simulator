#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <vector>
#include <queue>
#include <iostream>

#include "glut.h"
#include "Constants.h"
#include "Room.h"
#include "MedicStates.h" 
#include "Cell.h"
#include "CompareCells.h"

#include "World.h"
#include "MapItems.h"
#include "GameSetup.h"
#include "GameTick.h"
#include "ProjectileManager.h"

const int W = 600;
const int H = 600;
const int NUM_ROOMS = 8;

int maze[MSZ][MSZ] = { 0 };
int map[MSZ][MSZ] = { 0 };
double securityMap[MSZ][MSZ] = { 0 };

Room* rooms[NUM_ROOMS] = { 0 };


static inline bool InBoundsRC(int r, int c)
{
    return r >= 0 && r < MSZ && c >= 0 && c < MSZ;
}

static inline bool PassableForConnectivity(int r, int c)
{
    return maze[r][c] != WALL;
}

static bool IsAlreadyConnected(int sr, int sc, int tr, int tc)
{
    if (!InBoundsRC(sr, sc) || !InBoundsRC(tr, tc)) return false;
    if (!PassableForConnectivity(sr, sc) || !PassableForConnectivity(tr, tc)) return false;

    static bool vis[MSZ][MSZ];
    for (int r = 0; r < MSZ; r++)
        for (int c = 0; c < MSZ; c++)
            vis[r][c] = false;

    std::queue<std::pair<int, int>> q;
    q.push({ sr, sc });
    vis[sr][sc] = true;

    const int dr[4] = { 1,-1,0,0 };
    const int dc[4] = { 0,0,1,-1 };

    while (!q.empty())
    {
         std::pair<int, int> p = q.front();
    q.pop();

    int r = p.first;
    int c = p.second;

        if (r == tr && c == tc) return true;

        for (int k = 0; k < 4; k++)
        {
            int nr = r + dr[k];
            int nc = c + dc[k];
            if (!InBoundsRC(nr, nc)) continue;
            if (vis[nr][nc]) continue;
            if (!PassableForConnectivity(nr, nc)) continue;

            vis[nr][nc] = true;
            q.push({ nr, nc });
        }
    }
    return false;
}

struct DSU
{
    int p[NUM_ROOMS];
    int rk[NUM_ROOMS];

    void init()
    {
        for (int i = 0; i < NUM_ROOMS; i++) { p[i] = i; rk[i] = 0; }
    }
    int find(int a)
    {
        if (p[a] == a) return a;
        p[a] = find(p[a]);
        return p[a];
    }
    bool unite(int a, int b)
    {
        a = find(a); b = find(b);
        if (a == b) return false;
        if (rk[a] < rk[b]) std::swap(a, b);
        p[b] = a;
        if (rk[a] == rk[b]) rk[a]++;
        return true;
    }
};


void generateDungeon();
bool overlap(int index, int row, int col, int w, int h);
void addRoom(Room* prm);
void BuildPath(int i, int j);


void BuildPath(int i, int j)
{
    int r1 = rooms[i]->getCenterRow();
    int c1 = rooms[i]->getCenterCol();
    int r2 = rooms[j]->getCenterRow();
    int c2 = rooms[j]->getCenterCol();

    if (IsAlreadyConnected(r1, c1, r2, c2))
        return;

    int c = c1;
    while (c != c2)
    {
        if (maze[r1][c] == WALL) maze[r1][c] = SPACE;
        c += (c2 > c1) ? 1 : -1;
    }

    int r = r1;
    while (r != r2)
    {
        if (maze[r][c2] == WALL) maze[r][c2] = SPACE;
        r += (r2 > r1) ? 1 : -1;
    }

    maze[r1][c1] = SPACE;
    maze[r2][c2] = SPACE;
}

void generateDungeon()
{
    int gap = 6;

    // fill with walls
    for (int i = 0; i < MSZ; i++)
        for (int j = 0; j < MSZ; j++)
            maze[i][j] = WALL;

    // rooms
    for (int i = 0; i < NUM_ROOMS; i++)
    {
        int r, c, w, h;
        int hrange, vrange;

        do
        {
            w = 8 + rand() % 20;
            h = 8 + rand() % 20;

            vrange = MSZ - 2 * (gap + h / 2);
            hrange = MSZ - 2 * (gap + w / 2);

            r = gap + h / 2 + rand() % vrange;
            c = gap + w / 2 + rand() % hrange;

        } while (overlap(i, r, c, w, h));

        rooms[i] = new Room(r, c, w, h);
        addRoom(rooms[i]);
    }

    DSU dsu;
    dsu.init();

    for (int i = 1; i < NUM_ROOMS; i++)
    {
        int r1 = rooms[i]->getCenterRow();
        int c1 = rooms[i]->getCenterCol();

        int bestJ = -1;
        int bestD2 = 1e9;

        for (int j = 0; j < i; j++)
        {
            if (dsu.find(i) == dsu.find(j)) continue;

            int r2 = rooms[j]->getCenterRow();
            int c2 = rooms[j]->getCenterCol();
            int dr = r2 - r1;
            int dc = c2 - c1;
            int d2 = dr * dr + dc * dc;

            if (d2 < bestD2)
            {
                bestD2 = d2;
                bestJ = j;
            }
        }

        if (bestJ != -1)
        {
            int tr = rooms[bestJ]->getCenterRow();
            int tc = rooms[bestJ]->getCenterCol();

            if (!IsAlreadyConnected(r1, c1, tr, tc))
                BuildPath(i, bestJ);

            dsu.unite(i, bestJ);
        }
    }
}

bool overlap(int index, int row, int col, int w, int h)
{
    int gap = 5;
    for (int i = 0; i < index; i++)
    {
        int vdist = abs(rooms[i]->getCenterRow() - row);
        int hdist = abs(rooms[i]->getCenterCol() - col);
        if (vdist - gap < h / 2 + rooms[i]->getHeight() / 2 &&
            hdist - gap < w / 2 + rooms[i]->getWidth() / 2)
            return true;
    }
    return false;
}

void addRoom(Room* prm)
{
    int start_row = prm->getCenterRow() - prm->getHeight() / 2;
    int start_col = prm->getCenterCol() - prm->getWidth() / 2;
    int end_row = prm->getCenterRow() + prm->getHeight() / 2;
    int end_col = prm->getCenterCol() + prm->getWidth() / 2;

    for (int i = start_row; i <= end_row; i++)
        for (int j = start_col; j <= end_col; j++)
            maze[i][j] = SPACE;
}



struct Bird
{
    double x;
    double y;
    double speed;
    double phase;
    double wing;
    int dir;
    double amplitude;
    double baseY;
    double color[3];
};

static Bird g_birds[2];
static int g_prevTimeMs = 0;

static void UpdateBird(Bird& b, double dt)
{
    if (dt <= 0.0) return;

    b.x += b.speed * dt * b.dir;

    b.phase += dt * 2.0;
    b.y = b.baseY + std::sin(b.x * 0.08 + b.phase) * b.amplitude;

    const double wingSpeed = 18.0;
    b.wing += dt * wingSpeed;

    const double margin = 12.0;
    if (b.dir == 1 && b.x > MSZ + margin)
    {
        b.dir = -1;
        b.x = MSZ + margin;
    }
    else if (b.dir == -1 && b.x < -margin)
    {
        b.dir = 1;
        b.x = -margin;
    }
}

static void DrawBirdInstance(const Bird& b)
{
    double x = b.x;
    double y = b.y;
    double dir = (b.dir == 1) ? 1.0 : -1.0;
    double wingOffset = std::sin(b.wing) * 0.9;
    double r = b.color[0], g = b.color[1], bl = b.color[2];

    glPushMatrix();
    glTranslated(x, y, 0.0);
    if (dir < 0) glScaled(-1.0, 1.0, 1.0);

    glColor3d(r, g, bl);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 24; ++i)
    {
        double a = 2.0 * 3.14159265358979323846 * i / 24.0;
        double bx = std::cos(a) * 1.2;
        double by = std::sin(a) * 0.8;
        glVertex2d(bx * 1.4, by * 1.0);
    }
    glEnd();

    glColor3d(0.95, 0.95, 0.95);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 16; ++i)
    {
        double a = 2.0 * 3.14159265358979323846 * i / 16.0;
        double bx = std::cos(a) * 0.55;
        double by = std::sin(a) * 0.55;
        glVertex2d(bx + 1.25, by * 0.9);
    }
    glEnd();

    glColor3d(0.92, 0.55, 0.12);
    glBegin(GL_TRIANGLES);
    glVertex2d(1.65, 0.0);
    glVertex2d(1.25, 0.20);
    glVertex2d(1.25, -0.20);
    glEnd();

    glColor3d(0.22, 0.30, 0.50);
    glBegin(GL_POLYGON);
    glVertex2d(-0.6, 0.2 + wingOffset);
    glVertex2d(0.7, 1.8 + wingOffset * 1.2);
    glVertex2d(1.2, 1.4 + wingOffset * 0.6);
    glVertex2d(0.0, -0.2 + wingOffset * 0.1);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2d(-0.6, -0.2 - wingOffset);
    glVertex2d(0.7, -1.8 - wingOffset * 1.2);
    glVertex2d(1.2, -1.4 - wingOffset * 0.6);
    glVertex2d(0.0, 0.2 - wingOffset * 0.1);
    glEnd();

    glColor3d(0.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 24; ++i)
    {
        double a = 2.0 * 3.14159265358979323846 * i / 24.0;
        double bx = std::cos(a) * 1.4;
        double by = std::sin(a) * 1.0;
        glVertex2d(bx, by);
    }
    glEnd();

    glPopMatrix();
}

static void UpdateBirds(double dt)
{
    for (int i = 0; i < 2; ++i)
        UpdateBird(g_birds[i], dt);
}

static void DrawBirds()
{
    for (int i = 0; i < 2; ++i)
        DrawBirdInstance(g_birds[i]);
}


void init()
{
    srand((unsigned)time(0));
    glClearColor(0.0, 0.0, 0.0, 0);
    glOrtho(0, MSZ, 0, MSZ, -1, 1);

    generateDungeon();
    PlaceProjectItems((Room**)rooms, NUM_ROOMS, maze);

    for (int r = 0; r < MSZ; r++)
        for (int c = 0; c < MSZ; c++)
            securityMap[r][c] = 0.0;

    SetupGame((Room**)rooms, NUM_ROOMS);

    

    bool anyMedic = false;
    for (auto* a : World::I().actors)
    {
        if (!a) continue;
        Character* c = dynamic_cast<Character*>(a);
        if (!c) continue;
        int r = (int)std::round(c->getY());
        int col = (int)std::round(c->getX());
        std::printf("[Actor] Role=%d Team=%d Pos=(%d,%d) HP=%.1f Ammo=%d MedStock=%d\n",
            (int)c->GetRole(), c->GetTeamId(), r, col, c->GetHP(), c->GetAmmo(), c->GetMedStock());

        if (c->GetRole() == Role::Medic)
        {
            anyMedic = true;
            if (!c->GetState())
            {
                c->SetState(new MedicDecideState());
                std::printf("[ActorInit] Assigned MedicDecideState to medic at (%d,%d)\n", r, col);
            }
        }
    }

    if (!anyMedic)
        std::printf("[ActorInit] WARNING: no medics were spawned\n");

    g_prevTimeMs = glutGet(GLUT_ELAPSED_TIME);

    // bird 0
    g_birds[0].x = -20.0;
    g_birds[0].baseY = MSZ * 0.70;
    g_birds[0].y = g_birds[0].baseY;
    g_birds[0].speed = 28.0;
    g_birds[0].phase = 0.0;
    g_birds[0].wing = 0.0;
    g_birds[0].dir = 1;
    g_birds[0].amplitude = 6.0;
    g_birds[0].color[0] = 0.95; g_birds[0].color[1] = 0.85; g_birds[0].color[2] = 0.35;

    // bird 1
    g_birds[1].x = MSZ + 20.0;
    g_birds[1].baseY = MSZ * 0.78;
    g_birds[1].y = g_birds[1].baseY;
    g_birds[1].speed = 20.0;
    g_birds[1].phase = 1.2;
    g_birds[1].wing = 0.5;
    g_birds[1].dir = -1;
    g_birds[1].amplitude = 8.0;
    g_birds[1].color[0] = 0.40; g_birds[1].color[1] = 0.65; g_birds[1].color[2] = 0.95;
}

void DrawDungeon()
{

    for (int i = 0; i < MSZ; i++)
        for (int j = 0; j < MSZ; j++)
        {
            switch (maze[i][j])
            {
            case WALL:       glColor3d(0.4, 0.0, 0.0); break;
            case SPACE:      glColor3d(1 - securityMap[i][j], 1 - securityMap[i][j], 1 - securityMap[i][j]); break;
            case AMMO_DEPOT: glColor3d(0.2, 0.2, 0.9); break;
            case MED_DEPOT:  glColor3d(0.2, 0.85, 0.2); break;
            case COVER:      glColor3d(0.25, 0.25, 0.25); break;
            default:         glColor3d(0.8, 0.8, 0.8); break;
            }

            glBegin(GL_POLYGON);
            glVertex2d(j, i);
            glVertex2d(j, i + 1);
            glVertex2d(j + 1, i + 1);
            glVertex2d(j + 1, i);
            glEnd();
        }

    glColor3d(0, 0, 0);
    for (int i = 0; i < NUM_ROOMS; i++)
    {
        glRasterPos2d(rooms[i]->getCenterCol(), rooms[i]->getCenterRow());
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, '0' + i);
    }
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    

    DrawDungeon();

    for (auto* a : World::I().actors)
        if (a && !a->IsDead())
            a->show();

    ProjectileManager::I().Draw();

    DrawBirds();

    glutSwapBuffers();
}

void idle()
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    int dtms = now - g_prevTimeMs;
    if (dtms < 0) dtms = 0;
    double dt = dtms * 0.001;
    g_prevTimeMs = now;

    UpdateBirds(dt);

    if (!GameTick())
        exit(0);

    glutPostRedisplay();
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(W, H);
    glutInitWindowPosition(400, 100);
    glutCreateWindow("AI Project Final");

    glutDisplayFunc(display);
    glutIdleFunc(idle);

    init();
    glutMainLoop();
    return 0;
}
