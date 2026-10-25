#include "editorhubwidget.h"
#include "newbeatmapdialog.h"
#include "songselectwidget.h"
#include "beatmapmanager.h"
#include "settingsmanager.h"
#include "audiomanager.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QKeySequence>
#include <QDebug>
#include <QFileDialog>
#include <QStandardPaths>

EditorHubWidget::EditorHubWidget(QWidget *parent)
    : QWidget(parent)
    , m_backBtn(nullptr)
    , m_importFolderBtn(nullptr)
    , m_createMapBtn(nullptr)
    , m_contentContainer(nullptr)
    , m_emptyStateWidget(nullptr)
    , m_mapListWidget(nullptr)
    , m_detailsCard(nullptr)
    , m_coverArtLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_artistLabel(nullptr)
    , m_mapperLabel(nullptr)
    , m_difficultyLabel(nullptr)
    , m_bpmLabel(nullptr)
    , m_notesCountLabel(nullptr)
    , m_durationLabel(nullptr)
    , m_startEditBtn(nullptr)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
}

void EditorHubWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    Theme::paintWindowBackground(&painter, rect());
}

void EditorHubWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshMapList();
}

void EditorHubWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(16);

    // ========================================================================
    // Header Layout: Back button, Title, Create Map button
    // ========================================================================
    auto *headerLayout = new QHBoxLayout();

    m_backBtn = new QPushButton(QStringLiteral("← Главное меню"), this);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(Theme::secondaryButtonStyle());
    connect(m_backBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        AudioManager::instance().playMenuMusic();
        emit backRequested();
    });

    auto *titleLabel = new QLabel(QStringLiteral("Редактор карт"), this);
    titleLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 19px;"
        "font-weight: 600;"
        "color: #b0a6cc;"
    ));

    m_createMapBtn = new QPushButton(QStringLiteral("+ Создать карту"), this);
    m_createMapBtn->setCursor(Qt::PointingHandCursor);
    m_createMapBtn->setStyleSheet(Theme::primaryButtonStyle());
    connect(m_createMapBtn, &QPushButton::clicked, this, &EditorHubWidget::handleCreateNewMap);

    m_importFolderBtn = new QPushButton(QStringLiteral("Загрузить папку"), this);
    m_importFolderBtn->setCursor(Qt::PointingHandCursor);
    m_importFolderBtn->setStyleSheet(Theme::secondaryButtonStyle());
    connect(m_importFolderBtn, &QPushButton::clicked, this, &EditorHubWidget::handleImportFolder);

    headerLayout->addWidget(m_backBtn);
    headerLayout->addSpacing(20);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);
    headerLayout->addWidget(m_importFolderBtn);
    headerLayout->addSpacing(12);
    headerLayout->addWidget(m_createMapBtn);

    mainLayout->addLayout(headerLayout);

    // ========================================================================
    // Central Container (Split into Map List and Details Card)
    // ========================================================================
    m_contentContainer = new QWidget(this);
    m_contentContainer->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *contentLayout = new QHBoxLayout(m_contentContainer);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(25);

    // 1. Map List (Left column)
    m_mapListWidget = new QListWidget(m_contentContainer);
    m_mapListWidget->setMinimumWidth(360);
    m_mapListWidget->setMaximumWidth(440);
    m_mapListWidget->setStyleSheet(QStringLiteral(
        "QListWidget {"
        "  background: rgba(28, 20, 48, 180);"
        "  border: 1px solid #5a388f;"
        "  border-radius: 12px;"
        "  padding: 8px;"
        "  outline: none;"
        "}"
        "QListWidget::item {"
        "  background: #251b3d;"
        "  border: 1px solid #553388;"
        "  border-radius: 8px;"
        "  padding: 12px 14px;"
        "  margin-bottom: 8px;"
        "  color: #ffffff;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  font-size: 14px;"
        "}"
        "QListWidget::item:hover {"
        "  background: #3a2b56;"
        "  border: 1px solid #7d44cf;"
        "}"
        "QListWidget::item:selected {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #5c2bb8, stop:1 #8e44ad);"
        "  border: 1px solid #a259ff;"
        "  font-weight: bold;"
        "}"
    ));
    connect(m_mapListWidget, &QListWidget::currentRowChanged,
            this, &EditorHubWidget::handleMapSelected);

    contentLayout->addWidget(m_mapListWidget);

    // 2. Details Card (Right column)
    m_detailsCard = new QFrame(m_contentContainer);
    m_detailsCard->setObjectName(QStringLiteral("detailsCard"));
    m_detailsCard->setStyleSheet(QStringLiteral(
        "QFrame#detailsCard {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgba(32, 22, 58, 220), stop:1 rgba(48, 28, 80, 220));"
        "  border: 2px solid #7d44cf;"
        "  border-radius: 16px;"
        "}"
    ));

    auto *cardShadow = new QGraphicsDropShadowEffect(m_detailsCard);
    cardShadow->setBlurRadius(20);
    cardShadow->setColor(QColor(0, 0, 0, 160));
    cardShadow->setOffset(0, 6);
    m_detailsCard->setGraphicsEffect(cardShadow);

    auto *detailsLayout = new QVBoxLayout(m_detailsCard);
    detailsLayout->setContentsMargins(30, 26, 30, 26);
    detailsLayout->setSpacing(16);

    // Header inside card (Cover Art + Title/Artist/Mapper)
    auto *topMetaLayout = new QHBoxLayout();
    topMetaLayout->setSpacing(20);

    m_coverArtLabel = new QLabel(m_detailsCard);
    m_coverArtLabel->setFixedSize(140, 140);
    m_coverArtLabel->setStyleSheet(QStringLiteral(
        "background: #181128;"
        "border: 1px solid #65439c;"
        "border-radius: 12px;"
        "color: #7d6b99;"
        "font-size: 13px;"
    ));
    m_coverArtLabel->setAlignment(Qt::AlignCenter);

    auto *titleCol = new QVBoxLayout();
    titleCol->setAlignment(Qt::AlignVCenter);
    titleCol->setSpacing(6);

    m_titleLabel = new QLabel(m_detailsCard);
    m_titleLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 24px;"
        "font-weight: 900;"
        "color: #ffffff;"
    ));
    m_titleLabel->setWordWrap(true);

    m_artistLabel = new QLabel(m_detailsCard);
    m_artistLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 16px;"
        "font-weight: 600;"
        "color: #bb9af7;"
    ));

    m_mapperLabel = new QLabel(m_detailsCard);
    m_mapperLabel->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 13px;"));

    titleCol->addWidget(m_titleLabel);
    titleCol->addWidget(m_artistLabel);
    titleCol->addWidget(m_mapperLabel);

    topMetaLayout->addWidget(m_coverArtLabel);
    topMetaLayout->addLayout(titleCol, 1);
    detailsLayout->addLayout(topMetaLayout);

    // Separator line
    auto *separator = new QFrame(m_detailsCard);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet(QStringLiteral("background: #5a388f; max-height: 1px; border: none;"));
    detailsLayout->addWidget(separator);

    // Grid of parameters
    auto *grid = new QGridLayout();
    grid->setSpacing(16);

    auto createParamBox = [this](const QString &title, QLabel *&valLabel) -> QWidget* {
        auto *box = new QWidget(m_detailsCard);
        box->setStyleSheet(QStringLiteral(
            "background: rgba(24, 17, 44, 180);"
            "border: 1px solid #5a388f;"
            "border-radius: 8px;"
            "padding: 8px;"
        ));
        auto *l = new QVBoxLayout(box);
        l->setContentsMargins(10, 8, 10, 8);
        l->setSpacing(4);

        auto *t = new QLabel(title, box);
        t->setStyleSheet(QStringLiteral("color: #8888aa; font-size: 11px; font-weight: bold; text-transform: uppercase;"));

        valLabel = new QLabel(QStringLiteral("-"), box);
        valLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 15px; font-weight: bold;"));

        l->addWidget(t);
        l->addWidget(valLabel);
        return box;
    };

    grid->addWidget(createParamBox(QStringLiteral("Сложность"), m_difficultyLabel), 0, 0);
    grid->addWidget(createParamBox(QStringLiteral("Темп (BPM)"), m_bpmLabel), 0, 1);
    grid->addWidget(createParamBox(QStringLiteral("Количество нот"), m_notesCountLabel), 1, 0);
    grid->addWidget(createParamBox(QStringLiteral("Длительность"), m_durationLabel), 1, 1);

    detailsLayout->addLayout(grid);
    detailsLayout->addStretch(1);

    // Action button to open in editor
    m_startEditBtn = new QPushButton(QStringLiteral("Редактировать карту"), m_detailsCard);
    m_startEditBtn->setCursor(Qt::PointingHandCursor);
    m_startEditBtn->setStyleSheet(Theme::primaryButtonStyle());
    m_startEditBtn->setMinimumHeight(46);
    connect(m_startEditBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        int row = m_mapListWidget->currentRow();
        qDebug() << "[EditorHubWidget] edit button clicked! row=" << row << "m_loadedBeatmaps count=" << m_loadedBeatmaps.size();
        if (row >= 0 && row < m_loadedBeatmaps.size()) {
            emit editMapRequested(m_loadedBeatmaps[row]);
        }
    });

    detailsLayout->addWidget(m_startEditBtn);

    contentLayout->addWidget(m_detailsCard, 1);
    mainLayout->addWidget(m_contentContainer, 1);

    // ========================================================================
    // Empty State Widget (When no .bkey files are found)
    // ========================================================================
    m_emptyStateWidget = new QWidget(this);
    m_emptyStateWidget->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *emptyLayout = new QVBoxLayout(m_emptyStateWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(14);

    auto *emptyIcon = new QLabel(QStringLiteral(".bkey"), m_emptyStateWidget);
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyIcon->setStyleSheet(QStringLiteral("font-size: 24px; font-weight: 600; color: #8e62c8; letter-spacing: 1px;"));

    auto *emptyTitle = new QLabel(QStringLiteral("Нет карт для редактирования"), m_emptyStateWidget);
    emptyTitle->setAlignment(Qt::AlignCenter);
    emptyTitle->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 24px;"
        "font-weight: bold;"
        "color: #ffffff;"
    ));

    auto *emptyDesc = new QLabel(
        QStringLiteral("В папке пока нет доступных карт.\n"
                       "Нажмите кнопку «+ Создать карту» или «Загрузить папку», чтобы начать работу."),
        m_emptyStateWidget
    );
    emptyDesc->setAlignment(Qt::AlignCenter);
    emptyDesc->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 15px; line-height: 1.5;"));

    emptyLayout->addStretch(1);
    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addWidget(emptyTitle);
    emptyLayout->addWidget(emptyDesc);
    emptyLayout->addStretch(2);

    mainLayout->addWidget(m_emptyStateWidget, 1);
}

