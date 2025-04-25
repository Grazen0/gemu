{
  pkgs ? import <nixpkgs> { },
  mkShell,
  xxd,
  sdl3,
  ...
}:
mkShell {
  inputsFrom = [ (pkgs.callPackage ./default.nix { }) ];

  env = {
    CMAKE_PREFIX_PATH = "${sdl3.dev}/lib/cmake";
  };

  nativeBuildInputs = [ xxd ];
}
