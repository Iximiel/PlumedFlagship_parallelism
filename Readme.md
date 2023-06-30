# Plumed Flagship meeting: Lecture: Parallel and GPUs programming in PLUMED

We have some simple examples for using parallelism in plumed:

- `SerialCoordination.cpp` is a simplified version of the cv `Coordination`
- `OMPCoordination.cpp` is an example on how openMP can be used in Plumed
- `MPICoordination.cpp` is an example on how MPI can be used in Plumed
- `CUDACoordination.cpp` is an example on trying to user Cuda for solving this problem (it needs an ad hoc compiler queue and the kernel `CUDACoordinationkernel.cu` )

(The original `Coordination` combines the use of both openMP and MPI)

Plumed helps the developer with some tools for parallelism:
- `tools/OpenMP.h` contains some function that are useful in working with openMP
- `tools/Communicator.h` is more or less always present as the variable `comm` that is inherited through `PLMD::Action`
