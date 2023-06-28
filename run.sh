#!/bin/bash

module purge
module load gnu9 openmpi4 plumed/MPI

time plumed driver --plumed plumed_serial.dat --igro trajectory.gro
time plumed driver --plumed plumed_omp.dat --igro trajectory.gro