void EditorHubWidget::refreshMapList()
{
    QString targetDir = SongSelectWidget::songsDirectory();

    QStringList errors;
    m_loadedBeatmaps = BeatmapManager::instance().loadAllFromDirectory(targetDir, &errors);

    {
        const QSignalBlocker blocker(m_mapListWidget);
        m_mapListWidget->clear();
        for (const auto &map : m_loadedBeatmaps) {
            QString itemText = QStringLiteral("%1 — %2\n[%3 • %4]")
                .arg(map.meta.artist, map.meta.title, map.meta.difficultyName, map.formattedBpm());
            m_mapListWidget->addItem(itemText);
        }

        if (!m_loadedBeatmaps.isEmpty()) {
            m_mapListWidget->setCurrentRow(0);
        }
    }

    updateEmptyStateVisibility();

    if (!m_loadedBeatmaps.isEmpty()) {
        updateDetailsCard(m_loadedBeatmaps[0]);
    }
}

void EditorHubWidget::updateEmptyStateVisibility()
{
    bool hasMaps = !m_loadedBeatmaps.isEmpty();
    m_contentContainer->setVisible(hasMaps);
    m_emptyStateWidget->setVisible(!hasMaps);
}

void EditorHubWidget::handleImportFolder()
{
    AudioManager::instance().playClickSound();

    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString chosenDir = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Выберите папку с картами BeatKey"),
        desktopPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (chosenDir.isEmpty()) {
        return;
    }

    QString targetDir = SongSelectWidget::songsDirectory();
    BeatmapManager::instance().importAllFromDirectory(chosenDir, targetDir);

    refreshMapList();
}

