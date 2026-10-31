#include "newbeatmapdialog.h"
#include "settingsmanager.h"
#include "audiomanager.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QLibrary>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

NewBeatmapDialog::NewBeatmapDialog(QWidget *parent)
    : QDialog(parent)
    , m_isDragging(false)
    , m_chooseAudioBtn(nullptr)
    , m_audioPathLabel(nullptr)
    , m_titleEdit(nullptr)
    , m_mapperEdit(nullptr)
    , m_toggleAdvancedBtn(nullptr)
    , m_advancedContainer(nullptr)
    , m_artistEdit(nullptr)
    , m_difficultyCombo(nullptr)
    , m_bpmSpin(nullptr)
    , m_offsetSpin(nullptr)
    , m_chooseCoverBtn(nullptr)
    , m_coverPathLabel(nullptr)
    , m_createBtn(nullptr)
    , m_isAdvancedVisible(false)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);

    setupUi();
}

void NewBeatmapDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    adjustSize();
    centerOnParent();

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    typedef HRESULT (WINAPI *DwmSetWindowAttributeFunc)(HWND, DWORD, LPCVOID, DWORD);
    auto pDwmSetWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFunc>(
        QLibrary::resolve(QStringLiteral("dwmapi"), "DwmSetWindowAttribute")
    );
    if (pDwmSetWindowAttribute) {
        COLORREF borderColor = 0xFFFFFFFE;
        pDwmSetWindowAttribute(hwnd, 34, &borderColor, sizeof(borderColor));

        DWORD cornerPref = 1;
        pDwmSetWindowAttribute(hwnd, 33, &cornerPref, sizeof(cornerPref));
    }
#endif
}

void NewBeatmapDialog::centerOnParent()
{
    QWidget *topWin = parentWidget() ? parentWidget()->window() : nullptr;
    if (topWin) {
        QRect parentGeo = topWin->geometry();
        int x = parentGeo.x() + (parentGeo.width() - width()) / 2;
        int y = parentGeo.y() + (parentGeo.height() - height()) / 2;
        move(x, y);
    } else if (QScreen *screen = QGuiApplication::primaryScreen()) {
        QRect screenGeo = screen->geometry();
        int x = screenGeo.x() + (screenGeo.width() - width()) / 2;
        int y = screenGeo.y() + (screenGeo.height() - height()) / 2;
        move(x, y);
    }
}

void NewBeatmapDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *child = childAt(event->position().toPoint());
        bool isInteractive = false;
        for (QWidget *w = child; w && w != this; w = w->parentWidget()) {
            if (qobject_cast<QPushButton *>(w) ||
                qobject_cast<QLineEdit *>(w) ||
                qobject_cast<QComboBox *>(w) ||
                qobject_cast<QAbstractSpinBox *>(w)) {
                isInteractive = true;
                break;
            }
        }

        if (!isInteractive) {
            m_isDragging = true;
            m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
    QDialog::mousePressEvent(event);
}

void NewBeatmapDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
        return;
    }
    QDialog::mouseMoveEvent(event);
}

void NewBeatmapDialog::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        event->accept();
        return;
    }
    QDialog::mouseReleaseEvent(event);
}

QLabel *NewBeatmapDialog::createFieldLabel(const QString &text, QWidget *parent)
{
    auto *lbl = new QLabel(text, parent);
    lbl->setStyleSheet(QStringLiteral(
        "color: #a09cb0;"
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 12px;"
        "font-weight: normal;"
        "padding: 0px;"
        "margin: 0px;"
    ));
    return lbl;
}

