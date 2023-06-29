
.PHONY: all small big clean cleanbk

.NOTINTERMEDIATE: OMPCoordination.so SerialCoordination.so MPICoordination.so

all: small big

big: Serial_big OMP_big MPI_big

small: Serial_small OMP_small MPI_small

plumed_%.dat:plumed.dat.in
	sed 's/@type@/$*/g' plumed.dat.in > $@

.ONESHELL:
#some tricks are involved in order to invoke mpirun only for the MPI set up, in order to avoid overhead
%_small: %Coordination.so plumed_%.dat
	@[[ $* == "MPI" ]]  && prepend="mpirun -n 2 " || prepend=""
	echo "$${prepend}plumed driver --plumed plumed_$*.dat --ixyz trajectorysmall.xyz > $@.out"
	$$prepend plumed driver --plumed plumed_$*.dat --ixyz trajectorysmall.xyz > $@.out
	@mv $*_colvar $@_colvar
	@grep "PLUMED: 4 Calculating (forward loop)" $@.out | \
	 awk '{printf "Time passed in calculate: %.3g ms in %i cycles (averaging %.3g ms)\n", $$7*1000, $$6, $$8*1000 }'

%_big: %Coordination.so plumed_%.dat
	@[[ $* == "MPI" ]]  && prepend="mpirun -n 2 " || prepend=""
	echo "$${prepend}plumed driver --plumed plumed_$*.dat --ixyz gas-one.xyz > $@.out"
	$$prepend plumed driver --plumed plumed_$*.dat --ixyz gas-one.xyz > $@.out
	@mv $*_colvar $@_colvar
	@grep "PLUMED: 4 Calculating (forward loop)" $@.out | \
		awk '{printf "Time passed in calculate: %.3g ms in %i cycles (averaging %.3g ms)\n", $$7*1000, $$6, $$8*1000 }'

%.so:%.cpp
	plumed mklib $<

cleanbk:
	@rm -vf bck.*	

clean: cleanbk
	@rm -vf *.o *.so *.out