void EditorHubWidget::handleCreateNewMap()
{
    AudioManager::instance().playClickSound();

    NewBeatmapDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Beatmap newMap = dialog.createdBeatmap();
        QString audioPath = dialog.sourceAudioPath();
        QString coverPath = dialog.sourceCoverPath();

        // 1. Генерируем уникальный путь файла .bkey в папке songs/
        QString uniqueBkeyPath = BeatmapManager::instance().generateUniqueBkeyPath(newMap.meta.title);

        // 2. Сразу же создаем архив .bkey в папке songs/
        QString error;
        bool ok = BeatmapManager::instance().exportToFile(
            uniqueBkeyPath,
            newMap,
            audioPath,
            coverPath,
            &error
        );

        if (!ok) {
            qWarning() << "[EditorHubWidget] Ошибка создания .bkey карты:" << error;
            return;
        }

        newMap.bkeyFilePath = uniqueBkeyPath;

        // 3. Обновляем список карт в хабе
        refreshMapList();

        // 4. Открываем редактор с только что созданной картой
        emit createMapRequested(newMap, audioPath, coverPath);
    }
}

void EditorHubWidget::handleMapSelected(int row)
{
    if (row < 0 || row >= m_loadedBeatmaps.size()) return;

    AudioManager::instance().playClickSound();
    const Beatmap &map = m_loadedBeatmaps[row];
    updateDetailsCard(map);
}

