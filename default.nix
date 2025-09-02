{
  stdenv,
  lib,
  cmake,
  pkg-config,
  sdl3,
  unity-test,
  cjson,
  ruby,
}:
stdenv.mkDerivation (finalAttrs: {
  pname = "gemu";
  version = "main";

  src = lib.cleanSource ./.;

  nativeBuildInputs = [
    cmake
    pkg-config
    sdl3
    unity-test
    cjson
    ruby
  ];

  cmakeFlags = [
    (lib.cmakeBool "BUILD_TESTING" finalAttrs.doCheck)
  ];

  enableParallelBuilding = true;
  doCheck = true;

  meta = with lib; {
    description = "A Game Boy emulator written in C.";
    homepage = "https://github.com/Grazen0/gemu";
    license = licenses.gpl3;
  };
})
