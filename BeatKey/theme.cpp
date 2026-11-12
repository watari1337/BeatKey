#include "theme.h"

#include <QApplication>
#include <QStyleFactory>
#include <QLinearGradient>
#include <QProxyStyle>
#include <QStyleOptionSlider>
#include <algorithm>
#include <cmath>


namespace Theme {

class BeatKeyProxyStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override
    {
        if (hint == QStyle::SH_Slider_AbsoluteSetButtons) {
            return Qt::LeftButton | Qt::MiddleButton;
        }
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

void applyApplicationStyle(QApplication &app)
{
    QStyle *baseStyle = QStyleFactory::create(QStringLiteral("Fusion"));
    app.setStyle(new BeatKeyProxyStyle(baseStyle));
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
        "  padding: 4px 12px;"
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
        "  padding: 4px 12px;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  font-size: 13px;"
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
        "  padding: 4px 12px;"
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
        "  padding: 4px 12px;"
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

QString easterEggDangerButtonStyle(int step)
{
    step = qBound(1, step, 8);
    int fontSize = qMax(9, 13 - (step - 1));
    int padV = qMax(1, 4 - (step - 1) / 2);
    int padH = qMax(4, 12 - (step - 1));
    return QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #b82b4a, stop:1 #e03a60);"
        "  color: #ffffff;"
        "  border: 1px solid #ff5577;"
        "  border-radius: 8px;"
        "  padding: %1px %2px;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  font-size: %3px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #d9385c, stop:1 #ff5577);"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #8c1e36, stop:1 #b32a48);"
        "}"
    ).arg(padV).arg(padH).arg(fontSize);
}

QString easterEggCancelButtonStyle(int step)
{
    step = qBound(1, step, 8);
    int fontSize = qMin(18, 13 + (step - 1));
    int padV = qMin(10, 4 + (step - 1));
    int padH = qMin(24, 12 + (step - 1) * 2);
    return QStringLiteral(
        "QPushButton {"
        "  background: #2a1b4e;"
        "  color: #cbb2fe;"
        "  border: 1px solid #7d44cf;"
        "  border-radius: 8px;"
        "  padding: %1px %2px;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  font-size: %3px;"
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
    ).arg(padV).arg(padH).arg(fontSize);
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

QString dialogCloseButtonStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background: transparent;"
        "  color: #a09cb0;"
        "  border: none;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "  border-radius: 14px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(255, 255, 255, 0.08);"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background: rgba(255, 255, 255, 0.15);"
        "  color: #bb9af7;"
        "}"
    );
}

QString dialogTitleStyle()
{
    return QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 20px;"
        "font-weight: bold;"
        "color: #ffffff;"
        "background: transparent;"
        "border: none;"
    );
}

QString dialogErrorTitleStyle()
{
    return QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 19px;"
        "font-weight: bold;"
        "color: #ff5577;"
        "background: transparent;"
        "border: none;"
    );
}

QString dialogMessageStyle()
{
    return QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 14px;"
        "color: #e0dced;"
        "line-height: 1.4;"
        "background: transparent;"
        "border: none;"
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
        "  padding: 4px 12px;"
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
        "QSpinBox, QDoubleSpinBox {"
        "  background: #1c1430;"
        "  color: #ffffff;"
        "  border: 1px solid #7d44cf;"
        "  border-radius: 8px;"
        "  padding: 4px 12px;"
        "  font-size: 14px;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  min-width: 80px;"
        "}"
        "QSpinBox:focus, QDoubleSpinBox:focus {"
        "  border: 1px solid #a259ff;"
        "}"
        "QSpinBox::up-button, QSpinBox::down-button, "
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {"
        "  width: 0px;"
        "  height: 0px;"
        "  border: none;"
        "  padding: 0px;"
        "}"
        "QSpinBox::up-arrow, QSpinBox::down-arrow, "
        "QDoubleSpinBox::up-arrow, QDoubleSpinBox::down-arrow {"
        "  width: 0px;"
        "  height: 0px;"
        "  border: none;"
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
        "  padding-right: 32px;"
        "  font-size: 14px;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  min-width: 140px;"
        "}"
        "QComboBox:hover {"
        "  border: 1px solid #a259ff;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: center right;"
        "  width: 26px;"
        "  border: none;"
        "  background: transparent;"
        "}"
        "QComboBox::down-arrow {"
        "  width: 0px;"
        "  height: 0px;"
        "  border-left: 5px solid transparent;"
        "  border-right: 5px solid transparent;"
        "  border-top: 6px solid #bb9af7;"
        "  margin-right: 8px;"
        "}"
        "QComboBox::down-arrow:hover {"
        "  border-top: 6px solid #ffffff;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #1c1430;"
        "  color: #ffffff;"
        "  selection-background-color: #8c42ec;"
        "  selection-color: #ffffff;"
        "  border: 1px solid #a259ff;"
        "  border-radius: 6px;"
        "  padding: 4px;"
        "  outline: none;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "  padding: 4px 12px;"
        "  border-radius: 4px;"
        "}"
        "QComboBox QAbstractItemView::item:hover,"
        "QComboBox QAbstractItemView::item:selected {"
        "  background: #8c42ec;"
        "  color: #ffffff;"
        "}"
    );
}

