#pragma once
#include "Cell.h"

class CompareCells
{
public:
    bool operator()(const Cell* a, const Cell* b) const;
};
