with import <nixpkgs> {};

mkShell {
  buildInputs = [
    gcc
    gdb
    cmake
    bear
    pkg-config
    clang-tools
  ];
}
