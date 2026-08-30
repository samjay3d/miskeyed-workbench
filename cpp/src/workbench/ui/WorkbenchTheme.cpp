#include <miskeyed/workbench/ui/WorkbenchTheme.h>
#include <QWidget>
namespace miskeyed::workbench::ui {
void WorkbenchTheme::apply(QWidget& root)
{
    root.setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget { background: #16171b; color: #c8ccd4; }
        QToolTip { background: #24262e; color: #e6e6e6; border: 1px solid #3a3d47; padding: 4px; }
        QLabel#PanelHeader, QWidget#PanelHeader {
            background: #22242b; color: #e6e6e6; font-weight: 600;
            padding: 4px 10px; border-bottom: 1px solid #2f323b;
        }
        QLabel#PanelHeaderInline { color: #e6e6e6; font-weight: 600; padding-right: 6px; }
        QLabel#HintLabel { color: #7f8794; padding-left: 12px; }
        QWidget#InspectorPanel { border-left: 1px solid #2f323b; }
        QWidget#InspectorHeader { background: #1d1f25; border-bottom: 1px solid #2f323b; }
        QLabel#InspectorDocument { color: #e6e6e6; font-weight: 600; }
        QLabel#InspectorContext { color: #9aa0ac; }
        QLabel#BindingSummary { color: #aab0bc; padding-left: 8px; }
        #SceneViewport[documentFocus="true"], #PostViewport[documentFocus="true"] {
            border: 2px solid #7c5ce7;
        }
        QWidget#DocumentViewBar {
            background: #22242b; border-top: 1px solid #2f323b;
            border-bottom: 1px solid #2f323b;
        }
        QWidget#Timeline {
            background: #1d1f25; border-top: 1px solid #2f323b;
            border-bottom: 1px solid #2f323b;
        }
        QPlainTextEdit {
            background: #1b1c22; color: #c8ccd4; border: none;
            selection-background-color: #33467c; selection-color: #ffffff;
        }
        QToolBar { background: #1d1f25; border: none; spacing: 4px; padding: 4px; }
        QToolBar QToolButton {
            color: #d5d9e0; padding: 5px 12px; border-radius: 6px; background: transparent;
        }
        QToolBar QToolButton:hover { background: #2c2f38; }
        QToolBar QToolButton:pressed, QToolBar QToolButton:checked { background: #3a5fbf; color: #ffffff; }
        QToolBar::separator { background: #2f323b; width: 1px; margin: 4px 6px; }
        QPushButton {
            background: #2c2f38; color: #d5d9e0; border: 1px solid #3a3d47;
            border-radius: 6px; padding: 3px 12px;
        }
        QPushButton:hover { background: #363a45; border-color: #4a4e5a; }
        QPushButton:pressed { background: #3a5fbf; color: #ffffff; }
        QPushButton#CompileStatus {
            font-weight: 600; padding: 3px 12px; border-radius: 10px; border: 1px solid transparent;
        }
        QPushButton#CompileStatus[state="ok"]        { background: #17321f; color: #9ece6a; border-color: #2c5335; }
        QPushButton#CompileStatus[state="ok"]:hover   { border-color: #9ece6a; }
        QPushButton#CompileStatus[state="warn"]      { background: #33301b; color: #e0af68; border-color: #5a4d2e; }
        QPushButton#CompileStatus[state="warn"]:hover { border-color: #e0af68; }
        QPushButton#CompileStatus[state="error"]     { background: #35191f; color: #f7768e; border-color: #5a2b36; }
        QPushButton#CompileStatus[state="error"]:hover{ border-color: #f7768e; }
        QPushButton#CompileStatus[state="compiling"] { background: #1a2740; color: #7aa2f7; border-color: #2e4370; }
        QPushButton#CompileStatus[state="dirty"]     { background: #2a2c36; color: #c0caf5; border-color: #3a3d47; }
        QPushButton#CompileStatus[state="dirty"]:hover{ border-color: #7aa2f7; }
        QComboBox {
            background: #24262e; color: #e6e6e6; border: 1px solid #3a3d47;
            border-radius: 6px; padding: 3px 26px 3px 10px; min-height: 20px;
        }
        QComboBox:hover { border-color: #4a4e5a; }
        QComboBox::drop-down { border: none; width: 22px; }
        QComboBox QAbstractItemView {
            background: #24262e; color: #e6e6e6; border: 1px solid #3a3d47;
            selection-background-color: #3a5fbf; selection-color: #ffffff; outline: none;
        }
        QTabWidget::pane { border: 1px solid #2f323b; background: #1b1c22; }
        QTabBar::tab {
            background: #1d1f25; color: #9aa0ac; padding: 6px 16px;
            border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px;
        }
        QTabBar::tab:selected { background: #22242b; color: #e6e6e6; border-bottom: 2px solid #7aa2f7; }
        QTabBar::tab:hover:!selected { color: #c8ccd4; }
        QTabWidget#ActiveDocumentInspector QTabBar::tab { padding-left: 8px; padding-right: 8px; }
        QSplitter::handle { background: #0f1013; }
        QSplitter::handle:horizontal { width: 3px; }
        QSplitter::handle:vertical { height: 3px; }
        QSplitter::handle:hover { background: #3a5fbf; }
        QStatusBar { background: #1d1f25; color: #9aa0ac; border-top: 1px solid #2f323b; }
        QScrollBar:vertical { background: #1b1c22; width: 12px; margin: 0; }
        QScrollBar::handle:vertical { background: #353842; min-height: 28px; border-radius: 6px; margin: 2px; }
        QScrollBar::handle:vertical:hover { background: #454956; }
        QScrollBar:horizontal { background: #1b1c22; height: 12px; margin: 0; }
        QScrollBar::handle:horizontal { background: #353842; min-width: 28px; border-radius: 6px; margin: 2px; }
        QScrollBar::handle:horizontal:hover { background: #454956; }
        QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
        QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
    )"));
}
}
