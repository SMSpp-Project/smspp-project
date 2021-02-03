##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of test (for MCFMILP)                                           #
#                                                                            #
#                                VERSION 1.00                                #
#                               16 - 06 - 2020                               #
#                                                                            #
#                              Implementation by:                            #
#                                                                            #
#                              Antonio Frangioni                             #
#                           Operations Research Group                        #
#                          Dipartimento di Informatica                       #
#                              Universita' di Pisa                           #
#                                                                            #
##############################################################################

# module name
NAME = MCF_MILP_test

# basic directory
DIR = ./

# switch to select the :MCFClass solver, supported values are
# -DHAVE_CSCL2      for the CS2 class
# -DHAVE_CPLEX      for the MCFCplex class
# -DHAVE_MFSMX      for the MCFSimplex class
# -DHAVE_MFZIB      for the MCFZIB class
# -DHAVE_RELAX      for the RelaxIV class
# -DHAVE_CPLEX      for the MCFCplex class
# -DHAVE_SPTRE      for the SPTree class; note that SPTree cannot solve
#                   most MCF instances, except those with SPT structure
MCFCS = -DHAVE_MFSMX

# debug switches
SW = -g3 -glldb -fno-inline -std=c++17 -ferror-limit=1 $(MCFCS)
# production switches
#SW = -O3 -std=c++17 -DNDEBUG $(MCFCS)

# compiler
CC = clang++

# default target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

default: $(NAME)

# clean - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

clean::
	rm -f $(DIR)*.o $(DIR)*~ $(NAME)

# define & include the necessary modules- - - - - - - - - - - - - - - - - - -
# if a module is not used in the current configuration, just comment out the
# corresponding include line
# each module outputs some macros to be used here:
# *OBJ is the final object(s) / library
# *LIB is the external libreries + -L<libdirs>
# *H   is the list of all include files
# *INC is the -I<include directories>

# not necessary, MCFBlock does that already
# define input macros for SMS++ complete makefile, then include it
#SMS++SDR = ../..SMS++/
#include $(SMS++SDR)lib/makefile-inc

# MCFBlock
MCFBkSDR = ../../MCFBlock/
include $(MCFBkSDR)makefile-c

# not necessary, MILPSolver does that already
# define input macros for libCPLEX complete makefile, then include it
#libCPLEXDIR = ../../extlib/
#include $(libCPLEXDIR)makefile-libCPLEX

# not necessary, MILPSolver does that already
# define input macros for libSCIP complete makefile, then include it
#libSCIPDIR = ../../extlib/
#include $(libSCIPDIR)makefile-libSCIP

# MILPSolver
MILPSSDR = ../../MILPSolver/
include $(MILPSSDR)makefile-c

# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ =  $(MCFBkOBJ) $(MILPSOBJ)

# libreries
MLIB =  $(MCFBkLIB) $(MILPSLIB) -lgtest

$(NAME): $(MOBJ) $(DIR)test.o
	$(CC) -o $(DIR)$(NAME) $(DIR)test.o $(MOBJ) $(MLIB) $(SW)

# dependencies: every .o from its .C + every recursively included .h- - - - -

# include directives
MINC =  $(MCFBkINC) $(MILPSINC)

# includes
MH =    $(MCFBkH) $(MILPSH)

# compile command

$(DIR)test.o: $(DIR)test.cpp $(MH)
	$(CC) -c $(DIR)test.cpp -o $@ $(MINC) $(SW)

############################ End of makefile #################################
