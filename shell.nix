{
  pkgs ? import <nixpkgs> { },
  mkShell,
  xxd,
  clang-tools,
  ...
}:
mkShell {
  inputsFrom = [ (pkgs.callPackage ./default.nix { }) ];

  nativeBuildInputs = [
    clang-tools
    xxd
  ];
}
