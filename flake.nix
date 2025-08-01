{
  description = "A Game Boy emulator written in C.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
    systems.url = "github:nix-systems/default";

    argparse.url = "github:Grazen0/argparse";
  };

  outputs =
    inputs@{
      self,
      argparse,
      flake-parts,
      ...
    }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = import inputs.systems;

      perSystem =
        {
          self',
          pkgs,
          system,
          ...
        }:
        {
          packages = {
            gemu = pkgs.callPackage ./default.nix {
              cjson = pkgs.cjson.overrideAttrs (prev: {
                cmakeFlags = (prev.cmakeFlags or [ ]) ++ [
                  "-DBUILD_SHARED_AND_STATIC_LIBS=On"
                ];
              });
              argparse = argparse.packages.${system}.default;
            };
            default = self'.packages.gemu;
          };

          devShells.default = pkgs.callPackage ./shell.nix {
            inherit (self'.packages) gemu;
          };
        };
    };
}
