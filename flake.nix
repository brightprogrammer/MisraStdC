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
        });
    };
}
