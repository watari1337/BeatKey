#include "calibrationdialog.h"
#include "settingsmanager.h"
#include "audiomanager.h"
#include "savepromptdialog.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QRandomGenerator>
#include <QDir>
#include <QFile>
#include <QUrl>
#include <cmath>
#include <algorithm>

#pragma pack(push, 1)
struct WavPcmHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    quint32 chunkSize = 0;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    quint32 subchunk1Size = 16;
    quint16 audioFormat = 1; // PCM
    quint16 numChannels = 1; // Mono
    quint32 sampleRate = 44100;
    quint32 byteRate = 44100 * 2;
    quint16 blockAlign = 2;
    quint16 bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    quint32 subchunk2Size = 0;
};
#pragma pack(pop)

// ============================================================================
// CalibrationSquareWidget Implementation
// Отрисовывается прямо внутри контейнера диалога, исключая проблемы перекрытия слоев
// ============================================================================
class CalibrationSquareWidget : public QWidget
{
public:
    explicit CalibrationSquareWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedHeight(150);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

    void setState(qreal flashRatio, int currentBeatIdx)
    {
        m_flashRatio = flashRatio;
        m_currentBeatIdx = currentBeatIdx;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QPointF center(width() / 2.0, 58.0);
        const qreal boxSize = 92.0;
        QRectF boxRect(center.x() - boxSize / 2.0, center.y() - boxSize / 2.0, boxSize, boxSize);

        // Интерполяция цвета от темного фиолетового до яркого неонового циана
        int r = static_cast<int>(34 + (0 - 34) * m_flashRatio);
        int g = static_cast<int>(22 + (229 - 22) * m_flashRatio);
        int b = static_cast<int>(59 + (255 - 59) * m_flashRatio);
        QColor fillColor(r, g, b);

        // Свечение при вспышке
        if (m_flashRatio > 0.05) {
            QColor glowColor(0, 229, 255, static_cast<int>(130 * m_flashRatio));
            painter.setPen(Qt::NoPen);
            painter.setBrush(glowColor);
            painter.drawRoundedRect(boxRect.adjusted(-8, -8, 8, 8), 16, 16);
        }

        QColor borderColor = (m_flashRatio > 0.1)
            ? QColor(255, 255, 255)
            : QColor(107, 64, 176);
        painter.setPen(QPen(borderColor, 2.5));
        painter.setBrush(fillColor);
        painter.drawRoundedRect(boxRect, 12, 12);

        // Ромбик в центре квадрата
        QPolygonF diamond;
        diamond << QPointF(center.x(), center.y() - 14)
                << QPointF(center.x() + 14, center.y())
                << QPointF(center.x(), center.y() + 14)
                << QPointF(center.x() - 14, center.y());
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_flashRatio > 0.1 ? QColor(255, 255, 255) : QColor(125, 68, 207));
        painter.drawPolygon(diamond);

        // Индикаторы 4 долей бита под квадратом
        qreal dotY = boxRect.bottom() + 18;
        for (int i = 0; i < 4; ++i) {
            qreal dotX = center.x() - 36 + i * 24;
            bool isActive = (i == m_currentBeatIdx);
            painter.setPen(Qt::NoPen);
            painter.setBrush(isActive ? QColor(0, 229, 255) : QColor(64, 45, 96));
            painter.drawEllipse(QPointF(dotX, dotY), isActive ? 5.0 : 3.5, isActive ? 5.0 : 3.5);
        }
    }

private:
    qreal m_flashRatio = 0.0;
    int m_currentBeatIdx = 0;
};

// ============================================================================
// CalibrationDialog Implementation
// ============================================================================

