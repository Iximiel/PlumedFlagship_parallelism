#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "CUDACoordinationkernel.hpp"
#include <iostream>

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

__global__ void reduction(double *input) {
  const int tid = threadIdx.x;

  auto step_size = 1;
  int number_of_threads = blockDim.x;

  while (number_of_threads > 0) {
    if (tid < number_of_threads) // still alive?
    {
      const auto fst = tid * step_size * 2;
      const auto snd = fst + step_size;
      input[fst] += input[snd];
    }

    step_size <<= 1;
    number_of_threads >>= 1;
  }
}

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
  getCoord<<<nat, 1>>>(d_data, ncoords, nat, Rsqr);
  reduction<<<1, nat / 2>>>(ncoords);
  double result;
  cudaMemcpy(&result, ncoords, sizeof(double), cudaMemcpyDeviceToHost);

  cudaFree(d_data);
  cudaFree(ncoords);
  return result;
}
