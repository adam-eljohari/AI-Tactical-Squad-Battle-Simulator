#include "MapItems.h"
#include "Room.h"
#include <cstdlib>

static void RoomBounds(Room* room, int& sr, int& er, int& sc, int& ec)
{
    sr = room->getCenterRow() - room->getHeight() / 2 + 1;
    er = room->getCenterRow() + room->getHeight() / 2 - 1;
    sc = room->getCenterCol() - room->getWidth() / 2 + 1;
    ec = room->getCenterCol() + room->getWidth() / 2 - 1;
}

void PlaceRandomInRooms(Room** rooms, int numRooms, int maze[MSZ][MSZ], int tileType, int count)
{
    int placed = 0;
    while (placed < count)
    {
        int ri = rand() % numRooms;
        int sr, er, sc, ec;
        RoomBounds(rooms[ri], sr, er, sc, ec);

        int r = sr + rand() % (er - sr + 1);
        int c = sc + rand() % (ec - sc + 1);

        if (maze[r][c] == SPACE)
        {
            maze[r][c] = tileType;
            placed++;
        }
    }
}

void PlaceProjectItems(Room** rooms, int numRooms, int maze[MSZ][MSZ])
{
    PlaceRandomInRooms(rooms, numRooms, maze, AMMO_DEPOT, 2);
    PlaceRandomInRooms(rooms, numRooms, maze, MED_DEPOT, 2);
    PlaceRandomInRooms(rooms, numRooms, maze, COVER, 28);
}