CalibrationDialog::CalibrationDialog(QWidget *parent)
    : QDialog(parent)
    , m_initialOffsetMs(SettingsManager::instance().audioOffsetMs())
    , m_currentOffsetMs(m_initialOffsetMs)
    , m_player(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_syncTimer(new QTimer(this))
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);

    setupUi();

    // Настройка аудиоплеера
    m_player->setAudioOutput(m_audioOutput);
    m_player->setLoops(QMediaPlayer::Infinite);
    m_audioOutput->setVolume(qBound(0.0, SettingsManager::instance().musicVolume() / 100.0, 1.0));

    QString beatPath = generate8BitBeatFile();
    if (QFile::exists(beatPath)) {
        m_player->setSource(QUrl::fromLocalFile(beatPath));
        m_player->play();
        m_audioClock.restart();
    }

    // 60 FPS таймер обновления визуализации
    m_syncTimer->setInterval(16);
    connect(m_syncTimer, &QTimer::timeout, this, &CalibrationDialog::onTick);
    m_syncTimer->start();
}

CalibrationDialog::~CalibrationDialog()
{
    if (m_syncTimer) {
        m_syncTimer->stop();
    }
    if (m_player) {
        m_player->stop();
    }
}

QString CalibrationDialog::generate8BitBeatFile()
{
    QString filePath = QDir::temp().filePath(QStringLiteral("beatkey_calibration_8bit.wav"));

    const int sampleRate = 44100;
    const int durationSec = 4;
    const int totalSamples = sampleRate * durationSec;
    const int dataSize = totalSamples * sizeof(qint16);

    QByteArray buffer;
    buffer.resize(sizeof(WavPcmHeader) + dataSize);

    auto *header = reinterpret_cast<WavPcmHeader*>(buffer.data());
    header->chunkSize = static_cast<quint32>(sizeof(WavPcmHeader) - 8 + dataSize);
    header->subchunk2Size = static_cast<quint32>(dataSize);

    auto *samples = reinterpret_cast<qint16*>(buffer.data() + sizeof(WavPcmHeader));

    double phaseKick = 0.0;
    double phaseSnare = 0.0;

    for (int i = 0; i < totalSamples; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        int beat = static_cast<int>(t / 0.5) % 8;
        double tau = t - static_cast<int>(t / 0.5) * 0.5;

        double sample = 0.0;

        if (beat % 2 == 0) {
            // Ударные доли (0, 2, 4, 6): плотный 8-битный бочечный бас
            if (tau < 0.28) {
                double freq = 165.0 * std::exp(-19.0 * tau) + 48.0;
                phaseKick += 2.0 * M_PI * freq / sampleRate;
                double wave = (std::sin(phaseKick) > 0.0) ? 1.0 : -1.0;
                double env = std::exp(-10.5 * tau);
                sample += wave * env * 0.70;
            }
        } else {
            // Доли 1, 3, 5, 7: 8-битный снейр/щелчок со спадом
            if (tau < 0.20) {
                double noise = QRandomGenerator::global()->generateDouble() * 2.0 - 1.0;
                noise = std::round(noise * 4.0) / 4.0;
                phaseSnare += 2.0 * M_PI * 360.0 / sampleRate;
                double tone = (std::sin(phaseSnare) > 0.0) ? 0.4 : -0.4;
                double env = std::exp(-24.0 * tau);
                sample += (noise * 0.65 + tone * 0.35) * env * 0.65;
            }
        }

        // Ритмичный хэт на каждые 125 мс
        double tauHat = std::fmod(t, 0.25);
        if (tauHat < 0.035) {
            double noiseHat = QRandomGenerator::global()->generateDouble() * 2.0 - 1.0;
            double envHat = std::exp(-70.0 * tauHat);
            sample += noiseHat * envHat * 0.18;
        }

        sample = std::clamp(sample, -1.0, 1.0);
        samples[i] = static_cast<qint16>(sample * 29000.0);
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(buffer);
        file.close();
    }

    return filePath;
}

