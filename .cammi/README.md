# Custom Dolphin build

This directory packages the fork's linear tabs and its local-file right-click
actions. Build Dolphin with `nix build ./.cammi#dolphin`. The package includes
its service menus; no manual copying into `~/.local/share` is required.

## Right-click actions

- **Extract Pages to Images** appears on PDFs. `Example.pdf` creates the sibling
  directory `Example/`, containing whole-page, 300-DPI PNGs named `page-001.png`,
  `page-002.png`, etc. Each selected PDF gets its own folder. An existing file,
  folder or symlink at that name is an error, never an overwrite. Invalid or
  password-protected PDFs report errors and their incomplete output is removed.
- **PowerRename…** opens KDE's KRename with the selected files/folders already
  loaded. It provides search/replace (including regular expressions), case
  changes, numbering, a before/after preview and undo. Nothing is renamed until
  confirmed in KRename. This integrates a maintained KDE tool, not Microsoft's
  Windows application or a new home-grown rename engine. A small packaging patch
  makes command-line folder selections rename the folders themselves, rather than
  implicitly expanding their contents; explicit recursive mode is unchanged.
- **Resize Pictures…** offers Small (854×480), Medium (1366×768), Large
  (1920×1080), Phone (320×568), or custom pixel/percentage sizes. Choose Fit
  (proportional), Fill (crop), or Stretch, shrink-only, portrait/landscape matching,
  JPEG/WebP/AVIF quality and optional metadata removal. Copies stay beside their
  originals and keep the original format. Names include the preset or custom
  size, with numbered suffixes on collisions. Originals are never replaced.

Resizing supports JPEG, PNG, WebP, BMP, TIFF, GIF and AVIF when supported by Pillow's
codec. Animated images and multipage TIFFs are explicitly skipped rather than
silently flattened. Camera EXIF orientation is applied before resizing; supported
EXIF, ICC and DPI data are retained unless metadata removal is selected. Not every
format-specific metadata field can be preserved. The tools process local files
only, use argument lists rather than shell interpolation, and report individual
errors without stopping the rest of a batch.

## Development and checks

The tools live in `packages/dolphin-tools/`; their package runs the backend and
offscreen Qt tests in `tests/test_dolphin_tools.py`, including real multipage PDF
rendering. They can be built/tested without recompiling Dolphin:

```sh
nix build ./.cammi#dolphin-file-tools
nix build ./.cammi#checks.x86_64-linux.dolphin-file-tools
nix build ./.cammi#checks.x86_64-linux.dolphin-file-menus
```

The menu check exercises KIO's real menu discovery with PDFs, images, folders,
mixed selections and remote URLs in a disposable home. The `dolphin-cache.yml`
workflow builds Dolphin and runs both tool/menu checks before publishing the
signed closure. After publishing the fork, advance the consuming NixOS
configuration's `dolphin` input and rebuild. Reopen Dolphin to load the new build;
the tools do not close any existing file-manager windows.
