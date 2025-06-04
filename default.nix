{
  pkgs ? import <nixpkgs> { },
  ...
}:
let
  inherit (pkgs)
    lib
    cmake
    sdl3
    xxd
    unity-test
    ruby
    ;
in
pkgs.stdenv.mkDerivation {
  pname = "gemu";
  version = "0.1.0";

  src = lib.cleanSource ./.;

  nativeBuildInputs = [
    cmake
    sdl3.dev
    xxd
    unity-test
    ruby
  ];

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
    "-DBUILD_TESTING=on"
  ];

  enableParallelBuilding = true;

  doCheck = true;

  installPhase = ''
    runHook preInstall
    install -Dm755 gemu -t "$out/bin"
    runHook postInstall
  '';

  env = {
    CMAKE_PREFIX_PATH = "${sdl3.dev}/lib/cmake:${unity-test}/lib/cmake";
  };

  meta = with lib; {
    description = "A Game Boy emulator written in C.";
    homepage = "https://github.com/Grazen0/gemu";
    license = licenses.gpl3;
  };
}
