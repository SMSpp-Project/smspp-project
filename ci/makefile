##############################################################################
################################# makefile ###################################
##############################################################################
#                                                                            #
#   makefile of the CI: builds one module with its makefiles                 #
#                                                                            #
#   Builds the objects (or the library) of the module $(MODULE) and of the   #
#   modules it depends on, as its makefile-c gives them, and its C++20       #
#   module if $(SMSppMOD) is not empty [see SMS++/lib/makefile-mod]; the     #
#   pipelines run it after the build with CMake, so that the makefiles are   #
#   kept working as well, e.g.:                                              #
#                                                                            #
#       make -f ci/makefile -j8 MODULE=UCBlock                               #
#       make -f ci/makefile -j8 MODULE=UCBlock CC=clang++ SMSppMOD=1         #
#                                                                            #
#   from the directory of the umbrella. The prefix of the macros of the      #
#   module (e.g., UCBck) and the macro of its directory (e.g., UCBckDIR)     #
#   are read from its makefile, so that any module is built with no table    #
#   to keep. FastFlow is taken from $(FastFlow_ROOT), by default the one     #
#   that CMake fetched in build/.                                            #
#                                                                            #
#       make -f ci/makefile clean-all                                        #
#                                                                            #
#   removes the objects, the libraries and the module files of all the       #
#   modules, so that another compiler starts anew.                           #
#                                                                            #
#   Input:  $(MODULE)        = the directory of the module (e.g., SMS++)     #
#           $(CC), $(SW)     = compiler command and options (default g++,    #
#                              -O1 -std=c++20 -DNDEBUG)                      #
#           $(SMSppMOD)      = not empty if the C++20 modules are built      #
#           $(FastFlow_ROOT) = the directory of FastFlow                     #
#                                                                            #
#                                 Donato Meoli                               #
#                         Dipartimento di Informatica                        #
#                             Universita' di Pisa                            #
#                                                                            #
##############################################################################

# all is the default, though the makefiles of the modules, included before
# it, define clean first
.DEFAULT_GOAL := all

# compiler and options- - - - - - - - - - - - - - - - - - - - - - - - - - - -

# NDEBUG as in the release builds of the makefiles of the tests and of the
# tools, which link these objects afterwards: it is part of the binary
# interface, since without it Block and the classes derived from it declare
# the virtual methods expected_dims() and expected_vars(), and an object
# compiled the other way calls the virtual methods declared after those
# through the wrong entry of the vtable (e.g., register_Solver(), so that a
# BlockSolverConfig attaches no Solver, or load() in place of serialize());
# whoever gives SW has to agree on NDEBUG with what links the objects
CC = g++
SW = -O1 -std=c++20 -DNDEBUG

# FastFlow, the one CMake fetched unless it is given on the command line: the
# default paths of extlib would set /opt/fastflow, hence the override
ifneq ($(origin FastFlow_ROOT),command line)
    override FastFlow_ROOT = $(CURDIR)/build/_deps/fastflow-src
endif

# the module - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ifneq ($(MODULE),)
    # its makefile, its makefile-c, the prefix of its macros and the macro of
    # its directory
    CIMK := $(firstword $(wildcard $(MODULE)/lib/makefile $(MODULE)/makefile))
    CIMKC := $(CIMK)-c
    CIPFX := $(shell sed -n 's/^\([A-Za-z+]*\)OBJ *=.*/\1/p' $(CIMK) | head -1)
    CIDIR := $(shell grep -o '$$($(CIPFX)\(SDR\|DIR\))' $(CIMK) | head -1 \
	| tr -d '$$()')
    ifeq ($(CIDIR),)
        $(error $(MODULE) has no makefile with the macros of a module)
    endif
    $(CIDIR) = $(CURDIR)/$(MODULE)
    include $(CIMKC)
endif

# default target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

all: $($(CIPFX)OBJ) $($(CIPFX)MOD)
	@echo "$(MODULE): built with the makefiles"

# clean - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

clean-all:
	find . -path ./build -prune -o \( -name '*.o' -o -name '*.a' \
	-o -name '*.pcm' \) -type f -print | xargs rm -f

# phony targets - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

.PHONY: all clean-all

############################ End of makefile #################################
