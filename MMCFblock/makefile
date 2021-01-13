##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of test (for core SMS++)                                        #
#                                                                            #
#                                VERSION 2.00                                #
#                               12 - 11 - 2018                               #
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
NAME = MMCFtest

# basic directory
DIR = ./

# debug switches
SW = -g3 -fno-inline -std=c++17 -ferror-limit=1

# production switches
#SW = -O3 -DNDEBUG -std=c++17  

# compiler
#CC = /usr/local/Cellar/gcc/8.3.0_2/bin/g++-8 
CC = clang++ 

# default target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

all default: $(NAME)

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

# MMCF
libMMCFSDR = $(DIR)/MMCFClass/
include $(libMMCFSDR)lib/makefile-inc

# OPT*** stuff
OPTUxH   = $(libMMCFSDR)OPTUtils/OPTUtils.h $(libMMCFSDR)OPTUtils/OPTtypes.h \
	 $(libMMCFSDR)OPTUtils/OPTvect.h
OPTUxINC = -I$(libMMCFSDR)OPTUtils


# core SMS++
#SMS++SDR = ../../SMS++/
#include $(SMS++SDR)lib/makefile-inc

# MILPSolver
MILPSSDR = ../../MILPSolver/
include $(MILPSSDR)makefile-c

# MMCFBlock
MMCFSDR = ../../mmcfblock/
include $(MMCFSDR)makefile-c

# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ = $(MMCFOBJ)\
       $(SMS++OBJ)\
       $(MILPSOBJ)\
       $(libMMCFOBJ)

# libreries
MLIB = $(SMS++LIB)\
       $(MILPSLIB)\
       $(MMCFLIB)\
       $(libMMCFLIB)

$(NAME): $(MOBJ) $(DIR)main.o 
	$(CC) -o $(NAME) $(DIR)main.o $(MOBJ) $(MLIB) $(SW)

# dependencies: every .o from its .C + every recursively included .h- - - - -

# include directives
MINC = $(libMMCFINC)\
       $(SMS++INC)\
       $(MILPSINC)\
       $(MMCFINC)\
       $(OPTUxINC)

# includes
MH = $(SMS++H)\
     $(MILPSH)\
     $(MMCFH)\
     $(libMMCFH)\
     $(OPTUxH)  

# compile command

$(DIR)main.o: $(DIR)main.cpp $(MH) 
	$(CC) -c $*.cpp -o $@ $(MINC) $(SW)

############################ End of makefile ################################

