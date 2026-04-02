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
          name = "server";
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
          name = "default";
          pname = "ttp2-default";

          inherit version;
          src = ./.;

          buildInputs = packagesList;

        };
      };

      devShells.${system}.default = let
        devPackages = packagesList
          ++ [ pkgs.bridge-utils pkgs.clang-tools pkgs.asn1c ];
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
