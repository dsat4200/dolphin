{ pkgs }:

# Keep the small policy adapter on the pinned upstream build, retaining KIO,
# native undo-close/session state and the distro's Qt wrapping/integration.
pkgs.kdePackages.dolphin.overrideAttrs (old: {
  patches = (old.patches or [ ]) ++ [ ./dolphin-linear-tabs.patch ];
  postPatch = (old.postPatch or "") + ''
    cat ${./dolphin-linear-page.inc} >> src/dolphintabpage.cpp
    cat ${./dolphin-linear-widget.inc} >> src/dolphintabwidget.cpp
    cat ${./dolphin-linear-bar.inc} >> src/dolphintabbar.cpp
    cp ${../tests/dolphin-linear-test.cpp} src/tests/dolphin-linear-test.cpp
    cat >> src/tests/CMakeLists.txt <<'EOF'
    ecm_add_test(dolphin-linear-test.cpp ''${CMAKE_SOURCE_DIR}/src/dolphin.qrc
      TEST_NAME dolphin-linear-test LINK_LIBRARIES dolphinprivate dolphinstatic Qt6::Test)
    # ECM sets a test-specific QT_PLUGIN_PATH, overriding checkPhase's environment.
    # Preserve build plugins and add KIO's file/trash workers before wrapping.
    set_tests_properties(dolphin-linear-test PROPERTIES ENVIRONMENT
      "QT_PLUGIN_PATH=''${CMAKE_BINARY_DIR}/bin:${pkgs.kdePackages.kio}/lib/qt-6/plugins:${pkgs.qt6.qtbase}/lib/qt-6/plugins")
    EOF
  '';
  cmakeFlags = (old.cmakeFlags or [ ]) ++ [ "-DBUILD_TESTING=ON" ];
  nativeCheckInputs = (old.nativeCheckInputs or [ ]) ++ [ pkgs.dbus ];
  doCheck = true;
  checkPhase = ''
    runHook preCheck
    export HOME="$TMPDIR/dolphin-test-home" XDG_RUNTIME_DIR="$TMPDIR/dolphin-test-runtime"
    mkdir -p "$HOME" "$XDG_RUNTIME_DIR"
    chmod 700 "$XDG_RUNTIME_DIR"
    export QT_QPA_PLATFORM=offscreen GIO_USE_VFS=local
    # The check runs before wrapQtAppsHook installs the normal launcher wrapper.
    export QT_PLUGIN_PATH="${pkgs.kdePackages.kio}/lib/qt-6/plugins:${pkgs.qt6.qtbase}/lib/qt-6/plugins"
    dbus-run-session --dbus-daemon=${pkgs.dbus}/bin/dbus-daemon \
      --config-file=${pkgs.dbus}/share/dbus-1/session.conf \
      -- ctest --output-on-failure --timeout 60 -R '^dolphin-linear-test$'
    runHook postCheck
  '';
})
