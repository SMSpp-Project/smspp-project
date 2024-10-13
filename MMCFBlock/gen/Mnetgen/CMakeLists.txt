# --------------------------------------------------------------------------- #
#    CMake file for MMCFBlock/gen/Mnetgen                                     #
# --------------------------------------------------------------------------- #

# This is needed for setting the runtime path when installing.
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# ----- dmx2nc4 ------------------------------------------------------------- #
add_executable(mnetgen mnetgen.C)

# --------------------------------------------------------------------------- #