void EditorHubWidget::updateDetailsCard(const Beatmap &map)
{
    m_titleLabel->setText(map.meta.title.isEmpty() ? QStringLiteral("Без названия") : map.meta.title);
    m_artistLabel->setText(map.meta.artist.isEmpty() ? QStringLiteral("Неизвестный исполнитель") : map.meta.artist);
    m_mapperLabel->setText(QStringLiteral("Карта: %1").arg(map.meta.mapper.isEmpty() ? QStringLiteral("Аноним") : map.meta.mapper));

    m_difficultyLabel->setText(map.meta.difficultyName.isEmpty() ? QStringLiteral("Нормально") : map.meta.difficultyName);
    m_bpmLabel->setText(map.formattedBpm());
    m_notesCountLabel->setText(QString::number(map.totalNotes()));
    m_durationLabel->setText(map.formattedDuration());

    QPixmap cover = map.coverImage;
    if (cover.isNull()) {
        static QPixmap defaultBg(QStringLiteral(":/assets/images/standard_bg2.jpg"));
        cover = defaultBg;
    }
    if (!cover.isNull()) {
        m_coverArtLabel->setPixmap(cover.scaled(m_coverArtLabel->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        m_coverArtLabel->clear();
        m_coverArtLabel->setText(QStringLiteral("Нет обложки"));
        m_coverArtLabel->setStyleSheet(QStringLiteral(
            "background: #181128;"
            "border: 1px solid #65439c;"
            "border-radius: 12px;"
            "color: #7d6b99;"
            "font-size: 13px;"
        ));
    }
}
