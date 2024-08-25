##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of test (for Write-Read)                                        #
#                                                                            #
#                              Antonio Frangioni                             #
#                          Dipartimento di Informatica                       #
#                              Universita' di Pisa                           #
#                                                                            #
##############################################################################

# module name
NAME = writeandread_test

# basic directory
DIR = .

# common flags
COMMON_SW = -std=c++17 -ferror-limit=1 -Wno-enum-compare

# debug switches
SW_DEBUG = -g3 -glldb -fno-inline $(COMMON_SW)
# debug switches with address sanitizer and extra pedantic warning
SW_DEBUG_ASAN = -g3 -g -fno-inline $(COMMON_SW) -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer -Wpedantic -Wextra -Wno-unused-parameter -DCLANG_1200_0_32_27_PATCH
# production switches with address sanitizer
SW_RELEASE_ASAN = -O3 $(COMMON_SW) -DNDEBUG -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
# production switches
SW_RELEASE = -O3 $(COMMON_SW) -DNDEBUG -DCLANG_1200_0_32_27_PATCH

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

# release_asan target - - - - - - - - - - - - - - - - - - - - - - - - - - - -

release_asan: SW = $(SW_RELEASE_ASAN)
release_asan: $(DIR)/$(NAME)

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

# define input macros for SMS++ complete makefile, then include it
SMS++SDR = ../../SMS++/
include $(SMS++SDR)lib/makefile-c

# MILPSolver
MILPSSDR = ../../MILPSolver
include $(MILPSSDR)/makefile-s

# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ =  $(SMS++OBJ) $(MILPSOBJ)

# libraries
MLIB =  $(SMS++LIB) $(MILPSLIB)

$(DIR)/$(NAME): $(DIR)/test.o $(MOBJ)
	$(CC) -o $(DIR)/$(NAME) $^ $(MLIB) $(SW)

# dependencies: every .o from its .cpp + every recursively included .h- - - -

# include directives
MINC =  $(SMS++INC) $(MILPSINC)

# includes
MH =    $(SMS++H) $(MILPSH)

# compile command

$(DIR)/test.o: $(DIR)/test.cpp $(MH)
	$(CC) -c $(DIR)/test.cpp -o $@ $(MINC) $(SW)

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

.PHONY: debug debug_asan release release_asan install uninstall clean distclean

############################ End of makefile #################################
