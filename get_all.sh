#!/usr/bin/env bash
# --------------------------------------------------------------------------- #
#    This Bash script (sorry, plain sh does not support arrays) fetches       #
#    all the SMS++ submodules and checks out their master branches.           #
#    Comment out the submodules you do not need.                              #
# --------------------------------------------------------------------------- #

submodules=(
#  "LukFiBlock"
  "MCFBlock"
  "MCFClass"
  "MILPSolver"
  "SMILPBlock"
  "SMS++"
#  "SubGradSolver"
  "UCBlock"
)

for i in "${submodules[@]}"; do
  :
  git submodule init "$i"
  git submodule update --remote "$i"
done

git submodule foreach git checkout master
