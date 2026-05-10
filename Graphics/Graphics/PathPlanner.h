#pragma once
#include <vector>
#include "NPC.h"

class PathPlanner
{
public:
    static bool FindPathTeam(int sr, int sc, int tr, int tc, int teamId, std::vector<GridPos>& outPath);

    static bool FindPath(int sr, int sc, int tr, int tc, std::vector<GridPos>& outPath)
    {
        return FindPathTeam(sr, sc, tr, tc, 0, outPath);
    }
};
