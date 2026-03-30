{
  description = "Tablo Transfere Protocol 2";

  inputs = { nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable"; };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };

      version = "1.2";
      packagesList = with pkgs; [ cmake gcc gnumake ];
    in {
      packages.${system} = {
        client = pkgs.stdenv.mkDerivation {
          name = "client";
          pname = "ttp2-client";

          inherit version;
          src = ./test/client/.;

          buildInputs = packagesList;

          configurePhase = ''
            tmpSrc=$PWD/build-src
            mkdir -p $tmpSrc
            cp -r $src/* $tmpSrc/
            mkdir -p build
            cmake -B build -S $tmpSrc -DCMAKE_BUILD_TYPE=Release
          '';

          buildPhase = ''
            cmake --build build
          '';

          installPhase = ''
            cmake --install build --prefix=$out
            mkdir -p $out/bin
            cp build/ttp2-client $out/bin/ttp2-client
          '';
        };

        server = pkgs.stdenv.mkDerivation {
          name = "server";
          pname = "ttp2-server";

          inherit version;
          src = ./test/server/.;

          buildInputs = packagesList;

          configurePhase = ''
            tmpSrc=$PWD/build-src
            mkdir -p $tmpSrc
            cp -r $src/* $tmpSrc/
            mkdir -p build
            cmake -B build -S $tmpSrc -DCMAKE_BUILD_TYPE=Release
          '';

          buildPhase = ''
            cmake --build build
          '';

          installPhase = ''
            cmake --install build --prefix=$out
            mkdir -p $out/bin
            cp build/ttp2-server $out/bin/ttp2-server
          '';

        };
      };

      devShells.${system}.default =
        let devPackages = packagesList ++ [ pkgs.bridge-utils ];
        in pkgs.mkShell {
          packages = devPackages;

          # bring build tools from our package
          inputsFrom = [ self.packages.${system}.default ];

          shellHook = ''
            git status
          '';
        };
    };
}
