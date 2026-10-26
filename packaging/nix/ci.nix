# The check of the CI [see the job packaging:nix of .gitlab-ci.yml]: the
# package with the given modules only, on the nixpkgs that flake.lock pins,
# and the consumer of packaging/consumer built against it, which makes a
# UCBlock and a HiGHSMILPSolver by the factories; then the Unit Commitment
# tool solves the example installed with it.
#   nix-build packaging/nix/ci.nix --argstr modules ucblock,milp,mcfclass
{
  modules ? "ucblock,milp,mcfclass",
}:

let
  lock = (builtins.fromJSON (builtins.readFile ./flake.lock)).nodes.nixpkgs;
  pkgs = import (fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/${lock.locked.rev}.tar.gz";
    sha256 = lock.locked.narHash;
  }) { };
  stopt = pkgs.callPackage ./stopt.nix { };
  smspp = pkgs.callPackage ./package.nix {
    inherit stopt;
    modules = pkgs.lib.splitString "," modules;
  };
in
pkgs.stdenv.mkDerivation {
  name = "smspp-consumer";
  src = ../consumer;
  nativeBuildInputs = [ pkgs.cmake ];
  buildInputs = [ smspp ];
  installPhase = "install -Dm755 consumer $out/bin/consumer";
  doInstallCheck = true;
  installCheckPhase = ''
    $out/bin/consumer
    ${smspp}/bin/smspp_ucblock_solver \
      ${smspp}/share/SMS++_tools/ucblock_solver/examples/Bus_Test.nc4 \
      | tee uc.log
    grep -q "Upper bound = 4.14035578e+05" uc.log
  '';
}
