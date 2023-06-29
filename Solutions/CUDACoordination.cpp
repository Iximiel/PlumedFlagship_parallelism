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
  // const unsigned nn = pos.size();
  /*std::vector<double> vpos(3 * nn);
  for (unsigned int i = 0; i < nn; ++i) {
    vpos[3 * i] = pos[i][0];
    vpos[3 * i + 1] = pos[i][1];
    vpos[3 * i + 2] = pos[i][2];
  }*/
  double ncoord = getCoordination(pos, R_0);
  setValue(ncoord);
}

} // namespace PLMD
