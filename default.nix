{
  stdenv,
  lib,
  cmake,
  pkg-config,
  sdl3,
  argparse,
  unity-test,
  cjson,
  ruby,
}:
stdenv.mkDerivation {
  pname = "gemu";
  version = "0.1.0";

  src = lib.cleanSource ./.;

  nativeBuildInputs = [
    cmake
    pkg-config
    argparse
    sdl3
    unity-test
    cjson
    ruby
  ];

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
    "-DGEMU_BUILD_TESTING=On"
  ];

  enableParallelBuilding = true;
  doCheck = true;

  meta = with lib; {
    description = "A Game Boy emulator written in C.";
    homepage = "https://github.com/Grazen0/gemu";
    license = licenses.gpl3;
  };
}
