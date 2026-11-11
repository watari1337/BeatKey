#ifndef THEME_H
#define THEME_H

#include <QString>
#include <QColor>
#include <QPalette>
#include <QPainter>
#include <QRect>
#include <QSlider>
#include <QMouseEvent>
#include <QTimer>


class QApplication;

/**
 * @brief Theme centralizes all styling constants, colors, and QSS rules for BeatKey.
 *
 * Provides a single source of truth for the application's visual identity,
 * preventing style fragmentation across screens and ensuring seamless future maintenance.
 */
namespace Theme {

    // ========================================================================
    // Color Palette
    // ========================================================================

namespace Colors {
    inline const QColor BackgroundDark{18, 12, 34};       // #120c22
    inline const QColor BackgroundDeep{28, 16, 52};       // #1c1034
    inline const QColor BackgroundCard{30, 22, 50, 180};  // rgba(30, 22, 50, 180)
    inline const QColor BackgroundInput{28, 20, 48};      // #1c1430
    inline const QColor BackgroundButtonDark{42, 31, 66}; // #2a1f42

    inline const QColor AccentPurple{162, 89, 255};       // #a259ff
    inline const QColor AccentPurpleDark{125, 68, 207};   // #7d44cf
    inline const QColor AccentPurpleDeep{92, 43, 184};    // #5c2bb8
    inline const QColor AccentPurpleLight{187, 154, 247}; // #bb9af7

    inline const QColor TextPrimary{255, 255, 255};       // #ffffff
    inline const QColor TextSecondary{221, 221, 221};     // #dddddd
    inline const QColor TextMuted{160, 156, 176};         // #a09cb0
    inline const QColor TextSubtle{112, 106, 132};        // #706a84

    inline const QColor BorderPrimary{125, 68, 207};      // #7d44cf
    inline const QColor BorderSubtle{90, 56, 143};        // #5a388f
    inline const QColor BorderFocused{162, 89, 255};      // #a259ff

    inline const QColor Danger{217, 83, 111};             // #d9536f (soft rose)
    inline const QColor DangerDark{170, 50, 75};          // #b82b4a
    inline const QColor Success{46, 204, 113};            // #2ecc71
    inline const QColor SuccessDark{27, 138, 90};         // #1b8a5a
    inline const QColor Warning{255, 170, 0};             // #ffaa00
    inline const QColor NoteCyan{0, 229, 255};            // #00e5ff
    inline const QColor NoteCyanDark{0, 140, 255};        // #008cff
    inline const QColor HitLineColor{187, 154, 247, 180}; // rgba(187, 154, 247, 180)
    inline const QColor TrackProgressBarBg{42, 31, 66, 180}; // rgba(42, 31, 66, 180)
    inline const QColor TrackLineWhite{255, 255, 255, 200}; // Белая горизонтальная полоска от края до края
    inline const QColor HitTargetBorder{255, 255, 255, 230}; // Обводка полого прямоугольника зоны нажатия
    inline const QColor PauseOverlayDim{10, 6, 20, 160}; // Затемнение экрана паузы
    inline const QColor CountdownOverlayDim{16, 10, 28, 80}; // Полупрозрачное затемнение при отсчете
    inline const QColor CountdownBadgeBg{26, 16, 48, 120}; // Полупрозрачный фон бейджа отсчета
    inline const QColor CountdownBadgeBorder{187, 154, 247, 180}; // Неоновый контур бейджа
}

