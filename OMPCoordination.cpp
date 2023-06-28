#include "plumed/colvar/Colvar.h"
#include "plumed/core/ActionRegister.h"
#include "plumed/tools/Communicator.h"
#include "plumed/tools/NeighborList.h"
#include "plumed/tools/OpenMP.h"
#include <iostream>
#include <memory>

namespace PLMD {

class MyCoordination : public colvar::Colvar {
  double R_0;
  bool pbc{true};

public:
  explicit MyCoordination(const ActionOptions &);
  ~MyCoordination() = default;
  // active methods:
  void calculate() override;
  static void registerKeywords(Keywords &keys);
};

PLUMED_REGISTER_ACTION(MyCoordination, "MYCOORDINATION")

void MyCoordination::registerKeywords(Keywords &keys) {
  Colvar::registerKeywords(keys);
  keys.add("compulsory", "R_0",
           "The distance under which calculate neighbours");
  keys.add("atoms", "ATOMS", "First list of atoms");
}

MyCoordination::MyCoordination(const ActionOptions &ao)
    : PLUMED_COLVAR_INIT(ao) {

  std::vector<AtomNumber> ga_lista;
  parseAtomList("ATOMS", ga_lista);

  bool nopbc = !pbc;
  parseFlag("NOPBC", nopbc);
  pbc = !nopbc;

  parse("R_0", R_0);

  addValue();
  setNotPeriodic();
  requestAtoms(ga_lista);

  if (pbc)
    log.printf("  using periodic boundary conditions\n");
  else
    log.printf("  without periodic boundary conditions\n");
}

// calculator
void MyCoordination::calculate() {
  double ncoord = 0.;
  auto pos = getPositions();
  const unsigned nn = pos.size();
  unsigned nt = OpenMP::getNumThreads();
#pragma omp parallel for reduction(+ : ncoord) num_threads(nt)
  for (unsigned int i0 = 0; i0 < nn; ++i0) {
    for (unsigned int i1 = 0; i1 < nn; ++i1) {
      Vector distance;

      if (i0 == i1)
        continue;

      if (pbc) {
        distance = pbcDistance(getPosition(i0), getPosition(i1));
      } else {
        distance = delta(getPosition(i0), getPosition(i1));
      }
      ncoord += (distance.modulo() < R_0) ? 1 : 0;
    }
  }
  setValue(ncoord);
}

} // namespace PLMD
