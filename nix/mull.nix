# Mull mutation-testing tool, repackaged from the upstream prebuilt release.
#
# Why prebuilt instead of from-source: as of 0.34.0 Mull's build moved to
# Bazel (no CMakeLists), which does not compose cleanly with nix's sandbox.
# Upstream publishes per-LLVM-version binaries, and the LLVM 19.1.7 build
# matches nixpkgs `llvmPackages_19` (also 19.1.7) byte-for-byte at the LLVM
# minor level, so the `mull-ir-frontend-19` clang pass plugin is ABI-compatible
# with this nixpkgs' `clang_19`. We fetch the .deb and autoPatchelf it onto the
# nix LLVM/clang/libc runtime.
#
# Provides: mull-runner-19, mull-instrument-19, mull-reporter-19, and the
# clang pass plugin at $out/lib/mull-ir-frontend-19.
{ lib
, stdenv
, fetchurl
, dpkg
, autoPatchelfHook
, llvmPackages_19
}:

let
  llvm = llvmPackages_19;
  version = "0.34.0";

  # sha256 values are base32, as emitted by `nix-prefetch-url`.
  sources = {
    x86_64-linux = {
      url = "https://github.com/mull-project/mull/releases/download/${version}/Mull-19-${version}-LLVM-19.1.7-debian-amd64-13.deb";
      sha256 = "01jwpbzly01z5z3f1kqsww6zbgk11lrkn9khcg3jvawdrlr85rbv";
    };
    aarch64-linux = {
      url = "https://github.com/mull-project/mull/releases/download/${version}/Mull-19-${version}-LLVM-19.1.7-ubuntu-aarch64-26.04.deb";
      sha256 = "078kdscycq2nsnjp8mbbxz1b4canyjzn623lcpxf7z90m21rw7n0";
    };
  };

  source = sources.${stdenv.hostPlatform.system}
    or (throw "mull ${version}: no prebuilt release for ${stdenv.hostPlatform.system}");
in
stdenv.mkDerivation {
  pname = "mull";
  inherit version;

  src = fetchurl { inherit (source) url sha256; };

  nativeBuildInputs = [ dpkg autoPatchelfHook ];

  # NEEDED by mull-instrument-19 and the mull-ir-frontend-19 plugin:
  #   libLLVM.so.19.1      <- llvm.lib
  #   libclang-cpp.so.19.1 <- clang-unwrapped.lib
  #   libgcc_s, libstdc++  <- stdenv.cc.cc.lib
  buildInputs = [
    stdenv.cc.cc.lib
    llvm.llvm.lib
    llvm.clang-unwrapped.lib
  ];

  unpackPhase = ''
    runHook preUnpack
    dpkg-deb -x "$src" .
    runHook postUnpack
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p "$out"
    cp -r usr/* "$out/"
    runHook postInstall
  '';

  # Expose the plugin path so consumers (devShell, CI) don't hardcode it.
  passthru.llvmVersion = "19";
  passthru.irFrontend = "/lib/mull-ir-frontend-19";

  meta = with lib; {
    description = "Practical mutation testing and fault injection for C and C++ (LLVM 19 build)";
    homepage = "https://github.com/mull-project/mull";
    license = licenses.asl20;
    platforms = [ "x86_64-linux" "aarch64-linux" ];
    mainProgram = "mull-runner-19";
  };
}
