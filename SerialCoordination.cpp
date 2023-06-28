#include "plumed/colvar/Colvar.h"
#include "plumed/core/ActionRegister.h"
#include "plumed/tools/Communicator.h"
#include "plumed/tools/NeighborList.h"
#include "plumed/tools/OpenMP.h"
#include <memory>

namespace PLMD {

class MyCoordination : public colvar::Colvar {
  std::unique_ptr<NeighborList> nl;
  double R_0;
  bool pbc{true};
  bool serial{false};
  bool invalidateList{true};
  bool firsttime{true};

public:
  explicit MyCoordination(const ActionOptions &);
  ~MyCoordination();
  // active methods:
  void calculate() override;
  void prepare() override;
  static void registerKeywords(Keywords &keys);
};

PLUMED_REGISTER_ACTION(MyCoordination, "MYCOORDINATION")

void MyCoordination::registerKeywords(Keywords &keys) {
  Colvar::registerKeywords(keys);
  keys.addFlag("SERIAL", false,
               "Perform the calculation in serial - for debug purpose");
  keys.addFlag("PAIR", false,
               "Pair only 1st element of the 1st group with 1st element in the "
               "second, etc");
  keys.add("compulsory", "R_0",
           "The distance under which calculate neighbours");
  keys.addFlag("NLIST", false,
               "Use a neighbor list to speed up the calculation");
  keys.add("optional", "NL_CUTOFF", "The cutoff for the neighbor list");
  keys.add("optional", "NL_STRIDE",
           "The frequency with which we are updating the atoms in the neighbor "
           "list");
  keys.add("atoms", "GROUPA", "First list of atoms");
  keys.add(
      "atoms", "GROUPB",
      "Second list of atoms (if empty, N*(N-1)/2 pairs in GROUPA are counted)");
}

MyCoordination::MyCoordination(const ActionOptions &ao)
    : PLUMED_COLVAR_INIT(ao) {

  parseFlag("SERIAL", serial);

  std::vector<AtomNumber> ga_lista, gb_lista;
  parseAtomList("GROUPA", ga_lista);
  parseAtomList("GROUPB", gb_lista);

  bool nopbc = !pbc;
  parseFlag("NOPBC", nopbc);
  pbc = !nopbc;

  // pair stuff
  bool dopair = false;
  parseFlag("PAIR", dopair);
  parse("R_0", R_0);

  // neighbor list stuff
  bool doneigh = false;
  double nl_cut = 0.0;
  int nl_st = 0;
  parseFlag("NLIST", doneigh);
  if (doneigh) {
    parse("NL_CUTOFF", nl_cut);
    if (nl_cut <= 0.0)
      error("NL_CUTOFF should be explicitly specified and positive");
    parse("NL_STRIDE", nl_st);
    if (nl_st <= 0)
      error("NL_STRIDE should be explicitly specified and positive");
  }

  addValue();
  setNotPeriodic();
  if (gb_lista.size() > 0) {
    if (doneigh)
      nl = Tools::make_unique<NeighborList>(ga_lista, gb_lista, serial, dopair,
                                            pbc, getPbc(), comm, nl_cut, nl_st);
    else
      nl = Tools::make_unique<NeighborList>(ga_lista, gb_lista, serial, dopair,
                                            pbc, getPbc(), comm);
  } else {
    if (doneigh)
      nl = Tools::make_unique<NeighborList>(ga_lista, serial, pbc, getPbc(),
                                            comm, nl_cut, nl_st);
    else
      nl = Tools::make_unique<NeighborList>(ga_lista, serial, pbc, getPbc(),
                                            comm);
  }

  requestAtoms(nl->getFullAtomList());

  log.printf("  between two groups of %u and %u atoms\n",
             static_cast<unsigned>(ga_lista.size()),
             static_cast<unsigned>(gb_lista.size()));
  log.printf("  first group:\n");
  for (unsigned int i = 0; i < ga_lista.size(); ++i) {
    if ((i + 1) % 25 == 0)
      log.printf("  \n");
    log.printf("  %d", ga_lista[i].serial());
  }
  log.printf("  \n  second group:\n");
  for (unsigned int i = 0; i < gb_lista.size(); ++i) {
    if ((i + 1) % 25 == 0)
      log.printf("  \n");
    log.printf("  %d", gb_lista[i].serial());
  }
  log.printf("  \n");
  if (pbc)
    log.printf("  using periodic boundary conditions\n");
  else
    log.printf("  without periodic boundary conditions\n");
  if (dopair)
    log.printf("  with PAIR option\n");
  if (doneigh) {
    log.printf("  using neighbor lists with\n");
    log.printf("  update every %d steps and cutoff %f\n", nl_st, nl_cut);
  }
}

MyCoordination::~MyCoordination() {
  // destructor required to delete forward declared class
}

void MyCoordination::prepare() {
  if (nl->getStride() > 0) {
    if (firsttime || (getStep() % nl->getStride() == 0)) {
      requestAtoms(nl->getFullAtomList());
      invalidateList = true;
      firsttime = false;
    } else {
      requestAtoms(nl->getReducedAtomList());
      invalidateList = false;
      if (getExchangeStep())
        error("Neighbor lists should be updated on exchange steps - choose a "
              "NL_STRIDE which divides the exchange stride!");
    }
    if (getExchangeStep())
      firsttime = true;
  }
}

// calculator
void MyCoordination::calculate() {

  double ncoord = 0.;

  if (nl->getStride() > 0 && invalidateList) {
    nl->update(getPositions());
  }

  const unsigned nn = nl->size();

  for (unsigned int i = 0; i < nn; ++i) {

    Vector distance;
    unsigned i0 = nl->getClosePair(i).first;
    unsigned i1 = nl->getClosePair(i).second;

    if (getAbsoluteIndex(i0) == getAbsoluteIndex(i1))
      continue;

    if (pbc) {
      distance = pbcDistance(getPosition(i0), getPosition(i1));
    } else {
      distance = delta(getPosition(i0), getPosition(i1));
    }
    ncoord += distance.modulo2() < R_0;
  }

  setValue(ncoord);
}

} // namespace PLMD
