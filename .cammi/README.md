# Custom Dolphin build

This directory packages the fork's linear tabs. Build Dolphin with
`nix build ./.cammi#dolphin`; its offscreen Qt integration tests run in the build.

PDF extraction, PowerRename and image resizing have moved to the independent
[File Tools repository](https://github.com/dsat4200/file-tools). Install that
program to get its normal KDE right-click service menus. Tool updates no longer
modify this fork or trigger a Dolphin rebuild.

The `dolphin-cache.yml` workflow builds/tests the exact Dolphin derivation before
publishing its signed closure. Advance the consuming NixOS `dolphin` input after
publication, then rebuild and reopen Dolphin. File Tools has its own independent
build and regression checks in its repository.
