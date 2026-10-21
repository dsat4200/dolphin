{
  description = "Cammi Dolphin: linear tabs, adaptive splits and signed binary cache";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
  outputs = { self, nixpkgs }: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
    dolphin = pkgs.callPackage ./packages/dolphin.nix { };
  in {
    packages.${system} = { default = dolphin; inherit dolphin; };
    checks.${system}.dolphin = dolphin;
  };
}
