{
  pkgs ? import <nixpkgs> { },
  mkShell,
  sdl3,
  unity-test,
  ...
}:
mkShell {
  inputsFrom = [ (pkgs.callPackage ./default.nix { }) ];

  env = {
    CMAKE_PREFIX_PATH = "${sdl3.dev}/lib/cmake:${unity-test}/lib/cmake";
  };
}
