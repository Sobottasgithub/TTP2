{
  description = "Tablo Transfer Protocol 2";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";

    tablog = {
      url = "github:Sobottasgithub/tablog";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      tablog,
    }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      forEachSystem = nixpkgs.lib.genAttrs supportedSystems;
    in
    {
      packages = forEachSystem (system:
        let
          pkgs = import nixpkgs { inherit system; };
          version = "2.3.0";
          libtablog = tablog.packages.${system}.lib;

          commonDeps = with pkgs; [
            cmake
            gcc
            gnumake
            libtasn1
            arrow-cpp
            libtablog
          ];

          mkTTP2Package =
            {
              pname,
              buildTarget,
              enableLib ? false,
              enableClient ? false,
              enableServer ? false,
              enablePeer ? false,
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
                  -DDEF_SERVER=${if enableServer then "ON" else "OFF"} \
                  -DDEF_PEER=${if enablePeer then "ON" else "OFF"}
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

          lib = mkTTP2Package {
            pname = "libttp2";
            buildTarget = "ttp2";
            enableLib = true;
          };
        in
        {
          inherit lib libtablog;

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

          peer = mkTTP2Package {
            pname = "ttp2-peer";
            buildTarget = "ttp2-peer";
            enablePeer = true;
            extraInputs = [ lib ];
          };

          full = mkTTP2Package {
            pname = "libttp2-full";
            buildTarget = "all";
            enableLib = true;
            enableServer = true;
            enableClient = true;
            enablePeer = true;
          };

          default = lib;
        }
      );

      devShells = forEachSystem (system:
        let
          pkgs = import nixpkgs { inherit system; };
          libtablog = tablog.packages.${system}.lib;
          commonDeps = with pkgs; [
            cmake
            gcc
            gnumake
            libtasn1
            arrow-cpp
            libtablog
          ];
        in
        {
          default = pkgs.mkShell {
            packages = commonDeps ++ [
              pkgs.bridge-utils
              pkgs.clang-tools
            ];

            shellHook = ''
              ./build-asn1-packets.sh
              git status
            '';
          };
        }
      );
    };
}
