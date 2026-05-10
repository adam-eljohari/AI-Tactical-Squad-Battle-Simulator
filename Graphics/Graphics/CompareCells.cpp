#include "CompareCells.h"

bool CompareCells::operator()(const Cell* a, const Cell* b) const
{
    return a->GetF() > b->GetF();
}
