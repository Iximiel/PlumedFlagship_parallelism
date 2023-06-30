#include "Base.hpp"
#include "CUDACoordinationkernel.hpp"

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

  auto pos = getPositions();
  double ncoord = getCoordination(pos, R_0);
  setValue(ncoord);
}

} // namespace PLMD
