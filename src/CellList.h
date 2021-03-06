/*******************************************************************************
GPU OPTIMIZED MONTE CARLO (GOMC) 2.31
Copyright (C) 2018  GOMC Group
A copy of the GNU General Public License can be found in the COPYRIGHT.txt
along with this program, also can be found at <http://www.gnu.org/licenses/>.
********************************************************************************/
#ifndef CELLLIST_H
#define CELLLIST_H
#include "BasicTypes.h"
#include "EnsemblePreprocessor.h"
#include "BoxDimensions.h"
#include "BoxDimensionsNonOrth.h"
#include <vector>
#include <cassert>
#include <iostream>
#include <omp.h>

class Molecules;
class XYZArray;
class BoxDimensions;
class MoleculeLookup;

class CellList
{
public:
  explicit CellList(const Molecules& mols, BoxDimensions& dims);
  
  CellList& operator=(CellList const& rhs){

    list = rhs.list;

    for (int i = 0; i < BOX_TOTAL; i++){
        neighbors[i] = rhs.neighbors[i];
        head[i]      = rhs.head[i];
        cellSize[i]  = rhs.cellSize[i];
        for (int j = 0; j < 3; j++){
            edgeCells[i][j] = rhs.edgeCells[i][j];
        }
    }

    // Dimensions is a reference to the system's object
    // Doesnt need to be swapped here
    //  *dimensions = *rhs.dimensions;

    cutoff = rhs.cutoff;
    isBuilt = rhs.isBuilt;

    return *this;
}


  void SetCutoff(double cut)
  {
    cutoff = cut;
  }

  void RemoveMol(const int molIndex, const int box, const XYZArray& pos);
  void AddMol(const int molIndex, const int box, const XYZArray& pos);
  void GridAll(BoxDimensions& dims, const XYZArray& pos, const MoleculeLookup& lookup);

  // Index of cell containing position
  int PositionToCell(const XYZ& posRef, int box) const;

  // Iterates over all particles in a cell
  class Cell;
  Cell EnumerateCell(int cell, int box) const;

  // Iterates over all particles in the neighborhood of a cell
  class Neighbors;
  Neighbors EnumerateLocal(const XYZ& pos, int box) const;
  Neighbors EnumerateLocal(int cell, int box) const;

  // Iterates over all distinct, colocal pairs in a box
  class Pairs;
  Pairs EnumeratePairs(int box) const;

  int CellsInBox(int box) const
  {
    return head[box].size();
  }

  // true if every particle is a member of exactly one cell
  bool IsExhaustive() const;

//private:
  static const int END_CELL = -1;

  // Resize all boxes to match current axes
  void ResizeGrid(const BoxDimensions& dims);
  // Rebuild head/neighbor lists in box b to match current grid
  void RebuildNeighbors(int b);

  std::vector<int> list;
  std::vector<std::vector<int> > neighbors[BOX_TOTAL];
  std::vector<int> head[BOX_TOTAL];
  XYZ cellSize[BOX_TOTAL];
  int edgeCells[BOX_TOTAL][3];
  const Molecules* mols;
  BoxDimensions *dimensions;
  double cutoff;
  bool isBuilt;
};



inline int CellList::PositionToCell(const XYZ& posRef, int box) const
{
  //Transfer to unslant coordinate to find the neighbor
  XYZ pos = dimensions->TransformUnSlant(posRef, box);
  int x = (int)(pos.x / cellSize[box].x);
  int y = (int)(pos.y / cellSize[box].y);
  int z = (int)(pos.z / cellSize[box].z);
  return x * edgeCells[box][1] * edgeCells[box][2] + y * edgeCells[box][2] + z;
}

class CellList::Cell
{
public:
  Cell(int start, const std::vector<int>& list) :
    at(start), list(list.begin()) {}

  int operator*() const
  {
    return at;
  }

  void Next()
  {
    at = list[at];
  }

  bool Done()
  {
    return at == CellList::END_CELL;
  }

  void Jump(int head)
  {
    at = head;
  }

private:
// GJS
  int at;
  std::vector<int>::const_iterator list;
// GJS
};


