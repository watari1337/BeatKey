#ifndef THEME_H
#define THEME_H

#include <QString>
#include <QColor>
#include <QPalette>
#include <QPainter>
#include <QRect>
#include <QSlider>
#include <QMouseEvent>


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
    }

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
    QString menuButtonStyle();
    QString menuExitButtonStyle();

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

#endif // THEME_H
