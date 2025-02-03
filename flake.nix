{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
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
              packages = [pkgs.cmake pkgs.ninja pkgs.pkg-config pkgs.gcc pkgs.vcpkg pkgs.libgccjit pkgs.zip];

              enterShell = ''
                export VCPKG_ROOT="${pkgs.vcpkg}/share/vcpkg"
              '';
            }
          ];
        };
      });
  };
}
