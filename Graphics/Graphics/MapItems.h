#pragma once
#include "Constants.h"
class Room;

void PlaceRandomInRooms(Room** rooms, int numRooms, int maze[MSZ][MSZ], int tileType, int count);
void PlaceProjectItems(Room** rooms, int numRooms, int maze[MSZ][MSZ]);
