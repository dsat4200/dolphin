{ pkgs }:

let
  python = pkgs.python3.withPackages (ps: [ ps.pillow ps.pyside6 ]);
  selectedKRename = pkgs.krename.overrideAttrs (old: {
    # KRename normally expands positional folders into their contents. A
    # selection-based rename menu must rename the selected folders themselves;
    # its explicit recursive -r option and in-app file picker stay unchanged.
    postPatch = (old.postPatch or "") + ''
      substituteInPlace src/krenameimpl.cpp \
        --replace-fail 'this->addFilesOrDirs(list);' \
          'for (const QUrl &url : list) { this->addFileOrDir(url); }'
    '';
  });
in
pkgs.stdenv.mkDerivation {
  pname = "dolphin-file-tools";
  version = "1.0";
  src = ./dolphin-tools;
  nativeBuildInputs = [ pkgs.makeWrapper pkgs.kdePackages.wrapQtAppsHook ];
  buildInputs = [ pkgs.kdePackages.qtbase ];
  dontConfigure = true;
  dontBuild = true;
  dontWrapQtApps = true;
  installPhase = ''
    runHook preInstall
    mkdir -p "$out/bin" "$out/share/dolphin-file-tools" "$out/share/kio/servicemenus"
    cp *.py "$out/share/dolphin-file-tools/"
    substituteInPlace "$out/share/dolphin-file-tools/dolphin_tools.py" \
      --replace-fail '@pdftoppm@' '${pkgs.poppler-utils}/bin/pdftoppm' \
      --replace-fail '@krename@' '${selectedKRename}/bin/krename'
    makeWrapper ${python}/bin/python3 "$out/bin/dolphin-file-tools" \
      --add-flags "$out/share/dolphin-file-tools/dolphin_tools.py" \
      "''${qtWrapperArgs[@]}"
    for menu in *.desktop; do
      substitute "$menu" "$out/share/kio/servicemenus/dolphin-$menu" \
        --replace-fail '@tools@' "$out/bin/dolphin-file-tools"
    done
    runHook postInstall
  '';
  doInstallCheck = true;
  nativeInstallCheckInputs = [ python pkgs.qpdf ];
  installCheckPhase = ''
    runHook preInstallCheck
    export QT_QPA_PLATFORM=offscreen
    export HOME="$TMPDIR/home" XDG_RUNTIME_DIR="$TMPDIR/runtime"
    mkdir -p "$HOME" "$XDG_RUNTIME_DIR"
    chmod 700 "$XDG_RUNTIME_DIR"
    export PYTHONPATH="$out/share/dolphin-file-tools"
    export PYTHONDONTWRITEBYTECODE=1
    export DOLPHIN_FILE_TOOLS="$out"
    ${python}/bin/python3 ${../tests/test_dolphin_tools.py}
    "$out/bin/dolphin-file-tools" --help >/dev/null
    runHook postInstallCheck
  '';
  meta = {
    description = "PDF page extraction, KRename and batch image resizing for Dolphin";
    mainProgram = "dolphin-file-tools";
    license = pkgs.lib.licenses.gpl2Plus;
    platforms = [ "x86_64-linux" ];
  };
}
