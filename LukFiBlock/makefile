##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of test (for core LukFiBlock)                                   #
#                                                                            #
#                              Antonio Frangioni                             #
#                          Dipartimento di Informatica                       #
#                              Universita' di Pisa                           #
#                                                                            #
##############################################################################

# module name
NAME = LukFi_test

# basic directory
DIR = .

# common flags
COMMON_SW = -std=c++17 -ferror-limit=1

# debug switches
SW_DEBUG = -g3 -glldb -fno-inline $(COMMON_SW)
# debug switches with address sanitizer and extra pedantic warning
SW_DEBUG_ASAN = -g3 -glldb -fno-inline $(COMMON_SW) -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer -Wpedantic -Wextra -Wno-unused-parameter -Wno-enum-compare -DCLANG_1200_0_32_27_PATCH
# production switches
SW_RELEASE = -O3 -DNDEBUG $(COMMON_SW)

# compiler
CC = clang++

# default target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

default: release

# debug target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

debug: SW = $(SW_DEBUG)
debug: $(DIR)/$(NAME)

# debug_asan target - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

debug_asan: SW = $(SW_DEBUG_ASAN)
debug_asan: $(DIR)/$(NAME)

# release target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

release: SW = $(SW_RELEASE)
release: $(DIR)/$(NAME)

# clean target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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

# core SMS++
SMS++SDR = ../../SMS++
include $(SMS++SDR)/lib/makefile-inc

# BundleSolver
BNDSLVSDR = ../../BundleSolver
include $(BNDSLVSDR)/makefile-c

# LukFiBlock
LukFiSDR = ../../LukFiBlock
include $(LukFiSDR)/makefile

# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ =  $(LukFiOBJ) $(SMS++OBJ) $(BNDSLVOBJ)

# libraries
MLIB =  $(SMS++LIB) $(BNDSLVLIB) $(LukFiLIB)

$(DIR)/$(NAME): $(MOBJ) $(DIR)/main.o
	$(CC) -o $(DIR)/$(NAME) $(DIR)/main.o $(MOBJ) $(MLIB) $(SW)

# dependencies: every .o from its .cpp + every recursively included .h- - - -

# include directives
MINC =  $(SMS++INC) $(BNDSLVINC) $(LukFiINC)

# includes
MH =    $(SMS++H) $(BNDSLVH) $(LukFiH)

# compile command

$(DIR)/main.o: $(DIR)/main.cpp $(MH)
	$(CC) -c $*.cpp -o $@ $(MINC) $(SW)

# install/uninstall - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

prefix = /usr/local
bindir = $(prefix)/bin

install: $(DIR)/$(NAME)
	install -s -D $(DIR)/$(NAME) $(DESTDIR)$(bindir)/$(NAME)

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(NAME)

# distclean target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

distclean: clean

# phony targets - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

.PHONY: debug debug_asan release install uninstall clean distclean

############################ End of makefile ################################
