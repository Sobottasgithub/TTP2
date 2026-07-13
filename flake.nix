{
  description = "Tablo Transfer Protocol 2";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };

      version = "2.0.7";

      commonDeps = with pkgs; [
        cmake
        gcc
        gnumake
        libtasn1
        arrow-cpp
      ];

      mkTTP2Package =
        {
          pname,
          buildTarget,
          enableLib ? false,
          enableClient ? false,
          enableServer ? false,
          extraInputs ? [ ],
        }:
        pkgs.stdenv.mkDerivation {
          inherit pname version;
          src = ./.;

          buildInputs = commonDeps ++ extraInputs;

          configurePhase = ''
            cmake -B build -S $src \
              -DCMAKE_BUILD_TYPE=Release \
              -DDEF_TTP2=${if enableLib then "ON" else "OFF"} \
              -DDEF_CLIENT=${if enableClient then "ON" else "OFF"} \
              -DDEF_SERVER=${if enableServer then "ON" else "OFF"}
          '';

          buildPhase = ''
            cmake --build build \
              --target ${buildTarget} \
              -j$NIX_BUILD_CORES
          '';

          installPhase = ''
            cmake --install build --prefix=$out
            cp LICENSE $out/
          '';
        };

    in
    {
      packages.${system} =
        let
          lib = mkTTP2Package {
            pname = "libttp2";
            buildTarget = "ttp2";
            enableLib = true;
          };
        in
        {
          inherit lib;

          client = mkTTP2Package {
            pname = "ttp2-client";
            buildTarget = "ttp2-client";
            enableClient = true;
            extraInputs = [ lib ];
          };

          server = mkTTP2Package {
            pname = "ttp2-server";
            buildTarget = "ttp2-server";
            enableServer = true;
            extraInputs = [ lib ];
          };

          full = mkTTP2Package {
            pname = "libttp2-full";
            buildTarget = "all";
            enableLib = true;
            enableServer = true;
            enableClient = true;
          };

          default = self.packages.${system}.lib;
        };

      devShells.${system}.default = pkgs.mkShell {
        packages = commonDeps ++ [
          pkgs.bridge-utils
          pkgs.clang-tools
        ];

        shellHook = ''
          ./build-asn1-packets.sh
          git status
        '';
      };
    };
}
