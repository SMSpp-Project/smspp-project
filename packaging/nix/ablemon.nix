# abLEMON, the fork of LEMON that MCFLemonSolver needs, which nixpkgs does
# not carry; its optional LP and MIP backends are left out.
{
  lib,
  stdenv,
  fetchurl,
  cmake,
}:

let
  # BEGIN of the table written by gen_recipes.py
  version = "1.9.0";
  hash = "sha256-GP96kfT6bjbdq1hPkBIYHxoj6Re+d+qfhRLsTq4GSNY=";
  # END of the table written by gen_recipes.py
in
stdenv.mkDerivation {
  pname = "ablemon";
  inherit version;

  src = fetchurl {
    url = "https://github.com/dmeoli/abLEMON/archive/refs/tags/"
      + "${version}.tar.gz";
    inherit hash;
  };

  nativeBuildInputs = [ cmake ];

  cmakeFlags = [
    (lib.cmakeBool "BUILD_SHARED_LIBS" true)
    (lib.cmakeFeature "LEMON_VERSION" version)
  ]
  ++ map (o: lib.cmakeBool "LEMON_BUILD_${o}" false) [
    "TESTS"
    "DEMOS"
    "TOOLS"
    "DOC"
  ]
  ++ map (o: lib.cmakeBool "LEMON_ENABLE_${o}" false) [
    "GLPK"
    "ILOG"
    "COIN"
    "SOPLEX"
  ];

  meta = {
    description = "Maintained fork of LEMON, a C++ library of graphs";
    homepage = "https://github.com/dmeoli/abLEMON";
    license = with lib.licenses; [
      boost
      bsd3
    ];
    platforms = lib.platforms.unix;
  };
}
