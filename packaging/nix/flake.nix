# The flake of SMS++:
#   nix build "gitlab:smspp/smspp-project?dir=packaging/nix"
# builds every module whose dependencies nixpkgs has, with the tools, and
# `nix run` of the same reference runs smspp_block_solver; the release it
# builds is the one of package.nix. StOpt, which SDDPBlock needs, is not in
# nixpkgs, and it is built here from stopt.nix.
{
  description = "SMS++, a framework for block-structured optimization";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forAll =
        f: nixpkgs.lib.genAttrs systems (s: f nixpkgs.legacyPackages.${s});
    in
    {
      packages = forAll (
        pkgs:
        let
          stopt = pkgs.callPackage ./stopt.nix { };
          smspp = pkgs.callPackage ./package.nix { inherit stopt; };
        in
        {
          inherit smspp stopt;
          default = smspp;
        }
      );

      # a program that finds UCBlock and HiGHSMILPSolver by their CMake
      # configuration and makes them by the factories, which works only if
      # the libraries of the modules have been kept at link time
      checks = forAll (pkgs: {
        consumer = pkgs.stdenv.mkDerivation {
          name = "smspp-consumer";
          src = ./consumer;
          nativeBuildInputs = [ pkgs.cmake ];
          buildInputs = [
            self.packages.${pkgs.stdenv.hostPlatform.system}.smspp
          ];
          doInstallCheck = true;
          installPhase = "install -Dm755 consumer $out/bin/consumer";
          installCheckPhase = "$out/bin/consumer";
        };
      });

      overlays.default = final: prev: {
        stopt = final.callPackage ./stopt.nix { };
        smspp = final.callPackage ./package.nix { };
      };
    };
}
