# Cammi Dolphin

Based on KDE Dolphin v26.04.3. The cammi-tabs branch contains the modified
source and .cammi/ contains a reproducible Nix adapter over KDE's release tarball
(which retains translations). Both contain the same downstream tab behavior.

Build/test: nix build ./.cammi#dolphin
Cache: https://github.com/dsat4200/dolphin/releases/download/nix-dolphin-cache
GitHub Actions builds and tests the package before publishing signed NARs.
Only the public verification key belongs in Nix client configurations; the
private key is an Actions secret, never a repository file or store input.

Consumers should pin this flake with dir=.cammi and synchronize nixpkgs locks
with .cammi/flake.lock to get matching binaries. CI publishes x86_64-linux.
Other architectures/uncached versions require a source build until CI covers
them. KDE's existing licenses apply; downstream tests are GPL-2.0-or-later.
