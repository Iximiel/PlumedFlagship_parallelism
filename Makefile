
.PHONY: all small big

.NOTINTERMEDIATE: OMPCoordination.so SerialCoordination.so MPIOMPCoordination.so

all: small big

big: Serial_big OMP_big

small: Serial_small OMP_small

%_small: %Coordination.so plumed_%.dat
	plumed driver --plumed plumed_$*.dat --ixyz trajectorysmall.xyz > $*.small.out
	@echo "PLUMED:                                               Cycles        Total      Average      Minimum      Maximum"
	@grep "PLUMED: 4 Calculating (forward loop)" $*.small.out

%_big: %Coordination.so plumed_%.dat
	plumed driver --plumed plumed_$*.dat --ixyz gas-one.xyz > $*.big.out
	@echo "PLUMED:                                               Cycles        Total      Average      Minimum      Maximum"
	@grep "PLUMED: 4 Calculating (forward loop)" $*.big.out

%.so:%.cpp
	plumed mklib $<

