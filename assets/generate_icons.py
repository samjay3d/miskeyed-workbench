"""One-off icon exporter: copies the source SVG into the package and renders
PNG sizes + a multi-size .ico into assets/. Safe to re-run."""

from __future__ import annotations

import shutil
from pathlib import Path

from PySide6.QtCore import QSize
from PySide6.QtGui import QGuiApplication, QImage, QPainter
from PySide6.QtSvg import QSvgRenderer

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "assets" / "workbench.svg"
PKG_ASSETS = ROOT / "python" / "miskeyed" / "workbench" / "assets"
ICONS = ROOT / "assets" / "icons"
SIZES = [16, 24, 32, 48, 64, 128, 256]


def render(size: int) -> QImage:
    renderer = QSvgRenderer(str(SRC))
    img = QImage(QSize(size, size), QImage.Format_ARGB32)
    img.fill(0)
    painter = QPainter(img)
    renderer.render(painter)
    painter.end()
    return img


def main() -> None:
    QGuiApplication([])

    PKG_ASSETS.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(SRC, PKG_ASSETS / "workbench.svg")

    ICONS.mkdir(parents=True, exist_ok=True)
    images = {}
    for size in SIZES:
        img = render(size)
        images[size] = img
        img.save(str(ICONS / f"workbench-{size}.png"), "PNG")

    ico = ROOT / "assets" / "workbench.ico"
    try:
        from PIL import Image  # multi-size .ico

        base = ICONS / "workbench-256.png"
        Image.open(base).save(ico, sizes=[(s, s) for s in SIZES])
        print("ico: multi-size via Pillow")
    except Exception:
        images[256].save(str(ico), "ICO")
        print("ico: single-size via Qt")

    print("done:", sorted(images))


if __name__ == "__main__":
    main()