void NewBeatmapDialog::setupUi()
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(16, 16, 16, 16);

    auto *container = new QWidget(this);
    container->setObjectName(QStringLiteral("dialogContainer"));
    container->setStyleSheet(Theme::dialogContainerStyle());

    auto *shadow = new QGraphicsDropShadowEffect(container);
    shadow->setBlurRadius(28);
    shadow->setColor(QColor(0, 0, 0, 200));
    shadow->setOffset(0, 8);
    container->setGraphicsEffect(shadow);

    auto *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(14);

    // Header with title and close button
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);

    auto *headerTitle = new QLabel(QStringLiteral("Создание новой карты"), container);
    headerTitle->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 20px;"
        "color: #ffffff;"
    ));

    auto *closeBtn = new QPushButton(QStringLiteral("✕"), container);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedSize(28, 28);
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: transparent;"
        "  color: #a09cb0;"
        "  border: none;"
        "  font-size: 16px;"
        "  border-radius: 14px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(217, 83, 111, 40);"
        "  color: #e57388;"
        "}"
    ));
    connect(closeBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        reject();
    });

    headerLayout->addWidget(headerTitle);
    headerLayout->addStretch(1);
    headerLayout->addWidget(closeBtn);
    mainLayout->addLayout(headerLayout);

    // ========================================================================
    // Mandatory Section
    // ========================================================================

    // 1. Audio selection
    mainLayout->addWidget(createFieldLabel(QStringLiteral("Аудиофайл (обязательно)"), container));

    auto *audioRow = new QHBoxLayout();
    audioRow->setSpacing(12);

    m_chooseAudioBtn = new QPushButton(QStringLiteral("Выбрать аудио..."), container);
    m_chooseAudioBtn->setCursor(Qt::PointingHandCursor);
    m_chooseAudioBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_chooseAudioBtn->setFixedSize(175, 36);
    connect(m_chooseAudioBtn, &QPushButton::clicked, this, &NewBeatmapDialog::handleChooseAudio);

    m_audioPathLabel = new QLabel(QStringLiteral("Файл не выбран"), container);
    m_audioPathLabel->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 12px; font-weight: normal;"));

    audioRow->addWidget(m_chooseAudioBtn);
    audioRow->addWidget(m_audioPathLabel, 1);
    mainLayout->addLayout(audioRow);

    // 2. Map Title (Mandatory)
    mainLayout->addWidget(createFieldLabel(QStringLiteral("Название карты (обязательно)"), container));

    m_titleEdit = new QLineEdit(container);
    m_titleEdit->setPlaceholderText(QStringLiteral("Например: Cyber Run"));
    m_titleEdit->setStyleSheet(Theme::lineEditStyle());
    connect(m_titleEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!text.trimmed().isEmpty()) {
            m_titleEdit->setStyleSheet(Theme::lineEditStyle());
        }
    });
    connect(m_titleEdit, &QLineEdit::returnPressed, this, &NewBeatmapDialog::validateAndAccept);
    mainLayout->addWidget(m_titleEdit);

    // 3. Mapper / Author (Mandatory)
    mainLayout->addWidget(createFieldLabel(QStringLiteral("Автор карты (обязательно)"), container));

    m_mapperEdit = new QLineEdit(container);
    m_mapperEdit->setPlaceholderText(QStringLiteral("Ваш никнейм"));
    m_mapperEdit->setStyleSheet(Theme::lineEditStyle());

    QString savedNick = SettingsManager::instance().nickname().trimmed();
    m_mapperEdit->setText(savedNick.isEmpty() ? QStringLiteral("Player") : savedNick);

    connect(m_mapperEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!text.trimmed().isEmpty()) {
            m_mapperEdit->setStyleSheet(Theme::lineEditStyle());
        }
    });
    connect(m_mapperEdit, &QLineEdit::returnPressed, this, &NewBeatmapDialog::validateAndAccept);
    mainLayout->addWidget(m_mapperEdit);

    // ========================================================================
    // Collapsible Advanced Settings Section
    // ========================================================================
    m_toggleAdvancedBtn = new QPushButton(QStringLiteral("Дополнительные настройки [ + ]"), container);
    m_toggleAdvancedBtn->setCursor(Qt::PointingHandCursor);
    m_toggleAdvancedBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: transparent;"
        "  color: #bb9af7;"
        "  border: none;"
        "  text-align: left;"
        "  font-size: 13px;"
        "  font-weight: normal;"
        "  padding: 4px 0;"
        "}"
        "QPushButton:hover {"
        "  color: #ffffff;"
        "}"
    ));
    connect(m_toggleAdvancedBtn, &QPushButton::clicked, this, &NewBeatmapDialog::toggleAdvancedSettings);
    mainLayout->addWidget(m_toggleAdvancedBtn);

    m_advancedContainer = new QWidget(container);
    auto *advLayout = new QVBoxLayout(m_advancedContainer);
    advLayout->setContentsMargins(0, 0, 0, 0);
    advLayout->setSpacing(12);

    // Artist
    advLayout->addWidget(createFieldLabel(QStringLiteral("Исполнитель трека"), m_advancedContainer));
    m_artistEdit = new QLineEdit(m_advancedContainer);
    m_artistEdit->setPlaceholderText(QStringLiteral("Например: Camellia"));
    m_artistEdit->setStyleSheet(Theme::lineEditStyle());
    advLayout->addWidget(m_artistEdit);

    // Difficulty
    advLayout->addWidget(createFieldLabel(QStringLiteral("Сложность"), m_advancedContainer));
    m_difficultyCombo = new QComboBox(m_advancedContainer);
    m_difficultyCombo->addItems(BeatmapDifficulties::ALL);
    m_difficultyCombo->setCurrentIndex(1); // "Нормально"
    m_difficultyCombo->setStyleSheet(Theme::comboBoxStyle());
    m_difficultyCombo->setCursor(Qt::PointingHandCursor);
    advLayout->addWidget(m_difficultyCombo);

    // Row for BPM and Offset
    auto *spinRow = new QHBoxLayout();
    spinRow->setSpacing(16);

    // Left: BPM
    auto *bpmCol = new QVBoxLayout();
    bpmCol->setSpacing(4);
    bpmCol->addWidget(createFieldLabel(QStringLiteral("Темп (BPM)"), m_advancedContainer));
    m_bpmSpin = new QDoubleSpinBox(m_advancedContainer);
    m_bpmSpin->setRange(30.0, 300.0);
    m_bpmSpin->setValue(120.0);
    m_bpmSpin->setSingleStep(1.0);
    m_bpmSpin->setDecimals(1);
    m_bpmSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_bpmSpin->setAlignment(Qt::AlignCenter);
    m_bpmSpin->setStyleSheet(Theme::spinBoxStyle());
    bpmCol->addWidget(m_bpmSpin);
    spinRow->addLayout(bpmCol, 1);

    // Right: Offset
    auto *offsetCol = new QVBoxLayout();
    offsetCol->setSpacing(4);
    offsetCol->addWidget(createFieldLabel(QStringLiteral("Смещение звука (Offset, мс)"), m_advancedContainer));
    m_offsetSpin = new QSpinBox(m_advancedContainer);
    m_offsetSpin->setRange(-500, 500);
    m_offsetSpin->setValue(0);
    m_offsetSpin->setSingleStep(1);
    m_offsetSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_offsetSpin->setAlignment(Qt::AlignCenter);
    m_offsetSpin->setStyleSheet(Theme::spinBoxStyle());
    offsetCol->addWidget(m_offsetSpin);
    spinRow->addLayout(offsetCol, 1);

    advLayout->addLayout(spinRow);

    // Cover art selection
    advLayout->addWidget(createFieldLabel(QStringLiteral("Обложка карты"), m_advancedContainer));
    auto *coverRow = new QHBoxLayout();
    coverRow->setSpacing(12);

    m_chooseCoverBtn = new QPushButton(QStringLiteral("Выбрать обложку..."), m_advancedContainer);
    m_chooseCoverBtn->setCursor(Qt::PointingHandCursor);
    m_chooseCoverBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_chooseCoverBtn->setFixedSize(175, 36);
    connect(m_chooseCoverBtn, &QPushButton::clicked, this, &NewBeatmapDialog::handleChooseCover);

    m_coverPathLabel = new QLabel(QStringLiteral("Файл не выбран"), m_advancedContainer);
    m_coverPathLabel->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 12px; font-weight: normal;"));

    coverRow->addWidget(m_chooseCoverBtn);
    coverRow->addWidget(m_coverPathLabel, 1);
    advLayout->addLayout(coverRow);

    m_advancedContainer->setVisible(false);
    mainLayout->addWidget(m_advancedContainer);

    // ========================================================================
    // Dialog Bottom Bar (Only "Создать карту", cancel is handled by top 'X' and Esc)
    // ========================================================================
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(0, 8, 0, 0);
    btnLayout->addStretch(1);

    m_createBtn = new QPushButton(QStringLiteral("Создать карту"), container);
    m_createBtn->setCursor(Qt::PointingHandCursor);
    m_createBtn->setStyleSheet(Theme::successButtonStyle());
    m_createBtn->setFixedSize(160, 40);
    connect(m_createBtn, &QPushButton::clicked, this, &NewBeatmapDialog::validateAndAccept);

    btnLayout->addWidget(m_createBtn);
    mainLayout->addLayout(btnLayout);

    outerLayout->addWidget(container);

    setFixedWidth(560);
}

