{
  pkgs ? import <nixpkgs> { },
}:
let
  inherit (pkgs)
    stdenv
    lib
    cmake
    sdl3
    unity-test
    cjson
    ruby
    ;
in
stdenv.mkDerivation {
  pname = "gemu";
  version = "0.1.0";

  src = lib.cleanSource ./.;

  nativeBuildInputs = [
    cmake
    sdl3
    unity-test
    cjson
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

  meta = with lib; {
    description = "A Game Boy emulator written in C.";
    homepage = "https://github.com/Grazen0/gemu";
    license = licenses.gpl3;
  };
}
