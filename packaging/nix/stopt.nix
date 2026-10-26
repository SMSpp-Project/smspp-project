# StOpt, the library of stochastic optimization that SDDPBlock needs, which
# nixpkgs does not carry; the geners serialization library travels inside
# it and is built with it, and the SDDP and DP CUTS switches build tests, not
# the library.
{
  lib,
  stdenv,
  fetchurl,
  cmake,
  boost,
  eigen,
  zlib,
  bzip2,
}:

let
  # BEGIN of the table written by gen_recipes.py
  version = "7.1";
  hash = "sha256-i+6PKqsJfJGnkXKc3jT1CwYiCKGrGvTWLuQduiSp5ec=";
  # END of the table written by gen_recipes.py
in
stdenv.mkDerivation {
  pname = "stopt";
  inherit version;

  src = fetchurl {
    url = "https://gitlab.com/stochastic-control/StOpt/-/archive/"
      + "v${version}/StOpt-v${version}.tar.gz";
    inherit hash;
  };

  nativeBuildInputs = [ cmake ];
  # what links StOpt finds these too
  propagatedBuildInputs = [
    boost
    eigen
    zlib
    bzip2
  ];

  cmakeFlags = [
    (lib.cmakeBool "BUILD_SHARED_LIBS" true)
    (lib.cmakeBool "BUILD_PYTHON" false)
    (lib.cmakeBool "BUILD_TEST" false)
    (lib.cmakeBool "BUILD_SDDP" false)
    (lib.cmakeBool "BUILD_DPCUTS" false)
    (lib.cmakeBool "BUILD_MPI" false)
    (lib.cmakeBool "BUILD_SYSTEM_INSTALL" true)
  ];

  meta = {
    description = "Library for stochastic optimization by dynamic programming";
    homepage = "https://gitlab.com/stochastic-control/StOpt";
    license = lib.licenses.lgpl3Only;
    platforms = lib.platforms.unix;
  };
}
