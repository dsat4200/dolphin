# Cammi linear tabs

Downstream of KDE Dolphin v26.04.3. This branch contains the source for flat
leaf navigation, adjacent tab pairing, state-preserving unsplit, group moves,
focused-leaf close/undo-close, adaptive portrait/landscape splits, and opaque
folder-style labels with grayscale contrast text.

The authoritative Nix adapter and CI build are maintained at
https://github.com/dsat4200/cammi-nix (packages/dolphin.nix). The fork is a
reviewable source mirror, not a second independently versioned Nix package.
The Nix recipe applies the same changes to KDE's release tarball to retain
release translations and ensure that CI/cache and configured machines share
exactly the same derivation. Update this branch when those adapters change.

GitHub Actions in cammi-nix publishes a signed Nix cache to its GitHub Releases.
Keep flake.lock synchronized to reuse the binary; no accounts or application
state are included. Existing KDE licenses apply; downstream test code is
GPL-2.0-or-later.