void CalibrationDialog::setupUi()
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 20, 20, 20);

    auto *container = new QWidget(this);
    container->setObjectName(QStringLiteral("dialogContainer"));
    container->setStyleSheet(Theme::exitDialogContainerStyle());
    container->setFixedSize(500, 440);

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(28);
    shadow->setColor(QColor(0, 0, 0, 190));
    shadow->setOffset(0, 8);
    container->setGraphicsEffect(shadow);

    auto *innerLayout = new QVBoxLayout(container);
    innerLayout->setContentsMargins(28, 20, 28, 24);
    innerLayout->setSpacing(14);

    // 1. Шапка с заголовком и кнопкой закрытия (✕)
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);

    auto *spacerLeft = new QWidget(container);
    spacerLeft->setFixedSize(28, 28);
    headerLayout->addWidget(spacerLeft);

    auto *titleLabel = new QLabel(QStringLiteral("Автокалибровка задержки"), container);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(Theme::dialogTitleStyle());
    headerLayout->addWidget(titleLabel, 1);

    m_closeBtn = new QPushButton(QStringLiteral("✕"), container);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setFixedSize(28, 28);
    m_closeBtn->setStyleSheet(Theme::dialogCloseButtonStyle());
    connect(m_closeBtn, &QPushButton::clicked, this, &CalibrationDialog::handleCancelOrClose);
    headerLayout->addWidget(m_closeBtn);
    innerLayout->addLayout(headerLayout);

    // Подзаголовок с пояснением
    auto *hintLabel = new QLabel(
        QStringLiteral("Квадрат вспыхивает в момент удара бита.\n"
                       "Отрегулируйте смещение стрелками до точного совпадения со звуком."),
        container
    );
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setStyleSheet(Theme::dialogMessageStyle());
    innerLayout->addWidget(hintLabel);

    // 2. Центральный виджет вспыхивающего квадрата
    m_squareWidget = new CalibrationSquareWidget(container);
    innerLayout->addWidget(m_squareWidget);

    // 3. Блок ручной регулировки задержки со стрелками
    auto *adjustLayout = new QHBoxLayout();
    adjustLayout->setAlignment(Qt::AlignCenter);
    adjustLayout->setSpacing(8);

    auto makeArrowBtn = [&](const QString &text, int delta) {
        auto *btn = new QPushButton(text, container);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(Theme::secondaryButtonStyle());
        btn->setFixedHeight(36);
        btn->setMinimumWidth(delta > 1 || delta < -1 ? 72 : 54);
        btn->setAutoRepeat(true);
        btn->setAutoRepeatDelay(280);
        btn->setAutoRepeatInterval(75);
        connect(btn, &QPushButton::clicked, this, [this, delta]() {
            adjustOffset(delta);
        });
        return btn;
    };

    adjustLayout->addWidget(makeArrowBtn(QStringLiteral("◄ -5 мс"), -5));
    adjustLayout->addWidget(makeArrowBtn(QStringLiteral("-1"), -1));

    m_offsetValueLabel = new QLabel(container);
    m_offsetValueLabel->setFixedSize(130, 38);
    m_offsetValueLabel->setAlignment(Qt::AlignCenter);
    m_offsetValueLabel->setStyleSheet(QStringLiteral(
        "background: #241740;"
        "border: 1px solid #7d44cf;"
        "border-radius: 8px;"
        "color: #ffffff;"
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 17px;"
        "font-weight: bold;"
    ));
    adjustLayout->addWidget(m_offsetValueLabel);

    adjustLayout->addWidget(makeArrowBtn(QStringLiteral("+1"), 1));
    adjustLayout->addWidget(makeArrowBtn(QStringLiteral("+5 мс ►"), 5));
    innerLayout->addLayout(adjustLayout);

    innerLayout->addStretch(1);

    // 4. Нижние кнопки: Сохранить и выйти, Отмена
    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(14);
    bottomLayout->setAlignment(Qt::AlignCenter);

    m_saveBtn = new QPushButton(QStringLiteral("Сохранить и выйти"), container);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setStyleSheet(Theme::primaryButtonStyle());
    m_saveBtn->setFixedHeight(40);
    m_saveBtn->setMinimumWidth(160);
    connect(m_saveBtn, &QPushButton::clicked, this, &CalibrationDialog::handleSaveAndExit);
    bottomLayout->addWidget(m_saveBtn);

    m_cancelBtn = new QPushButton(QStringLiteral("Отмена"), container);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_cancelBtn->setFixedHeight(40);
    m_cancelBtn->setMinimumWidth(120);
    connect(m_cancelBtn, &QPushButton::clicked, this, &CalibrationDialog::handleCancelOrClose);
    bottomLayout->addWidget(m_cancelBtn);

    innerLayout->addLayout(bottomLayout);
    outerLayout->addWidget(container);

    adjustOffset(0); // Инициализация текста смещения
}

