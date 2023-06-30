# Plumed Flagship meeting: Lecture: Parallel and GPUs programming in PLUMED

We have some simple examples for using parallelism in plumed:

- `SerialCoordination.cpp` is a simplified version of the cv `Coordination`
- `OMPCoordination.cpp` is an example on how openMP can be used in Plumed
- `MPICoordination.cpp` is an example on how MPI can be used in Plumed
- `CUDACoordination.cpp` is an example on trying to user Cuda for solving this problem (it needs an ad hoc compiler queue and the kernel `CUDACoordinationkernel.cu` )

(The original `Coordination` combines the use of both openMP and MPI)

Plumed helps the developer with some tools for parallelism:
- `tools/OpenMP.h` contains some function that are useful in working with openMP.
In the example we are using `OpenMP::getNumThreads()` to get the number of threads from the environmental variable `PLUMED_NUM_THREADS`

- `tools/Communicator.h` is present as the variable `comm` that is inherited through `PLMD::Action`.
`PLMD::Communicator` is an interface to some of the functionalities of the C API of mpi.h
In the example we are using `PLMD::Communicator::Get_size()` to get the number of the processes spawned by mpirun, 
`PLMD::Communicator::Get_rank()` to get the id of the process, and 
`PLMD::Communicator::Sum()` to sum the result of the coordination and make the correct value avayable for further calculations.

## Intro

The exercise uses `Base.hpp` to give a very base version of the COORDINATION CV, in this case it is returning the sum of the number of atoms that are within R_0 from each atom. For simplicity it the pbcs will be ignored throught all of the examples.
`MyCoordinationBase` in `Base.hpp` do not have the calculate method and so the example will have more or less the following structure:

```C++
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
void MyCoordination::calculate() {
...
}
} // namespace PLMD

```
For automation purposes (see the `Makefile`) we are using the same key `PLUMED_REGISTER_ACTION(MyCoordination, "MYCOORDINATION")` for all of the examples.
### Prerequisites
- Plumed 2.9.0 configured and installed with `--enable-modules=all` and MPI
  - in my workstation I am using gcc 9.4.0 and openmpi 4.1.1
- For the GPU offloading the example is written with Nvdia's Cuda
  - In my workstation I am using cuda 11.7 with a T1000 card


## The serial code

```C++
void MyCoordination::calculate() {
  double ncoord = 0.;
  auto pos = getPositions();
  const unsigned nn = pos.size();
  Vector distance;
  for (unsigned int i0 = 0; i0 < nn; ++i0) {
    for (unsigned int i1 = 0; i1 < nn; ++i1) {
      if (i0 == i1) {continue;}
      distance = delta(getPosition(i0), getPosition(i1));
      ncoord += (distance.modulo() < R_0) ? 1 : 0;
    }
  }
  setValue(ncoord);
}
```
the workflow is:
- get the positions
- initialize `ncoord` to zero
- do a double loop calculating the distance between each possible couple of atoms
and increment `ncoord` each time the distance is less than `R_0`
(avoid calculating the distance from self)

## Threading: openMP
```C++
void MyCoordination::calculate() {
  double ncoord = 0.;
  auto pos = getPositions();
  const unsigned nn = pos.size();
  unsigned nt = OpenMP::getNumThreads();
  Vector distance;
#pragma omp parallel num_threads(nt)
  {//start parallel scope
#pragma omp for 
    for (unsigned int i0 = 0; i0 < nn; ++i0) {
      for (unsigned int i1 = 0; i1 < nn; ++i1) {
        if (i0 == i1) {continue;}
        distance = delta(getPosition(i0), getPosition(i1));
        ncoord += (distance.modulo() < R_0) ? 1 : 0;
      }
    }
  }//end parallel scope
  setValue(ncoord);
}
```

This is nearly identical to the serial version. With few differences:
We are asking the system if we want to run with more threads with `unsigned nt = OpenMP::getNumThreads()`, (if is the first time that it is called stores and return the number in the environmental variable `PLUMED_NUM_THREADS`, otherwise it will return the stored number).

Then with that information we open a parallel environment with `#pragma omp parallel num_threads(nt)` in which we explicilty specify the number of threads (otherwise il will use the number in the environmntal variable `OMP_NUM_THREADS`).
Note that the pragma is prepended to a scope delimited by curly braces: the code there will be distribuited between threads.

Then we have the next pragma instruction: ``#pragma omp for`: this pragma will tranform the for in order to make it work in the multithreaded environment. We are splitting the for cycle in each thread

<details>
  <summary>SPOILER</summary>
  There is at least a race condition here: can you spot it?

