{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    systems.url = "github:nix-systems/default";
    devenv.url = "github:cachix/devenv";
  };

  outputs = {
    self,
    nixpkgs,
    devenv,
    systems,
    ...
  } @ inputs: let
    forEachSystem = nixpkgs.lib.genAttrs (import systems);
  in {
    devShells =
      forEachSystem
      (system: let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        default = devenv.lib.mkShell {
          inherit inputs pkgs;
          modules = [
            {
              packages = [pkgs.cmake pkgs.ninja pkgs.pkg-config pkgs.gcc pkgs.vcpkg pkgs.libgccjit];

              enterShell = ''
                export VCPKG_ROOT=$(realpath $(dirname $(readlink -f $(type -p vcpkg)))/../share/vcpkg)
              '';
            }
          ];
        };
      });
  };
}