QString sliderStyle()
{
    return QStringLiteral(
        "QSlider:horizontal {"
        "  min-height: 26px;"
        "}"
        "QSlider::groove:horizontal {"
        "  height: 6px;"
        "  background: #251b3d;"
        "  border-radius: 3px;"
        "  margin-left: 2px;"
        "  margin-right: 2px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #7d44cf, stop:1 #a259ff);"
        "  border-radius: 3px;"
        "  margin-left: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #ffffff;"
        "  border: 2px solid #a259ff;"
        "  width: 14px;"
        "  margin-top: -4px;"
        "  margin-bottom: -4px;"
        "  border-radius: 7px;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "  background: #f5eeff;"
        "  border: 2px solid #bb9af7;"
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


QString dialogContainerStyle()
{
    return QStringLiteral(
        "QWidget#dialogContainer {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #18112c, stop:1 #24143e);"
        "  border: 2px solid #7d44cf;"
        "  border-radius: 16px;"
        "}"
    );
}

QString editorCardStyle()
{
    return QStringLiteral(
        "QFrame {"
        "  background: rgba(26, 16, 44, 175);"
        "  border: 1px solid rgba(125, 68, 207, 180);"
        "  border-radius: 12px;"
        "}"
        "QLabel {"
        "  background: transparent;"
        "  border: none;"
        "}"
    );
}

QString errorLineEditStyle()
{
    return QStringLiteral(
        "QLineEdit {"
        "  background: #251622;"
        "  color: #ffffff;"
        "  border: 2px solid #d9536f;"
        "  border-radius: 8px;"
        "  padding: 4px 12px;"
        "  font-size: 14px;"
        "  font-family: 'Segoe UI', sans-serif;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #e57388;"
        "}"
    );
}

QString errorButtonStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background: #251622;"
        "  color: #e57388;"
        "  border: 2px solid #d9536f;"
        "  border-radius: 8px;"
        "  padding: 8px 14px;"
        "  font-size: 13px;"
        "  font-family: 'Segoe UI', sans-serif;"
        "}"
        "QPushButton:hover {"
        "  background: #331c2c;"
        "  border: 2px solid #e57388;"
        "}"
    );
}

QString toolButtonStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background: #281c44;"
        "  color: #bb9af7;"
        "  border: 1px solid #6b40b0;"
        "  border-radius: 8px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #3d2866;"
        "  color: #ffffff;"
        "  border: 1px solid #a259ff;"
        "}"
        "QPushButton:checked {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #7d44cf, stop:1 #a259ff);"
        "  color: #ffffff;"
        "  border: 2px solid #d580ff;"
        "}"
    );
}

QString tabButtonStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background: rgba(30, 20, 50, 170);"
        "  color: #a09cb0;"
        "  border: 1px solid rgba(125, 68, 207, 120);"
        "  border-radius: 8px;"
        "  padding: 6px 16px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  font-family: 'Segoe UI', sans-serif;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(56, 36, 92, 220);"
        "  color: #ffffff;"
        "  border: 1px solid #a259ff;"
        "}"
        "QPushButton:checked {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #7d44cf, stop:1 #a259ff);"
        "  color: #ffffff;"
        "  border: 1px solid #d580ff;"
        "}"
    );
}

