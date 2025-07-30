{
  pkgs ? import <nixpkgs> { },
}:
let
  inherit (pkgs)
    sdl3
    unity-test
    cjson
    clang-tools
    ;
in
pkgs.mkShell {
  inputsFrom = [ (pkgs.callPackage ./default.nix { }) ];

  packages = [
    clang-tools
  ];

  env = {
    CMAKE_PREFIX_PATH = "${sdl3.dev}/lib/cmake:${unity-test}/lib/cmake:${cjson}/lib/cmake";
  };
}