void NewBeatmapDialog::handleChooseAudio()
{
    AudioManager::instance().playClickSound();

    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Выберите аудиофайл для карты"),
        defaultDir,
        QStringLiteral("Музыкальные файлы (*.mp3 *.wav *.ogg);;Все файлы (*.*)")
    );

    if (!filePath.isEmpty()) {
        m_selectedAudioPath = filePath;
        QFileInfo fi(filePath);
        QFontMetrics fm(m_audioPathLabel->font());
        QString elided = fm.elidedText(fi.fileName(), Qt::ElideMiddle, 230);
        m_audioPathLabel->setText(elided);
        m_audioPathLabel->setToolTip(filePath);
        m_audioPathLabel->setStyleSheet(QStringLiteral("color: #2ecc71; font-size: 12px; font-weight: normal;"));
        m_chooseAudioBtn->setStyleSheet(Theme::secondaryButtonStyle());
    }
}

void NewBeatmapDialog::handleChooseCover()
{
    AudioManager::instance().playClickSound();

    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Выберите обложку карты"),
        defaultDir,
        QStringLiteral("Изображения (*.png *.jpg *.jpeg);;Все файлы (*.*)")
    );

    if (!filePath.isEmpty()) {
        m_selectedCoverPath = filePath;
        QFileInfo fi(filePath);
        QFontMetrics fm(m_coverPathLabel->font());
        QString elided = fm.elidedText(fi.fileName(), Qt::ElideMiddle, 230);
        m_coverPathLabel->setText(elided);
        m_coverPathLabel->setToolTip(filePath);
        m_coverPathLabel->setStyleSheet(QStringLiteral("color: #2ecc71; font-size: 12px; font-weight: normal;"));
    }
}

