#include "theme.h"

#include <QApplication>
#include <QStyleFactory>
#include <QLinearGradient>

namespace Theme {

void applyApplicationStyle(QApplication &app)
{
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    app.setPalette(createDarkPalette());
}

QPalette createDarkPalette()
{
    QPalette palette;
    palette.setColor(QPalette::Window, Colors::BackgroundDark);
    palette.setColor(QPalette::WindowText, Colors::TextPrimary);
    palette.setColor(QPalette::Base, Colors::BackgroundInput);
    palette.setColor(QPalette::AlternateBase, QColor(32, 22, 58));
    palette.setColor(QPalette::ToolTipBase, Colors::TextPrimary);
    palette.setColor(QPalette::ToolTipText, Colors::TextPrimary);
    palette.setColor(QPalette::Text, Colors::TextPrimary);
    palette.setColor(QPalette::Button, Colors::BackgroundButtonDark);
    palette.setColor(QPalette::ButtonText, Colors::TextPrimary);
    palette.setColor(QPalette::BrightText, Colors::Danger);
    palette.setColor(QPalette::Link, Colors::AccentPurple);
    palette.setColor(QPalette::Highlight, Colors::AccentPurpleDark);
    palette.setColor(QPalette::HighlightedText, Colors::TextPrimary);
    return palette;
}

void paintWindowBackground(QPainter *painter, const QRect &rect)
{
    if (!painter) return;
    QLinearGradient grad(0, 0, rect.width(), rect.height());
    grad.setColorAt(0.0, Colors::BackgroundDark);
    grad.setColorAt(1.0, Colors::BackgroundDeep);
    painter->fillRect(rect, grad);
}

QString primaryButtonStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #5c2bb8, stop:1 #8e44ad);"
        "  color: #ffffff;"
        "  border: 1px solid #a259ff;"
        "  border-radius: 8px;"
        "  padding: 10px 22px;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #7539e6, stop:1 #a259ff);"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #471f94, stop:1 #6c2d8a);"
        "}"
    );
}

QString secondaryButtonStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background: #2a1f42;"
        "  color: #bb9af7;"
        "  border: 1px solid #65439c;"
        "  border-radius: 8px;"
        "  padding: 8px 18px;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #3d2a61;"
        "  border: 1px solid #a259ff;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background: #1f1633;"
        "  border: 1px solid #553388;"
        "}"
    );
}

QString successButtonStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1b8a5a, stop:1 #27ae60);"
        "  color: #ffffff;"
        "  border: 1px solid #2ecc71;"
        "  border-radius: 8px;"
        "  padding: 10px 26px;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #23a86f, stop:1 #2ecc71);"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #146944, stop:1 #1e8449);"
        "}"
    );
}

QString dangerButtonStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #b82b4a, stop:1 #e03a60);"
        "  color: #ffffff;"
        "  border: 1px solid #ff5577;"
        "  border-radius: 8px;"
        "  padding: 10px 24px;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #d9385c, stop:1 #ff5577);"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #8c1e36, stop:1 #b32a48);"
        "}"
    );
}

QString menuButtonStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(88, 38, 175, 210), stop:1 rgba(142, 68, 173, 210));"
        "  color: #ffffff;"
        "  border: 2px solid #a259ff;"
        "  border-radius: 12px;"
        "  font-family: 'Segoe UI', 'Arial', sans-serif;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  letter-spacing: 2px;"
        "  text-align: center;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(120, 55, 235, 240), stop:1 rgba(175, 85, 215, 240));"
        "  border: 2px solid #d580ff;"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(65, 25, 135, 250), stop:1 rgba(110, 48, 140, 250));"
        "  border: 2px solid #a259ff;"
        "}"
    );
}

QString menuExitButtonStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(80, 25, 45, 210), stop:1 rgba(120, 35, 65, 210));"
        "  color: #ffffff;"
        "  border: 2px solid #cc4466;"
        "  border-radius: 12px;"
        "  font-family: 'Segoe UI', 'Arial', sans-serif;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  letter-spacing: 2px;"
        "  text-align: center;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(140, 40, 70, 240), stop:1 rgba(190, 50, 90, 240));"
        "  border: 2px solid #ff5577;"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(70, 20, 35, 250), stop:1 rgba(100, 30, 55, 250));"
        "  border: 2px solid #cc4466;"
        "}"
    );
}

QString sectionBoxStyle()
{
    return QStringLiteral(
        "background-color: rgba(30, 22, 50, 180);"
        "border: 1px solid #5a388f;"
        "border-radius: 12px;"
    );
}

QString lineEditStyle()
{
    return QStringLiteral(
        "QLineEdit {"
        "  background: #1c1430;"
        "  color: #ffffff;"
        "  border: 1px solid #7d44cf;"
        "  border-radius: 8px;"
        "  padding: 8px 12px;"
        "  font-size: 14px;"
        "  font-family: 'Segoe UI', sans-serif;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid #a259ff;"
        "}"
    );
}

QString spinBoxStyle()
{
    return QStringLiteral(
        "QSpinBox {"
        "  background: #1c1430;"
        "  color: #ffffff;"
        "  border: 1px solid #7d44cf;"
        "  border-radius: 6px;"
        "  padding: 6px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  min-width: 90px;"
        "}"
    );
}

QString comboBoxStyle()
{
    return QStringLiteral(
        "QComboBox {"
        "  background: #1c1430;"
        "  color: #ffffff;"
        "  border: 1px solid #7d44cf;"
        "  border-radius: 8px;"
        "  padding: 8px 14px;"
        "  font-size: 14px;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  min-width: 250px;"
        "}"
        "QComboBox:hover {"
        "  border: 1px solid #a259ff;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 30px;"
        "  border-left: 1px solid #5a388f;"
        "}"
        "QComboBox::down-arrow {"
        "  width: 0;"
        "  height: 0;"
        "  border-left: 5px solid transparent;"
        "  border-right: 5px solid transparent;"
        "  border-top: 6px solid #bb9af7;"
        "  margin-right: 2px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #1c1430;"
        "  color: #ffffff;"
        "  selection-background-color: #5c2bb8;"
        "  selection-color: #ffffff;"
        "  border: 1px solid #7d44cf;"
        "  border-radius: 6px;"
        "  padding: 4px;"
        "  outline: none;"
        "}"
    );
}

QString sliderStyle()
{
    return QStringLiteral(
        "QSlider::groove:horizontal {"
        "  height: 8px;"
        "  background: #251b3d;"
        "  border-radius: 4px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #7d44cf, stop:1 #a259ff);"
        "  border-radius: 4px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #ffffff;"
        "  border: 2px solid #a259ff;"
        "  width: 18px;"
        "  margin-top: -5px;"
        "  margin-bottom: -5px;"
        "  border-radius: 9px;"
        "}"
    );
}

QString scrollAreaStyle()
{
    return QStringLiteral(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical {"
        "  border: none;"
        "  background: #18112c;"
        "  width: 10px;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #553388;"
        "  min-height: 20px;"
        "  border-radius: 5px;"
        "}"
    );
}

QString exitDialogContainerStyle()
{
    return QStringLiteral(
        "QWidget#dialogContainer {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1a162b, stop:1 #271a3d);"
        "  border: 2px solid #7d44cf;"
        "  border-radius: 16px;"
        "}"
        "QLabel {"
        "  color: #ffffff;"
        "  font-family: 'Segoe UI', sans-serif;"
        "}"
    );
}

} // namespace Theme
