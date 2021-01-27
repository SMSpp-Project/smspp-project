##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of test (for LagrangianDualSolver_Box)                          #
#                                                                            #
#                                VERSION 1.00                                #
#                               12 - 01 - 2021                               #
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
NAME = LDS_Box_test

# basic directory
DIR = ./

# debug switches
#SW = -g3 -glldb -fno-inline -std=c++17 -ferror-limit=1
# debug switches with address sanitizer and extra pedantic warning
SW = -g3 -glldb -fno-inline -std=c++17 -ferror-limit=1 -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer -Wpedantic -Wextra -Wno-unused-parameter
# production switches with address sanitizer
#SW = -O3 -std=c++17 -DNDEBUG -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
# production switches
#SW = -O3 -std=c++17 -DNDEBUG

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

# not necessary, BundleSolver does that already
# MILPSolver
#MILPSSDR = ../../MILPSolver/
#include $(MILPSSDR)makefile-c

# BundleSolver
BNDSLVSDR = ../../BundleSolver/
include $(BNDSLVSDR)makefile-c

# LagrangianDualSolver
LgDSLVSDR = ../../LagrangianDualSolver/
include $(LgDSLVSDR)makefile

# not necessary, BundleSolver does that already
# define input macros for SMS++ complete makefile, then include it
#SMS++SDR = ../../../SMS++/
#include $(SMS++SDR)lib/makefile-inc

# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ =  $(BNDSLVOBJ) $(LgDSLVOBJ)

# libreries
MLIB =  $(BNDSLVLIB) $(LgDSLVLIB)

$(NAME): $(MOBJ) $(DIR)test.o
	$(CC) -o $(DIR)$(NAME) $^ $(MLIB) $(SW)

# dependencies: every .o from its .C + every recursively included .h- - - - -

# include directives
MINC =  $(BNDSLVINC) $(LgDSLVINC)

# includes
MH =    $(BNDSLVH) $(LgDSLVH)

# compile command

$(DIR)test.o: $(DIR)test.cpp $(MH)
	$(CC) -c $(DIR)test.cpp -o $@ $(MINC) $(SW)

############################ End of makefile #################################
