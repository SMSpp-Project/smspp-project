##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of test (for LagrangianDualSolver with MMCFBlock)               #
#                                                                            #
#                                VERSION 1.00                                #
#                               30 - 01 - 2021                               #
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
NAME = LDS_MMCF_test

# basic directory
DIR = ./

# debug switches
#SW = -g3 -glldb -fno-inline -std=c++17 -ferror-limit=1
# debug switches with address sanitizer and extra pedantic warning
#SW = -g3 -glldb -fno-inline -std=c++17 -ferror-limit=1 -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer -Wpedantic -Wextra -Wno-unused-parameter
# production switches with address sanitizer
#SW = -O3 -std=c++17 -DNDEBUG -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
# production switches
SW = -O3 -std=c++17 -DNDEBUG -DHAVE_CPLEX -DHAVE_MFSMX -DCLANG_1200_0_32_27_PATCH

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

# MILPSolver
MILPSSDR = ../../MILPSolver/
include $(MILPSSDR)makefile-c

# BundleSolver
BNDSLVSDR = ../../BundleSolver/
include $(BNDSLVSDR)makefile-c

# define input macros for libNDO library makefile, then include it
libNDOSDR = $(BNDSLVSDR)NdoFiOracle/
include $(libNDOSDR)lib/makefile-inc

# LagrangianDualSolver
LgDSLVSDR = ../../LagrangianDualSolver/
include $(LgDSLVSDR)makefile

# MMCFBlock
MMCFSDR = ../../MMCFBlock/
include $(MMCFSDR)makefile-c

# not necessary, MMCFBlock does that already
# define input macros for SMS++ complete makefile, then include it
#SMS++SDR = ../../../SMS++/
#include $(SMS++SDR)lib/makefile-inc

# define input macros for UCBlock makefile, then include it
# note that this is not the "complete" makefile: UCBlock only requires
# the core SMS++, that is already included above (by LagrangianDualSolver)
UCBckDIR = ../../UCBlock/
include $(UCBckDIR)lib/makefile

# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ =  $(MILPSOBJ) $(libNDOOBJ) $(BNDSLVOBJ) $(LgDSLVOBJ) $(MMCFOBJ)

# libreries
MLIB =  $(MILPSLIB) $(libNDOLIB) $(BNDSLVLIB) $(LgDSLVLIB) $(MMCFLIB)

$(NAME): $(MOBJ) $(DIR)test.o
	$(CC) -o $(DIR)$(NAME) $^ $(MLIB) $(SW)

# dependencies: every .o from its .C + every recursively included .h- - - - -

# include directives
MINC =  $(MILPSINC) $(libNDOINC) $(BNDSLVINC) $(LgDSLVINC) $(MMCFINC)

# includes
MH =    $(MILPSH) $(libNDOH) $(BNDSLVH) $(LgDSLVH) $(MMCFH)

# compile command

$(DIR)test.o: $(DIR)test.cpp $(MH)
	$(CC) -c $(DIR)test.cpp -o $@ $(MINC) $(SW)

############################ End of makefile #################################
