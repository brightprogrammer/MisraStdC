{
  description = "Nix flake for MisraStdC";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      lib = nixpkgs.lib;
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forAllSystems = f:
        lib.genAttrs systems (system:
          f {
            inherit system;
            pkgs = import nixpkgs { inherit system; };
          });
      # Mull only ships prebuilt Linux binaries (see nix/mull.nix), so the
      # mutation-testing package and dev shell are gated to Linux systems.
      isLinux = system: lib.hasSuffix "linux" system;
    in
    {
      packages = forAllSystems ({ system, pkgs }:
        let
          misraStdC = pkgs.stdenv.mkDerivation {
            pname = "misra-std-c";
            version = if self ? shortRev then self.shortRev else "dirty";

            src = self;
            strictDeps = true;

            nativeBuildInputs = with pkgs; [
              meson
              ninja
              pkg-config
            ];

            doCheck = true;

            checkPhase = ''
              runHook preCheck
              meson test --print-errorlogs
              runHook postCheck
            '';

            meta = with pkgs.lib; {
              description = "Macro-based generic containers, formatting, and utilities for C11";
              homepage = "https://git.anvielabs.com/bp/MisraStdC.git";
              license = licenses.unlicense;
              platforms = platforms.unix;
            };
          };
        in
        {
          default = misraStdC;
          "misra-std-c" = misraStdC;
        }
        // lib.optionalAttrs (isLinux system) {
          mull = pkgs.callPackage ./nix/mull.nix { };
        });

      checks = forAllSystems ({ system, ... }: {
        default = self.packages.${system}.default;
      });

      devShells = forAllSystems ({ system, pkgs }:
        {
          default = pkgs.mkShell {
            inputsFrom = [ self.packages.${system}.default ];
            packages = with pkgs; [
              stdenv.cc
              meson
              ninja
              pkg-config
            ];
          };
        }
        // lib.optionalAttrs (isLinux system) {
          # Mutation-testing shell: clang_19 (compiler whose libLLVM matches the
          # mull-ir-frontend-19 plugin) + mull + the usual build tools. Run
          # Scripts/mutation.sh from inside `nix develop .#mutation`.
          mutation = pkgs.mkShell {
            packages = with pkgs; [
              llvmPackages_19.clang
              self.packages.${system}.mull
              meson
              ninja
              pkg-config
            ];
            shellHook = ''
              export CC=clang
              export MULL_LLVM_VERSION=19
              export MULL_IR_FRONTEND="${self.packages.${system}.mull}/lib/mull-ir-frontend-19"
              export MULL_RUNNER="mull-runner-19"
            '';
          };
        });
    };
}
