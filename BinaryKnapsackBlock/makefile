##############################################################################
################################ makefile ####################################
##############################################################################
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
NAME = test

# basic directory
DIR = ./

# debug switches
SW = -g -glldb -fno-inline -std=c++17 -ferror-limit=1 -fsanitize=address 
# production switches
#SW = -O3 -std=c++17 -fsanitize=address -DNDEBUG

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

# BKBlock
BKBkSDR = ../../BinaryKnapsackBlock/
include $(BKBkSDR)makefile-c

# define input macros for MILPSolver complete makefile, then include it
MILPSSDR = $(BKBkSDR)../MILPSolver/
include $(MILPSSDR)makefile-c

# define input macros for libCPLEX complete makefile, then include it
#libCPLEXDIR = ../../extlib/
#include $(libCPLEXDIR)makefile-libCPLEX

# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ =  $(BKBkOBJ) $(MILPSOBJ)

# libreries
MLIB =  $(BKBkLIB) $(MILPSLIB) $(libCPLEXLIB)

$(NAME): $(MOBJ) $(DIR)$(NAME).o
	$(CC) -o $(NAME) $(DIR)$(NAME).o $(MOBJ) $(MLIB) $(SW)

# dependencies: every .o from its .C + every recursively included .h- - - - -

# include directives
MINC =  $(BKBkINC) $(MILPSINC) $(libCPLEXINC)

# includes
MH =    $(BKBkH)

# compile command

$(DIR)$(NAME).o: $(DIR)$(NAME).cpp $(MH)
	$(CC) -c $*.cpp -o $@ $(MINC) $(SW)

############################ End of makefile #################################