class CellList::Neighbors
{
public:
  Neighbors(const std::vector<int>& list,
            const std::vector<int>& head,
            const std::vector<int>& neighbors);

  int operator*() const
  {
    return *cell;
  }

  bool Done() const
  {
    return (neighbor == nEnd);
  }

  void Next();

private:
// GJS
  CellList::Cell cell;
  std::vector<int>::const_iterator head;
  std::vector<int>::const_iterator neighbor, nEnd;
// GJS
};

inline CellList::Cell CellList::EnumerateCell(int cell, int box) const
{
#ifndef NDEBUG
  if(cell >= head[box].size()) {
    std::cout << "CellList.h:153: box " << box << ", Out of cell" << std::endl;
  }
#endif
  return CellList::Cell(head[box][cell], list);
}

inline CellList::Neighbors CellList::EnumerateLocal(int cell, int box) const
{
#ifndef NDEBUG
  if(cell >= head[box].size()) {
    std::cout << "CellList.h:162: box " << box << ", Out of cell" << std::endl;
    std::cout << "AxisDimensions: " << dimensions->GetAxis(box) << std::endl;
  }
#endif
  return CellList::Neighbors(list, head[box], neighbors[box][cell]);
}

inline CellList::Neighbors CellList::EnumerateLocal(const XYZ& pos, int box) const
{
  int cell = PositionToCell(pos, box);
#ifndef NDEBUG
  if(cell >= head[box].size()) {
    std::cout << "CellList.h:172: box " << box << ", pos: " << pos
              << std::endl;
    std::cout << "AxisDimensions: " << dimensions->GetAxis(box) << std::endl;
  }
#endif
  return EnumerateLocal(cell, box);
}

inline CellList::Neighbors::Neighbors(const std::vector<int>& partList,
                                      const std::vector<int>& headList, const std::vector<int>& neighbors) :
  cell(headList[neighbors[0]], partList),
  head(headList.begin()),
  neighbor(neighbors.begin()),
  nEnd(neighbors.end())
{
  while(cell.Done()) {
    ++neighbor;
    if(Done()) {
      break;
    } else {
      cell.Jump(head[*neighbor]);
    }
  }
}


inline void CellList::Neighbors::Next()
{
  cell.Next();
  // skip over empty cells
  while(cell.Done()) {
    ++neighbor;
    if(Done()) {
      break;
    } else {
      cell.Jump(head[*neighbor]);
    }
  }
  assert(!cell.Done() || Done());
}


class CellList::Pairs
{
public:
  Pairs(const CellList& cellList, int box);

  int First() const
  {
    return *cellParticle;
  }
  int Second() const
  {
    return *localParticle;
  }
  void Next();
  bool Done() const
  {
    return cell == nCells;
  }
private:
  // skip to next nonempty cell
  void NextCell();

// GJS
  CellList::Cell cellParticle;
  CellList::Neighbors localParticle;
// maybe not the reference, or maybe remove the const?
  const CellList& cellList;
// maybe not the reference
  int box, cell, nCells;
// GJS
};

inline CellList::Pairs::Pairs(const CellList& cellList, int box) :
  cellParticle(cellList.EnumerateCell(0, box)),
  localParticle(cellList.EnumerateLocal(0, box)),
  cellList(cellList),
  box(box),
  cell(0),
  nCells(cellList.CellsInBox(box))
{
  if (cellParticle.Done()) NextCell();
  if (First() >= Second() &&
      !(First() == CellList::END_CELL && Second() == CellList::END_CELL))
    Next();
}

inline void CellList::Pairs::NextCell()
{
  do {
    ++cell;
    if (cell >= nCells) return;
    cellParticle = cellList.EnumerateCell(cell, box);
    localParticle = cellList.EnumerateLocal(cell, box);
    // skip empty cells
  } while (cellParticle.Done());
}

inline void CellList::Pairs::Next()
{
  do {
    cellParticle.Next();
    if (cellParticle.Done()) {
      localParticle.Next();
      if (localParticle.Done()) {
        NextCell();
        if (Done()) return;
      } else {
        cellParticle = cellList.EnumerateCell(cell, box);
      }
    }
    // skip over doubles
  } while (First() >= Second());
}
#endif
