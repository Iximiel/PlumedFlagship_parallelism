#include "Base.hpp"
namespace PLMD {
class MyCoordination : public MyCoordinationBase {
public:
  explicit MyCoordination(const ActionOptions &ao)
      : Action(ao), MyCoordinationBase(ao) {}
  ~MyCoordination() = default;
  // active methods:
  void calculate() override;
};
PLUMED_REGISTER_ACTION(MyCoordination, "MYCOORDINATION")
// calculator
void MyCoordination::calculate() {
  double ncoord = 0.;
  auto pos = getPositions();
  const unsigned nn = pos.size();
  unsigned nt = OpenMP::getNumThreads();

  Vector distance;

#pragma omp parallel num_threads(nt)
  {
#pragma omp for
    for (unsigned int i0 = 0; i0 < nn; ++i0) {
      for (unsigned int i1 = 0; i1 < nn; ++i1) {

        if (i0 == i1) {
          continue;
        }

        distance = delta(getPosition(i0), getPosition(i1));

        ncoord += (distance.modulo() < R_0) ? 1 : 0;
      }
    }
  }
  setValue(ncoord);
}

} // namespace PLMD
