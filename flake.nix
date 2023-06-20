{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.05";
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
        cmrc = pkgs.callPackage ./nix/cmrc.nix {};
        vcpkg = pkgs.callPackage ./nix/vcpkg.nix {inherit cmrc;};
      in {
        default = devenv.lib.mkShell {
          inherit inputs pkgs;
          modules = [
            {
              packages = [pkgs.cmake pkgs.ninja pkgs.pkg-config pkgs.gcc vcpkg];

              enterShell = ''
                export VCPKG_INSTALLATION_ROOT=$(vcpkg --root-for-nix-usage)
              '';
            }
          ];
        };
      });
  };
}
