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
          packages =
            let
              mkGemu =
                { frontend }:
                pkgs.stdenv.mkDerivation {
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

                  buildInputs =
                    with pkgs;
                    [ ]
                    ++ (lib.optionals (frontend == "sdl3") [ sdl3 ])
                    ++ (lib.optionals (frontend == "raylib") [ raylib ]);

                  mesonFlags = [
                    "-Dfrontend=${frontend}"
                  ];

                  doCheck = true;

                  meta = with lib; {
                    description = "A Game Boy emulator written in C.";
                    homepage = "https://codeberg.org/Grazen0/gemu";
                    license = licenses.gpl3;
                  };
                };
            in
            {
              gemu = self'.packages.gemu-sdl3;
              gemu-sdl3 = mkGemu { frontend = "sdl3"; };
              gemu-raylib = mkGemu { frontend = "raylib"; };

              default = self'.packages.gemu;
            };

          devShells.default = pkgs.mkShell {
            inputsFrom = [
              self'.packages.gemu-sdl3
              self'.packages.gemu-raylib
            ];
            packages = with pkgs; [ clang-tools ];
            hardeningDisable = [ "fortify" ];
          };
        };
    };
}
