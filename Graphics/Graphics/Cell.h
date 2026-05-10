#pragma once
#include "Constants.h"

class Cell
{
private:
    int row = 0;
    int col = 0;

    double g = 0.0; 
    double h = 0.0; 
    double f = 0.0; // g + h

    Cell* parent = nullptr;

public:
    Cell() = default;
    Cell(int r, int c);

    void Set(int r, int c);

    int GetRow() const { return row; }
    int GetCol() const { return col; }

    void SetG(double val);
    void SetH(double val);
    void UpdateF();

    double GetG() const { return g; }
    double GetH() const { return h; }
    double GetF() const { return f; }

    void SetParent(Cell* p) { parent = p; }
    Cell* GetParent() const { return parent; }
};
