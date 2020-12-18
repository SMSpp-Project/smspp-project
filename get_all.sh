#!/usr/bin/env bash
# --------------------------------------------------------------------------- #
#    This Bash script (sorry, plain sh does not support arrays) fetches       #
#    all the SMS++ submodules and checks out their master branches.           #
#    Comment out the submodules you do not need.                              #
# --------------------------------------------------------------------------- #

submodules=(
  "BundleSolver"
  "LukFiBlock"
  "MCFBlock"
  "MCFClass"
  "MILPSolver"
  "SDDPBlock"
  "SMILPBlock"
  "SMS++"
  "StochasticBlock"
#  "SubGradSolver"
  "UCBlock"
  "DPSolver"
  "tests"
  "tools"
)

for i in "${submodules[@]}"; do
  :
  git submodule init "$i"
  git submodule update --recursive --remote "$i"
done

git submodule foreach git checkout master
