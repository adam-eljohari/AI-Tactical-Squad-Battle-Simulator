#pragma once

class Room
{
private:
    int centerRow = 0;
    int centerCol = 0;
    int width = 0;
    int height = 0;

public:
    Room(int r, int c, int w, int h);

    int getCenterRow() const { return centerRow; }
    int getCenterCol() const { return centerCol; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};
