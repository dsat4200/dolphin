"""Small Qt dialogs; all actual file operations live in dolphin_tools."""

import sys

from PySide6.QtCore import Qt, QThread, Signal
from PySide6.QtWidgets import (
    QApplication,
    QCheckBox,
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QFormLayout,
    QLabel,
    QMessageBox,
    QProgressDialog,
    QSpinBox,
    QVBoxLayout,
)

from dolphin_tools import PRESETS, ResizeOptions, extract_pdf, resize_image


class ResizeDialog(QDialog):
    def __init__(self, count):
        super().__init__()
        self.setWindowTitle("Resize Pictures")
        layout = QVBoxLayout(self)
        layout.addWidget(QLabel(f"Resize {count} picture(s). Originals are always kept."))
        form = QFormLayout()
        layout.addLayout(form)
        self.preset = QComboBox()
        for name, (width, height) in PRESETS.items():
            self.preset.addItem(f"{name} ({width} × {height})", name)
        self.preset.addItem("Custom", "Custom")
        form.addRow("Size:", self.preset)
        self.units = QComboBox()
        self.units.addItems(["Pixels", "Percent"])
        form.addRow("Units:", self.units)
        self.width = QSpinBox()
        self.height = QSpinBox()
        for spin in (self.width, self.height):
            spin.setRange(1, 50000)
        form.addRow("Width:", self.width)
        form.addRow("Height:", self.height)
        self.mode = QComboBox()
        self.mode.addItems(["Fit", "Fill", "Stretch"])
        self.mode.setToolTip("Fit: keep proportions. Fill: crop to size. Stretch: exact dimensions.")
        form.addRow("Mode:", self.mode)
        self.quality = QSpinBox()
        self.quality.setRange(1, 100)
        self.quality.setValue(90)
        form.addRow("JPEG / WebP / AVIF quality:", self.quality)
        self.shrink = QCheckBox("Only make pictures smaller")
        self.shrink.setChecked(True)
        self.orientation = QCheckBox("Match each picture's landscape / portrait orientation")
        self.orientation.setChecked(True)
        self.strip = QCheckBox("Remove metadata from resized copies")
        for widget in (self.shrink, self.orientation, self.strip):
            layout.addWidget(widget)
        note = QLabel("Copies are saved alongside each original in the same format.\n"
                      "Existing files are never overwritten. Animated / multipage images are skipped.")
        note.setWordWrap(True)
        layout.addWidget(note)
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        buttons.button(QDialogButtonBox.Ok).setText("Resize")
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)
        self.preset.currentIndexChanged.connect(self.choose_preset)
        self.units.currentIndexChanged.connect(self.choose_units)
        self.choose_preset()
        self.preset.setCurrentIndex(1)

    def choose_preset(self):
        name = self.preset.currentData()
        custom = name == "Custom"
        for widget in (self.width, self.height, self.units):
            widget.setEnabled(custom)
        if not custom:
            self.units.setCurrentIndex(0)
            self.width.setValue(PRESETS[name][0])
            self.height.setValue(PRESETS[name][1])

    def choose_units(self):
        percent = self.units.currentIndex() == 1
        self.mode.setEnabled(not percent)
        self.orientation.setEnabled(not percent)
        if percent:
            self.width.setValue(50)
            self.height.setValue(50)

    def options(self):
        return ResizeOptions(
            width=self.width.value(),
            height=self.height.value(),
            mode=self.mode.currentText(),
            percent=self.units.currentIndex() == 1,
            shrink_only=self.shrink.isChecked(),
            match_orientation=self.orientation.isChecked(),
            label=self.preset.currentData(),
            quality=self.quality.value(),
            strip_metadata=self.strip.isChecked(),
        )


class Worker(QThread):
    current = Signal(int, str)
    results = Signal(list, list)

    def __init__(self, paths, operation):
        super().__init__()
        self.paths, self.operation = paths, operation

    def run(self):
        successes, failures = [], []
        for index, path in enumerate(self.paths):
            self.current.emit(index, path.name)
            try:
                successes.append(str(self.operation(path)))
            except Exception as error:
                failures.append(f"{path}: {error}")
        self.results.emit(successes, failures)


class ProgressDialog(QProgressDialog):
    def reject(self):
        # Closing the dialog must not terminate a worker in the middle of a write.
        pass


def process(paths, operation, title):
    progress = ProgressDialog("Starting…", "", 0, 0)
    progress.setWindowTitle(title)
    progress.setCancelButton(None)
    progress.setWindowFlag(Qt.WindowCloseButtonHint, False)
    progress.setMinimumWidth(450)
    worker = Worker(paths, operation)
    result = ([], [])

    def finish(successes, failures):
        nonlocal result
        result = successes, failures
        progress.accept()

    worker.current.connect(lambda index, name: progress.setLabelText(
        f"Processing {index + 1} of {len(paths)}: {name}"
    ))
    worker.results.connect(finish)
    # Start after the dialog's event loop is running, even for very fast failures.
    from PySide6.QtCore import QTimer

    QTimer.singleShot(0, worker.start)
    progress.exec()
    worker.wait()
    successes, failures = result
    box = QMessageBox()
    box.setWindowTitle(title)
    box.setIcon(QMessageBox.Warning if failures else QMessageBox.Information)
    box.setTextFormat(Qt.PlainText)
    box.setText(f"Created {len(successes)} output(s)." +
                (f" {len(failures)} file(s) could not be processed." if failures else ""))
    box.setInformativeText("Original files were kept. See details for output paths and any errors.")
    box.setDetailedText("\n".join(successes + failures))
    box.exec()
    return 1 if failures else 0


def run(action, paths):
    app = QApplication([sys.argv[0]])
    app.setApplicationName("Dolphin File Tools")
    app.setDesktopFileName("org.kde.dolphin")
    if action == "resize":
        dialog = ResizeDialog(len(paths))
        if dialog.exec() != QDialog.Accepted:
            return 0
        options = dialog.options()
        return process(paths, lambda path: resize_image(path, options), "Resize Pictures")
    return process(paths, extract_pdf, "Extract Pages to Images")
