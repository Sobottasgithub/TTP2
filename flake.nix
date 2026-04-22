{
  description = "Tablo Transfer Protocol 2";

  inputs = { nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable"; };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };

      version = "1.2";
      packagesList = with pkgs; [ cmake gcc gnumake libtasn1 ];
    in {
      packages.${system} = {
        client = pkgs.stdenv.mkDerivation {
          pname = "ttp2-client";

          inherit version;
          src = ./.;

          buildInputs = packagesList;

          configurePhase = ''
            cmake -B build -S $src -DCMAKE_BUILD_TYPE=Release
          '';

          buildPhase = ''
            cmake --build build
          '';

          installPhase = ''
            cmake --install build --prefix=$out
            cp LICENSE $out/
          '';
        };

        server = pkgs.stdenv.mkDerivation {
          pname = "ttp2-server";

          inherit version;
          src = ./.;

          buildInputs = packagesList;

          configurePhase = ''
            cmake -B build -S $src -DCMAKE_BUILD_TYPE=Release
          '';

          buildPhase = ''
            cmake --build build
          '';

          installPhase = ''
            cmake --install build --prefix=$out
            cp LICENSE $out/
          '';
        };

        default = pkgs.stdenv.mkDerivation {
          pname = "default";

          inherit version;
          src = ./.;

          buildInputs = packagesList;
        };
      };

      devShells.${system}.default = let
        devPackages = packagesList
          ++ [ pkgs.bridge-utils pkgs.clang-tools pkgs.libtasn1 ];
      in pkgs.mkShell {
        packages = devPackages;

        # bring build tools from our package
        inputsFrom = [ self.packages.${system}.default ];

        shellHook = ''
          cd lib/ttp2/src/
          ./build-asn1-packets.sh
          cd ../../../

          git status
        '';
      };
    };
}
