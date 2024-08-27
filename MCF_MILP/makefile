##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of test (for MCF_MILP)                                          #
#                                                                            #
#                              Antonio Frangioni                             #
#                          Dipartimento di Informatica                       #
#                              Universita' di Pisa                           #
#                                                                            #
##############################################################################

# module name
NAME = MCF_MILP_test

# basic directory
DIR = .

# debug switches
#SW = -g3 -glldb -fno-inline -std=c++20 -DCLANG_1200_0_32_27_PATCH -ferror-limit=1 -Wno-enum-compare -Wno-deprecated-enum-enum-conversion -Wno-deprecated-enum-float-conversion -Wno-deprecated-declarations
# debug switches with address sanitizer and extra pedantic warning
#SW = -g3 -glldb -fno-inline -std=c++20 -DCLANG_1200_0_32_27_PATCH -ferror-limit=1 -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer -Wpedantic -Wextra -Wno-unused-parameter -Wno-enum-compare -Wno-deprecated-enum-enum-conversion -Wno-deprecated-enum-float-conversion -Wno-deprecated-declarations -Wno-deprecated-copy-with-user-provided-copy
# production switches
SW = -O3 -std=c++20 -DNDEBUG -DCLANG_1200_0_32_27_PATCH -Wno-enum-compare -Wno-deprecated-enum-enum-conversion -Wno-deprecated-enum-float-conversion -Wno-deprecated-declarations -ferror-limit=1

# compiler
CC = g++

# default target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

default: $(DIR)/$(NAME)

# clean - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

clean::
	rm -f $(DIR)/*.o $(DIR)/*~ $(DIR)/$(NAME)

# define & include the necessary modules- - - - - - - - - - - - - - - - - - -
# if a module is not used in the current configuration, just comment out the
# corresponding include line
# each module outputs some macros to be used here:
# *OBJ is the final object(s) / library
# *LIB is the external libraries + -L< libdirs >
# *H   is the list of all include files
# *INC is the -I< include directories >

# not necessary, MCFBlock does that already
# define input macros for SMS++ complete makefile, then include it
#SMS++SDR = ../..SMS++
#include $(SMS++SDR)/lib/makefile-inc

# MCFBlock
MCFBkSDR = ../../mcfblock
include $(MCFBkSDR)/makefile-c

# LEMONSolver, assuming all dependencies (MCFBlock) are there already
# (comment out if not required)
MCFLESDR = ../../LEMONSolver
include $(MCFLESDR)/makefile

# MILPSolver
<<<<<<< Updated upstream
MILPSSDR = ../../MILPSolver
include $(MILPSSDR)/makefile-s
=======
MILPSSDR = ../../milpsolver
include $(MILPSSDR)/makefile-c
>>>>>>> Stashed changes

# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ =  $(MCFLEOBJ) $(MILPSOBJ) $(MCFBkOBJ)

# libraries
MLIB =  $(MCFBkLIB) $(MCFLELIB) $(MILPSLIB)

$(DIR)/$(NAME): $(MOBJ) $(DIR)/test.o
	$(CC) -o $(DIR)/$(NAME) $(DIR)/test.o $(MOBJ) $(MLIB) $(SW)
#	-Wl,-ld_classic
# emergency linker option for certain settings on macOS

# dependencies: every .o from its .C + every recursively included .h- - - - -

# include directives
MINC =  $(MCFBkINC) $(MCFLEINC) $(MILPSINC)

# includes
MH =    $(MCFBkH) $(MCFLEH) $(MILPSH)

# compile command

$(DIR)/test.o: $(DIR)/test.cpp $(MH)
	$(CC) -c $(DIR)/test.cpp -o $@ $(MINC) $(SW)

############################ End of makefile #################################
