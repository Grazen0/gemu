{
  gemu,
  mkShell,
  clang-tools,
}:
mkShell {
  inputsFrom = [ gemu ];

  packages = [ clang-tools ];
}