    /*namespace Colors {
        inline const QColor BackgroundDark{0,0,0};       // #120c22
        inline const QColor BackgroundDeep{20, 0, 70};       // #1c1034
        inline const QColor BackgroundCard{20, 0, 40, 180};  // rgba(30, 22, 50, 180)
        inline const QColor BackgroundInput{0, 0, 10};      // #1c1430
        inline const QColor BackgroundButtonDark{20, 0, 70}; // #2a1f42

        inline const QColor AccentPurple{100, 0, 255};       // #a259ff
        inline const QColor AccentPurpleDark{20, 0, 150};   // #7d44cf
        inline const QColor AccentPurpleDeep{50, 0, 200};    // #5c2bb8
        inline const QColor AccentPurpleLight{200, 154, 255}; // #bb9af7

        inline const QColor TextPrimary{255, 255, 255};       // #ffffff
        inline const QColor TextSecondary{221, 221, 221};     // #dddddd
        inline const QColor TextMuted{160, 100, 200};         // #a09cb0
        inline const QColor TextSubtle{100, 70, 160};        // #706a84

        inline const QColor BorderPrimary{100, 0, 255};      // #7d44cf
        inline const QColor BorderSubtle{50, 0, 200};        // #5a388f
        inline const QColor BorderFocused{255, 255, 255};      // #a259ff

        inline const QColor Danger{255, 0, 50};             // #d9536f (soft rose)
        inline const QColor DangerDark{170, 0, 20};          // #b82b4a
        inline const QColor Success{0, 255, 60};            // #2ecc71
        inline const QColor SuccessDark{0, 150, 30};         // #1b8a5a
        inline const QColor Warning{255, 255, 0};             // #ffaa00
    }

    //
    // namespace Colors {
        inline const QColor BackgroundDark{18, 12, 34};       // #120c22
        inline const QColor BackgroundDeep{28, 16, 52};       // #1c1034
        inline const QColor BackgroundCard{30, 22, 50, 180};  // rgba(30, 22, 50, 180)
        inline const QColor BackgroundInput{28, 20, 48};      // #1c1430
        inline const QColor BackgroundButtonDark{42, 31, 66}; // #2a1f42

        inline const QColor AccentPurple{162, 89, 255};       // #a259ff
        inline const QColor AccentPurpleDark{125, 68, 207};   // #7d44cf
        inline const QColor AccentPurpleDeep{92, 43, 184};    // #5c2bb8
        inline const QColor AccentPurpleLight{187, 154, 247}; // #bb9af7

        inline const QColor TextPrimary{255, 255, 255};       // #ffffff
        inline const QColor TextSecondary{221, 221, 221};     // #dddddd
        inline const QColor TextMuted{160, 156, 176};         // #a09cb0
        inline const QColor TextSubtle{112, 106, 132};        // #706a84

        inline const QColor BorderPrimary{125, 68, 207};      // #7d44cf
        inline const QColor BorderSubtle{90, 56, 143};        // #5a388f
        inline const QColor BorderFocused{162, 89, 255};      // #a259ff

        inline const QColor Danger{217, 83, 111};             // #d9536f (soft rose)
        inline const QColor DangerDark{170, 50, 75};          // #b82b4a
        inline const QColor Success{46, 204, 113};            // #2ecc71
        inline const QColor SuccessDark{27, 138, 90};         // #1b8a5a
        inline const QColor Warning{255, 170, 0};             // #ffaa00
    }*/

    // ========================================================================
    // Application Initialization
    // ========================================================================
    void applyApplicationStyle(QApplication &app);
    QPalette createDarkPalette();

    // ========================================================================
    // Painting Helpers
    // ========================================================================
    void paintWindowBackground(QPainter *painter, const QRect &rect);

    // ========================================================================
    // Component Stylesheets (QSS)
    // ========================================================================
    QString primaryButtonStyle();
    QString secondaryButtonStyle();
    QString successButtonStyle();
    QString dangerButtonStyle();
    QString easterEggDangerButtonStyle(int step);
    QString easterEggCancelButtonStyle(int step);
    QString menuButtonStyle();
    QString menuExitButtonStyle();
    QString dialogCloseButtonStyle();
    QString dialogTitleStyle();
    QString dialogErrorTitleStyle();
    QString dialogMessageStyle();

    QString sectionBoxStyle();
    QString lineEditStyle();
    QString spinBoxStyle();
    QString comboBoxStyle();
    QString sliderStyle();
    QString scrollAreaStyle();
    QString exitDialogContainerStyle();

    QString dialogContainerStyle();
    QString editorCardStyle();
    QString errorLineEditStyle();
    QString errorButtonStyle();
    QString toolButtonStyle();
    QString tabButtonStyle();
    QString cardHeaderStyle();
    QString secondaryLabelStyle();
    QString accentLabelStyle();
    QString failDialogContainerStyle();
    QString failDialogTitleStyle();
    QString failStatBoxStyle();
    QString failStatValueStyle();
    QString failStatTitleStyle();
    QString pauseCenterCardStyle();
    QString pauseGlassCardStyle();
}

/**
 * @brief ClickableSlider is a QSlider that jumps directly to the mouse click position
 * and supports smooth continuous dragging without stepping.
 */
class ClickableSlider : public QSlider
{
public:
    explicit ClickableSlider(Qt::Orientation orientation, QWidget *parent = nullptr);
    explicit ClickableSlider(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
};

/**
 * @brief MarqueeLabel is a widget that displays text and smoothly scrolls horizontally
 * like a ticker/marquee when active and when the text exceeds the widget's visible width.
 */
class MarqueeLabel : public QWidget
{
public:
    explicit MarqueeLabel(QWidget *parent = nullptr);
    explicit MarqueeLabel(const QString &text, QWidget *parent = nullptr);

    void setText(const QString &text);
    QString text() const { return m_text; }

    void setTextColor(const QColor &color);
    void setLabelFont(const QFont &font);

    void setActive(bool active);
    bool isActive() const { return m_active; }

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void onTick();

    QString m_text;
    QColor m_textColor;
    QFont m_font;
    bool m_active = false;
    double m_scrollX = 0.0;
    int m_pauseTicks = 0;
    QTimer *m_timer = nullptr;
};

#endif // THEME_H