void NewBeatmapDialog::toggleAdvancedSettings()
{
    AudioManager::instance().playClickSound();
    m_isAdvancedVisible = !m_isAdvancedVisible;
    m_advancedContainer->setVisible(m_isAdvancedVisible);
    m_toggleAdvancedBtn->setText(m_isAdvancedVisible
        ? QStringLiteral("Дополнительные настройки [ − ]")
        : QStringLiteral("Дополнительные настройки [ + ]"));

    if (m_advancedContainer->layout()) {
        m_advancedContainer->layout()->invalidate();
        m_advancedContainer->layout()->activate();
    }
    if (QWidget *cont = findChild<QWidget *>(QStringLiteral("dialogContainer"))) {
        if (cont->layout()) {
            cont->layout()->invalidate();
            cont->layout()->activate();
        }
    }
    if (layout()) {
        layout()->invalidate();
        layout()->activate();
    }

    QPoint currentPos = pos();
    if (!m_isAdvancedVisible) {
        resize(width(), 1);
    }
    adjustSize();
    move(currentPos); // Remain in the exact same position without jumping!
}

bool NewBeatmapDialog::validateForm()
{
    bool hasErrors = false;

    // 1. Audio validation
    if (m_selectedAudioPath.isEmpty() || !QFileInfo::exists(m_selectedAudioPath)) {
        m_chooseAudioBtn->setStyleSheet(Theme::errorButtonStyle());
        m_audioPathLabel->setText(QStringLiteral("Файл не выбран!"));
        m_audioPathLabel->setStyleSheet(QStringLiteral("color: #e57388; font-size: 12px; font-weight: normal;"));
        hasErrors = true;
    } else {
        m_chooseAudioBtn->setStyleSheet(Theme::secondaryButtonStyle());
    }

    // 2. Map Title validation
    QString title = m_titleEdit->text().trimmed();
    if (title.isEmpty()) {
        m_titleEdit->setStyleSheet(Theme::errorLineEditStyle());
        if (!hasErrors) {
            m_titleEdit->setFocus();
        }
        hasErrors = true;
    } else {
        m_titleEdit->setStyleSheet(Theme::lineEditStyle());
    }

    // 3. Mapper / Author validation
    QString mapper = m_mapperEdit->text().trimmed();
    if (mapper.isEmpty()) {
        m_mapperEdit->setStyleSheet(Theme::errorLineEditStyle());
        if (!hasErrors) {
            m_mapperEdit->setFocus();
        }
        hasErrors = true;
    } else {
        m_mapperEdit->setStyleSheet(Theme::lineEditStyle());
    }

    return !hasErrors;
}

void NewBeatmapDialog::validateAndAccept()
{
    AudioManager::instance().playClickSound();
    if (validateForm()) {
        QDialog::accept();
    }
}

void NewBeatmapDialog::accept()
{
    if (validateForm()) {
        QDialog::accept();
    }
}

Beatmap NewBeatmapDialog::createdBeatmap() const
{
    Beatmap map;
    map.meta.title = m_titleEdit->text().trimmed();
    map.meta.mapper = m_mapperEdit->text().trimmed();

    QString artist = m_artistEdit->text().trimmed();
    map.meta.artist = artist.isEmpty() ? QStringLiteral("Неизвестный исполнитель") : artist;

    map.meta.difficultyName = m_difficultyCombo->currentText();

    map.meta.bpm = m_bpmSpin->value();
    map.meta.audioOffset = m_offsetSpin->value();

    if (!m_selectedAudioPath.isEmpty()) {
        map.meta.audioFileName = QFileInfo(m_selectedAudioPath).fileName();
        map.extractedAudioPath = m_selectedAudioPath;
    }

    if (!m_selectedCoverPath.isEmpty()) {
        map.meta.coverFileName = QFileInfo(m_selectedCoverPath).fileName();
        map.coverImage.load(m_selectedCoverPath);
    }

    map.isValid = true;
    return map;
}

QString NewBeatmapDialog::sourceAudioPath() const
{
    return m_selectedAudioPath;
}

QString NewBeatmapDialog::sourceCoverPath() const
{
    return m_selectedCoverPath;
}
