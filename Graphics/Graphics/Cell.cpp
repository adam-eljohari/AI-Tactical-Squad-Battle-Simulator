#include "Cell.h"

Cell::Cell(int r, int c)
{
    row = r;
    col = c;
    g = 0.0;
    h = 0.0;
    f = 0.0;
    parent = nullptr;
}

void Cell::Set(int r, int c)
{
    row = r;
    col = c;
}

void Cell::SetG(double val)
{
    g = val;
    UpdateF();
}

void Cell::SetH(double val)
{
    h = val;
    UpdateF();
}

void Cell::UpdateF()
{
    f = g + h;
}
