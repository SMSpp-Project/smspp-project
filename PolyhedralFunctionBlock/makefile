##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of test (for PolthedralFunctionBlock)                           #
#                                                                            #
#                                VERSION 1.10                                #
#                               02 - 12 - 2019                               #
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
NAME = PolyhedralFunctionBlock_test

# basic directory
DIR = ./

# debug switches
#SW = -g3 -glldb -fno-inline -std=c++17 -ferror-limit=1 -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
# production switches with checks
#SW = -O3 -std=c++17 -DNDEBUG -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
# production switches with checks
SW = -O3 -std=c++17 -DNDEBUG

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

# BundleSolver
BNDSLVSDR = ../../BundleSolver/
BNDSLVODR = ../../BundleSolver/
include $(BNDSLVSDR)makefile-c

# not necessary, BundleSolver does that already
# define input macros for SMS++ complete makefile, then include it
#SMS++SDR = ../../../SMS++/
#include $(SMS++SDR)lib/makefile-inc

# not necessary, BundleSolver does that already
# define input macros for libCPLEX complete makefile, then include it
#libCPLEXDIR = ../../../extlib/
#include $(libCPLEXDIR)makefile-libCPLEX

# not necessary, BundleSolver does that already
# MILPSolver
#MILPSSDR = ../../../MILPSolver/
#include $(MILPSSDR)makefile

# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ =  $(BNDSLVOBJ)

# libreries
MLIB =  $(BNDSLVLIB)

$(NAME): $(MOBJ) $(DIR)test.o
	$(CC) -o $(DIR)$(NAME) $(DIR)test.o $(MOBJ) $(MLIB) $(SW)

# dependencies: every .o from its .C + every recursively included .h- - - - -

# include directives
MINC =  $(BNDSLVINC)

# includes
MH =    $(BNDSLVH)

# compile command

$(DIR)test.o: $(DIR)test.cpp $(MH)
	$(CC) -c $(DIR)test.cpp -o $@ $(MINC) $(SW)

############################ End of makefile #################################