<details>
    <summary>ANSWER</summary>

`ncoord` is the race condition: each time two threads or more threads execute the `+=` simultaneusly they will increment the `ncoord` from the same value, meaning the the number saved in memory has been incremented only once instead of twice or more.

The solution is to append `reduction(+:)` to for pragma: `#pragma for reduction(+:)`
  </details>
</details>

## Processes: MPI
```C++
void MyCoordination::calculate() {
  double ncoord = 0.;
  auto pos = getPositions();
  const unsigned nn = pos.size();
  unsigned stride = comm.Get_size();
  unsigned rank = comm.Get_rank();
  Vector distance;
  for (unsigned int i0 = rank; i0 < nn; i0 += stride) {
    for (unsigned int i1 = 0; i1 < nn; ++i1) {
      if (i0 == i1){continue;}
      distance = delta(getPosition(i0), getPosition(i1));
      ncoord += (distance.modulo() < R_0) ? 1 : 0;
    }
  }
  if (stride > 1) {
    comm.Sum(ncoord);
  }
  setValue(ncoord);
}
```

In this case the parallelization strategy passes through parallel processes:
with `unsigned rank = comm.Get_rank();` we inform the function wich is the this process, with `unsigned stride = comm.Get_size();` we want to know the number of processes that partecipate to the calculation.

The strategy here is to split the outer for cycle between the processes, so that each process will do only part of the loop. We are doing by hand what the `#pragma for` does behind the curtains in the threaded example.
so that the original cycle `for (unsigned int i0 = 0; i0 < nn; ++i0)` became `for (unsigned int i0 = rank; i0 < nn; i0 += stride)`: `stride` ensures that the calculation won't overlap.

We have less risk of having a data race because all the variable are instantiated in each different processor, meaning that non only the for loop is executed in parallel but the whole calculation!

We have then the problem of obtaining the correct result: we have to set `ncoord` to the sum of all the `ncoord` in the various processes.
To do the sum we use the Plumed mpi inteface: `comm.Sum(ncoord);` this will call the reduction sum algorithm of the MPI library with no complications.

## EXTRA: GPU offloading with Cuda
To get the cuda compiler compilated I set up an ad hoc alternative to `plumed mklib`, that uses nvcc to compile the kernel and to link the shared object.

```C++
void MyCoordination::calculate() {
  auto pos = getPositions();
  double ncoord = getCoordination(pos, R_0);
  setValue(ncoord);
}
```
The C++ insterface is simple, it calls `getCoordination(pos, R_0);`, that is an interface to the kernel:
```C++
double getCoordination(std::vector<PLMD::Vector> positions, double R_0) {
  auto nat = positions.size();
  double *d_data;
  double *ncoords;
  cudaMalloc(&d_data, 3 * nat * sizeof(double));
  cudaMalloc(&ncoords, nat * sizeof(double));
  for (size_t i = 0; i < nat; i++) {
    cudaMemcpy(d_data + i * 3, &positions[i][0], 3 * sizeof(double),
               cudaMemcpyHostToDevice);
  }

  double Rsqr = R_0 * R_0;
  cudakernel<<<1, nat>>>(d_data, ncoords, nat, Rsqr);
  reduction<<<1, nat / 2>>>(ncoords);
  double result;
  cudaMemcpy(&result, ncoords, sizeof(double), cudaMemcpyDeviceToHost);

  cudaFree(d_data);
  cudaFree(ncoords);
  return result;
}
```
there the data is initialized on the GPU with a combination of `cudaMalloc` and `cudaMemcpy` and then the two kernels are called with the `<<<Nblocks,threadperblock>>>` syntax.

Citing the Nvidia block:
>A group of threads is called a CUDA block. CUDA blocks are grouped into a grid. A kernel is executed as a grid of blocks of threads

Each thread execute the instructions in the kernel and shares informations with the other threads in the block

in this case we have a kernel for calculation the ncoord of each atom:
```C
__global__ void getCoord(double *coordinates, double *trt, unsigned Nat,
                         double Rsqr) {
  const int i = threadIdx.x + blockIdx.x;
  double x = coordinates[3 * i];
  double y = coordinates[3 * i + 1];
  double z = coordinates[3 * i + 2];
  trt[i] = 0.0;
  double dx, dy, dz;
  for (unsigned j = 0; j < Nat; j++) {
    if (i == j) {
      continue;
    }
    dx = x - coordinates[3 * j];
    dy = y - coordinates[3 * j + 1];
    dz = z - coordinates[3 * j + 2];
    if ((dx * dx + dy * dy + dz * dz) < Rsqr) {
      trt[i] += 1.0;
    }
  }
}
```