#include "plumed/colvar/Colvar.h"
#include "plumed/core/ActionRegister.h"
#include "plumed/tools/Communicator.h"
#include "plumed/tools/NeighborList.h"
#include "plumed/tools/OpenMP.h"
#include <iostream>
#include <memory>

namespace PLMD {

class MyCoordinationBase : public colvar::Colvar {
protected:
  double R_0;
  bool pbc{true};

public:
  explicit MyCoordinationBase(const ActionOptions &);
  ~MyCoordinationBase() = default;
  // active methods:
  static void registerKeywords(Keywords &keys);
};

void MyCoordinationBase::registerKeywords(Keywords &keys) {
  Colvar::registerKeywords(keys);
  keys.add("compulsory", "R_0",
           "The distance under which calculate neighbours");
  keys.add("atoms", "ATOMS", "First list of atoms");
}

MyCoordinationBase::MyCoordinationBase(const ActionOptions &ao)
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
} // namespace PLMD
