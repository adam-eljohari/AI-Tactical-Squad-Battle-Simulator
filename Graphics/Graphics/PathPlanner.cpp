#include "PathPlanner.h"
#include "Constants.h"
#include "World.h"
#include "Character.h"
#include <queue>
#include <cmath>

extern int maze[MSZ][MSZ];

static inline bool InBounds(int r, int c)
{
    return r >= 0 && r < MSZ && c >= 0 && c < MSZ;
}

static inline bool IsWalkableTile(int r, int c)
{
    int t = maze[r][c];
    return (t == SPACE || t == AMMO_DEPOT || t == MED_DEPOT);
}

static inline bool IsCorridor(int r, int c)
{
    return World::I().GetRoomIndexByCell(r, c) < 0;
}


static bool BlockedByEnemyInCorridor(int r, int c, int myTeam)
{
    if (!IsCorridor(r, c)) return false;

    for (size_t i = 0; i < World::I().actors.size(); i++)
    {
        Character* a = World::I().actors[i];
        if (!a || a->IsDead()) continue;
        if (a->GetTeamId() == myTeam) continue;

        int ar = (int)std::round(a->getY());
        int ac = (int)std::round(a->getX());

        if (ar == r && ac == c)
            return true;
    }
    return false;
}

bool PathPlanner::FindPathTeam(int sr, int sc, int tr, int tc, int teamId, std::vector<GridPos>& outPath)
{
    outPath.clear();
    if (!InBounds(sr, sc) || !InBounds(tr, tc)) return false;
    if (!IsWalkableTile(sr, sc) || !IsWalkableTile(tr, tc)) return false;


    static bool visited[MSZ][MSZ];
    static int  prevR[MSZ][MSZ];
    static int  prevC[MSZ][MSZ];

    for (int r = 0; r < MSZ; r++)
        for (int c = 0; c < MSZ; c++)
        {
            visited[r][c] = false;
            prevR[r][c] = -1;
            prevC[r][c] = -1;
        }

    std::queue<GridPos> q;
    q.push(GridPos(sr, sc));
    visited[sr][sc] = true;

    const int dr[4] = { 1, -1, 0, 0 };
    const int dc[4] = { 0, 0, 1, -1 };

    bool found = false;

    while (!q.empty())
    {
        GridPos cur = q.front();
        q.pop();

        if (cur.r == tr && cur.c == tc)
        {
            found = true;
            break;
        }

        for (int k = 0; k < 4; k++)
        {
            int nr = cur.r + dr[k];
            int nc = cur.c + dc[k];
            if (!InBounds(nr, nc)) continue;
            if (visited[nr][nc]) continue;
            if (!IsWalkableTile(nr, nc)) continue;

            if (!(nr == tr && nc == tc)) 
            {
                if (BlockedByEnemyInCorridor(nr, nc, teamId))
                    continue;
            }

            visited[nr][nc] = true;
            prevR[nr][nc] = cur.r;
            prevC[nr][nc] = cur.c;
            q.push(GridPos(nr, nc));
        }
    }

    if (!found) return false;

    
    int r = tr, c = tc;
    std::vector<GridPos> rev;
    rev.push_back(GridPos(r, c));

    while (!(r == sr && c == sc))
    {
        int pr = prevR[r][c];
        int pc = prevC[r][c];
        if (pr < 0 || pc < 0) break;
        r = pr; c = pc;
        rev.push_back(GridPos(r, c));
    }

    
    outPath.reserve(rev.size());
    for (int i = (int)rev.size() - 1; i >= 0; i--)
        outPath.push_back(rev[i]);

    return (outPath.size() >= 2);
}
