{
  description = "Tomasulo";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";

  };

  outputs =
    {
      self,
      nixpkgs,

      ...
    }@inputs:
    let

      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};

    in
    {

      devShells.${system}.default = pkgs.mkShell {
        buildInputs = [
          pkgs.gcc
          pkgs.cmake
          # pkgs.make

          (pkgs.writeShellScriptBin "compile" ''
                        
            g++ -o tomasulo main.cpp
          '')
        ];

      };
    };
}
