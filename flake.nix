{
  description = "A Game Boy emulator written in C.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
    systems.url = "github:nix-systems/default";
    argparse.url = "github:Grazen0/argparse";
  };

  outputs =
    inputs@{ argparse, flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = import inputs.systems;

      perSystem =
        { pkgs, system, ... }:
        {
          packages.default = pkgs.callPackage ./default.nix {
            argparse = argparse.packages.${system}.default;
          };
          devShells.default = pkgs.callPackage ./shell.nix { };
        };
    };
}
