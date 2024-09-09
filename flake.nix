{
  description = "NixImage";

  inputs = {
    nixpkgs = {
      url = "github:NixOS/nixpkgs/nixos-unstable";
    };

    flakelight = {
      url = "github:nix-community/flakelight";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { ... }@inputs:
    inputs.flakelight ./. ({ lib, ... }: {
      inherit inputs;
      systems = [ "x86_64-linux" "aarch64-linux" ];
      nixDir = ./.;
      nixDirAliases = {
        packages = [ "packages" ];
        devShells = [ "dev-shells" ];
        bundlers = [ "bundlers" ];
      };
    });
}