void CalibrationDialog::adjustOffset(int deltaMs)
{
    m_currentOffsetMs = qBound(
        SettingsManager::MIN_AUDIO_OFFSET_MS,
        m_currentOffsetMs + deltaMs,
        SettingsManager::MAX_AUDIO_OFFSET_MS
    );

    QString sign = (m_currentOffsetMs > 0) ? QStringLiteral("+") : QString();
    m_offsetValueLabel->setText(QStringLiteral("%1%2 мс").arg(sign).arg(m_currentOffsetMs));
}

void CalibrationDialog::onTick()
{
    // Расчет текущей позиции аудиотрека
    qint64 pos = m_player->position();
    if (pos == m_lastAudioPos && m_audioClock.isValid()) {
        pos += m_audioClock.elapsed();
    } else {
        m_lastAudioPos = pos;
        m_audioClock.restart();
    }

    // Смещение компенсирует задержку
    qint64 adjustedMs = pos - m_currentOffsetMs;
    if (adjustedMs < 0) {
        adjustedMs += 4000 * 20;
    }

    int phase = adjustedMs % 500;
    m_currentBeatIdx = (adjustedMs / 500) % 4;

    // Вспышка квадрата длится ~95 мс
    if (phase < 95) {
        m_flashRatio = 1.0 - (phase / 95.0);
    } else {
        m_flashRatio = 0.0;
    }

    if (m_squareWidget) {
        m_squareWidget->setState(m_flashRatio, m_currentBeatIdx);
    }
}

void CalibrationDialog::handleSaveAndExit()
{
    SettingsManager::instance().setAudioOffsetMs(m_currentOffsetMs);
    SettingsManager::instance().saveSettings();
    accept();
}

void CalibrationDialog::handleCancelOrClose()
{
    if (m_currentOffsetMs != m_initialOffsetMs) {
        SavePromptDialog dlg(
            this,
            QStringLiteral("Калибровка задержки"),
            QStringLiteral("Смещение задержки было изменено.\nСохранить новое значение перед выходом?")
        );
        dlg.exec();

        if (dlg.promptResult() == SavePromptDialog::Result::Save) {
            handleSaveAndExit();
            return;
        } else if (dlg.promptResult() == SavePromptDialog::Result::Discard) {
            m_currentOffsetMs = m_initialOffsetMs;
            reject();
            return;
        } else {
            // Cancel -> остаемся в диалоге калибровки
            return;
        }
    }

    reject();
}

void CalibrationDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        return;
    }

    if (event->key() == Qt::Key_Escape) {
        handleCancelOrClose();
        return;
    }

    // Стрелки влево/вправо для изменения задержки
    if (event->key() == Qt::Key_Left) {
        adjustOffset(-5);
        return;
    }
    if (event->key() == Qt::Key_Right) {
        adjustOffset(5);
        return;
    }
    if (event->key() == Qt::Key_Down) {
        adjustOffset(-1);
        return;
    }
    if (event->key() == Qt::Key_Up) {
        adjustOffset(1);
        return;
    }

    QDialog::keyPressEvent(event);
}

void CalibrationDialog::closeEvent(QCloseEvent *event)
{
    event->ignore();
    handleCancelOrClose();
}