QString cardHeaderStyle()
{
    return QStringLiteral(
        "color: #bb9af7;"
        "font-size: 12px;"
        "font-weight: bold;"
        "font-family: 'Segoe UI', sans-serif;"
        "background: transparent;"
        "border: none;"
    );
}

QString secondaryLabelStyle()
{
    return QStringLiteral(
        "color: #a09cb0;"
        "font-size: 11px;"
        "font-family: 'Segoe UI', sans-serif;"
        "background: transparent;"
        "border: none;"
    );
}

QString accentLabelStyle()
{
    return QStringLiteral(
        "color: #2ecc71;"
        "font-size: 11px;"
        "font-weight: bold;"
        "font-family: 'Segoe UI', sans-serif;"
        "background: transparent;"
        "border: none;"
    );
}

QString failDialogContainerStyle()
{
    return QStringLiteral(
        "#failDialogContainer {"
        "  background: rgba(26, 12, 24, 245);"
        "  border: 2px solid rgba(217, 83, 111, 220);"
        "  border-radius: 16px;"
        "}"
    );
}

QString failDialogTitleStyle()
{
    return QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 26px;"
        "font-weight: 900;"
        "color: #d9536f;"
        "background: transparent;"
        "border: none;"
    );
}

QString failStatBoxStyle()
{
    return QStringLiteral(
        "background: rgba(42, 20, 36, 180);"
        "border-radius: 8px;"
        "border: 1px solid rgba(217, 83, 111, 90);"
    );
}

QString failStatValueStyle()
{
    return QStringLiteral(
        "font-size: 16px;"
        "font-weight: bold;"
        "color: #ffffff;"
        "background: transparent;"
        "border: none;"
    );
}

QString failStatTitleStyle()
{
    return QStringLiteral(
        "font-size: 11px;"
        "color: #a09cb0;"
        "background: transparent;"
        "border: none;"
    );
}

QString pauseCenterCardStyle()
{
    return QStringLiteral(
        "#pauseCenterCard {"
        "  background: rgba(18, 12, 34, 240);"
        "  border: 1.5px solid rgba(125, 68, 207, 210);"
        "  border-radius: 16px;"
        "}"
    );
}

QString pauseGlassCardStyle()
{
    return QStringLiteral(
        "#pauseGlassCard {"
        "  background: rgba(18, 12, 34, 115);"
        "  border: 1px solid rgba(125, 68, 207, 100);"
        "  border-radius: 14px;"
        "}"
    );
}

QString checkBoxStyle()
{
    return QStringLiteral(
        "QCheckBox {"
        "  color: #dddddd;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  font-size: 12px;"
        "  spacing: 8px;"
        "  background: transparent;"
        "}"
        "QCheckBox:hover {"
        "  color: #ffffff;"
        "}"
        "QCheckBox::indicator {"
        "  width: 16px;"
        "  height: 16px;"
        "  border: 1.5px solid #7d44cf;"
        "  border-radius: 4px;"
        "  background: #1c1430;"
        "}"
        "QCheckBox::indicator:hover {"
        "  border-color: #a259ff;"
        "  background: #251b3d;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background: #a259ff;"
        "  border-color: #bb9af7;"
        "}"
    );
}

} // namespace Theme

// ============================================================================
// ClickableSlider Implementation
// ============================================================================

ClickableSlider::ClickableSlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
{
    setMinimumHeight(26);
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(Theme::sliderStyle());
}

ClickableSlider::ClickableSlider(QWidget *parent)
    : QSlider(parent)
{
    setMinimumHeight(26);
    setCursor(Qt::PointingHandCursor);
    setStyleSheet(Theme::sliderStyle());
}

void ClickableSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        QRect handleRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

        if (!handleRect.contains(event->position().toPoint())) {
            int minVal = minimum();
            int maxVal = maximum();
            int newVal = minVal;

            if (orientation() == Qt::Horizontal) {
                int halfHandle = handleRect.width() > 0 ? (handleRect.width() / 2) : 7;
                int trackWidth = width() - (halfHandle * 2);
                if (trackWidth > 0) {
                    int clickX = std::clamp(static_cast<int>(event->position().x()) - halfHandle, 0, trackWidth);
                    double ratio = static_cast<double>(clickX) / trackWidth;
                    if (invertedAppearance()) {
                        ratio = 1.0 - ratio;
                    }
                    newVal = minVal + static_cast<int>(std::round(ratio * (maxVal - minVal)));
                }
            } else {
                int halfHandle = handleRect.height() > 0 ? (handleRect.height() / 2) : 7;
                int trackHeight = height() - (halfHandle * 2);
                if (trackHeight > 0) {
                    int clickY = std::clamp(static_cast<int>(event->position().y()) - halfHandle, 0, trackHeight);
                    double ratio = static_cast<double>(clickY) / trackHeight;
                    if (!invertedAppearance()) {
                        ratio = 1.0 - ratio;
                    }
                    newVal = minVal + static_cast<int>(std::round(ratio * (maxVal - minVal)));
                }
            }

            setValue(newVal);
            setSliderPosition(newVal);
            emit sliderMoved(newVal);
        }
    }
    QSlider::mousePressEvent(event);
}

// ============================================================================
// MarqueeLabel Implementation
// ============================================================================

MarqueeLabel::MarqueeLabel(QWidget *parent)
    : MarqueeLabel(QString(), parent)
{
}

MarqueeLabel::MarqueeLabel(const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_text(text)
    , m_textColor(Theme::Colors::TextPrimary)
    , m_font(QFont(QStringLiteral("Segoe UI"), 13, QFont::DemiBold))
    , m_active(false)
    , m_scrollX(0.0)
    , m_pauseTicks(35)
    , m_timer(new QTimer(this))
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setToolTip(m_text);

    m_timer->setInterval(25);
    connect(m_timer, &QTimer::timeout, this, [this]() { onTick(); });
}

void MarqueeLabel::setText(const QString &text)
{
    m_text = text;
    m_scrollX = 0.0;
    m_pauseTicks = 35;
    setToolTip(m_text);
    if (m_active) {
        QFontMetrics fm(m_font);
        if (fm.horizontalAdvance(m_text) > width()) {
            m_timer->start();
        } else {
            m_timer->stop();
        }
    }
    update();
}

void MarqueeLabel::setTextColor(const QColor &color)
{
    m_textColor = color;
    update();
}

void MarqueeLabel::setLabelFont(const QFont &font)
{
    m_font = font;
    update();
}

void MarqueeLabel::setActive(bool active)
{
    if (m_active == active) return;
    m_active = active;
    m_scrollX = 0.0;
    m_pauseTicks = 35;
    if (m_active) {
        QFontMetrics fm(m_font);
        if (fm.horizontalAdvance(m_text) > width() && width() > 10) {
            m_timer->start();
        }
    } else {
        m_timer->stop();
    }
    update();
}

QSize MarqueeLabel::sizeHint() const
{
    QFontMetrics fm(m_font);
    return QSize(fm.horizontalAdvance(m_text), fm.height());
}

void MarqueeLabel::onTick()
{
    if (!m_active) {
        m_timer->stop();
        return;
    }
    QFontMetrics fm(m_font);
    int textWidth = fm.horizontalAdvance(m_text);
    if (textWidth <= width()) {
        m_scrollX = 0.0;
        m_timer->stop();
        update();
        return;
    }

    if (m_pauseTicks > 0) {
        m_pauseTicks--;
        return;
    }

    m_scrollX += 1.0;
    int gap = 50;
    int totalLoop = textWidth + gap;
    if (m_scrollX >= totalLoop) {
        m_scrollX = 0.0;
        m_pauseTicks = 30;
    }
    update();
}

void MarqueeLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setFont(m_font);
    painter.setPen(m_textColor);

    QFontMetrics fm(m_font);
    int textWidth = fm.horizontalAdvance(m_text);
    int textHeight = fm.height();
    int y = (height() - textHeight) / 2 + fm.ascent();

    if (!m_active || textWidth <= width()) {
        QString elided = fm.elidedText(m_text, Qt::ElideRight, width());
        painter.drawText(0, y, elided);
        return;
    }

    painter.setClipRect(rect());
    int gap = 50;
    int totalLoop = textWidth + gap;

    double drawX = -m_scrollX;
    painter.drawText(QPointF(drawX, y), m_text);
    painter.drawText(QPointF(drawX + totalLoop, y), m_text);
}
