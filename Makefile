
.PHONY: all small big clean cleanbk

.NOTINTERMEDIATE: OMPCoordination.so SerialCoordination.so MPICoordination.so plumed_CUDA.dat plumed_OMP.dat plumed_Serial.dat plumed_MPI.dat

all: small big

diff: diff_small diff_big

diff_small: diff_small_OMP diff_small_MPI diff_small_CUDA

diff_big: diff_big_OMP diff_big_MPI diff_big_CUDA

diff_small_%: %_small_colvar Serial_small_colvar
	@diff -y --suppress-common-lines Serial_small_colvar $*_small_colvar && echo "$* small is ok!" || echo "$* small differ!"
	
diff_big_%: %_big_colvar Serial_big_colvar
	@diff -y --suppress-common-lines Serial_big_colvar $*_big_colvar && echo "$* big is ok!" || echo "$* big differ!"


big: Serial_big_colvar OMP_big_colvar MPI_big_colvar CUDA_big

small: Serial_small_colvar OMP_small_colvar MPI_small_colvar CUDA_small

plumed_%.dat:plumed.dat.in
	@sed 's/@type@/$*/g' plumed.dat.in > $@

.ONESHELL:
#some tricks are involved in order to invoke mpirun only for the MPI set up, in order to avoid overhead
%_small_colvar: %Coordination.so plumed_%.dat
	@[[ $* == "MPI" ]]  && prepend="mpirun -n 2 " || prepend=""
	echo "$${prepend}plumed driver --plumed plumed_$*.dat --ixyz trajectorysmall.xyz > $@.out"
	$$prepend plumed driver --plumed plumed_$*.dat --ixyz trajectorysmall.xyz > $@.out
	@mv $*_colvar $@
	@grep "PLUMED: 4 Calculating (forward loop)" $@.out | \
	 awk '{printf "Time passed in calculate: %.3g ms in %i cycles (averaging %.3g ms)\n", $$7*1000, $$6, $$8*1000 }'

%_big_colvar: %Coordination.so plumed_%.dat
	@[[ $* == "MPI" ]]  && prepend="mpirun -n 2 " || prepend=""
	echo "$${prepend}plumed driver --plumed plumed_$*.dat --ixyz gas-one.xyz > $@.out"
	$$prepend plumed driver --plumed plumed_$*.dat --ixyz gas-one.xyz > $@.out
	@mv $*_colvar $@
	@grep "PLUMED: 4 Calculating (forward loop)" $@.out | \
		awk '{printf "Time passed in calculate: %.3g ms in %i cycles (averaging %.3g ms)\n", $$7*1000, $$6, $$8*1000 }'

CUDACoordination.so:CUDACoordinationkernel.cu CUDACoordination.cpp plumed-nvcc-mklib
	./plumed-nvcc-mklib CUDACoordination.cpp CUDACoordinationkernel.cu

%.so:%.cpp
	plumed mklib $<

cleanbk:
	@rm -vf bck.*	

clean: cleanbk
	@rm -vf *.o *.so *.out

veryclean: clean
	@rm -vf *_colvar

plumed-nvcc-mklib:
	cp $${PLUMED_KERNEL%libplumedKernel.so}plumed/plumed-mklib $@
	sed -i 's%source "$$PLUMED_ROOT"/scripts/mklib.sh%source "nvcc-mklib.sh"%' $@ 


