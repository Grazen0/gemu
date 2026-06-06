{
  description = "A Game Boy emulator written in C.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
    systems.url = "github:nix-systems/default";
  };

  outputs =
    inputs@{
      self,
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
        let
          inherit (pkgs) lib;
        in
        {
          packages = {
            gemu = pkgs.stdenv.mkDerivation (finalAttrs: {
              pname = "gemu";
              version = "main";

              src = lib.cleanSource ./.;

              nativeBuildInputs = with pkgs; [
                meson
                ninja
                pkg-config
                unity-test
                cjson
                ruby
              ];

              buildInputs = with pkgs; [
                sdl3
              ];

              doCheck = true;

              meta = with lib; {
                description = "A Game Boy emulator written in C.";
                homepage = "https://codeberg.org/Grazen0/gemu";
                license = licenses.gpl3;
              };
            });

            default = self'.packages.gemu;
          };

          devShells.default = pkgs.mkShell {
            inputsFrom = [ self'.packages.gemu ];
            packages = with pkgs; [ clang-tools ];
            hardeningDisable = [ "fortify" ];
          };
        };
    };
}
