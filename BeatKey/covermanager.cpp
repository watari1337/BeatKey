#include "covermanager.h"
#include "theme.h"
#include "audiomanager.h"

#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QImage>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>

QString CoverManager::standardCoverPath(StandardCover cover)
{
    return (cover == StandardCover::Standard1) ? STANDARD_COVER_1 : STANDARD_COVER_2;
}

QPixmap CoverManager::standardCoverPixmap(StandardCover cover)
{
    static QPixmap s_pixmap1(STANDARD_COVER_1);
    static QPixmap s_pixmap2(STANDARD_COVER_2);
    return (cover == StandardCover::Standard1) ? s_pixmap1 : s_pixmap2;
}

bool CoverManager::isStandardCover(const QString &path)
{
    return path == STANDARD_COVER_1 || path == STANDARD_COVER_2;
}

QString CoverManager::coverFilter()
{
    return QStringLiteral("Изображения (*.png *.jpg *.jpeg);;Все файлы (*.*)");
}

QString CoverManager::audioFilter()
{
    return QStringLiteral("Музыкальные файлы (*.mp3 *.wav *.ogg);;Все файлы (*.*)");
}

QStringList CoverManager::supportedCoverExtensions()
{
    return { QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg") };
}

QStringList CoverManager::supportedAudioExtensions()
{
    return { QStringLiteral("mp3"), QStringLiteral("wav"), QStringLiteral("ogg") };
}

bool CoverManager::validateAndLoadCover(const QString &filePath,
                                        QPixmap *outPixmap,
                                        QString *errorMessage)
{
    if (filePath.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Путь к файлу обложки не указан.");
        }
        return false;
    }

    if (isStandardCover(filePath)) {
        if (outPixmap) {
            *outPixmap = (filePath == STANDARD_COVER_1)
                ? standardCoverPixmap(StandardCover::Standard1)
                : standardCoverPixmap(StandardCover::Standard2);
        }
        return true;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Файл обложки не найден по указанному пути.");
        }
        return false;
    }

    if (!fileInfo.isFile()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Указанный путь не является файлом.");
        }
        return false;
    }

    if (fileInfo.size() <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Файл обложки пуст (0 байт) или поврежден.");
        }
        return false;
    }

    QString ext = fileInfo.suffix().toLower();
    if (!supportedCoverExtensions().contains(ext)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Неподдерживаемый формат изображения '%1'.\n"
                                           "Поддерживаются только форматы: .png, .jpg, .jpeg.")
                                .arg(fileInfo.suffix());
        }
        return false;
    }

    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    if (!reader.canRead()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Файл изображения поврежден или имеет неверный внутренний формат.");
        }
        return false;
    }

    QImage img = reader.read();
    if (img.isNull() || img.width() <= 0 || img.height() <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Не удалось открыть изображение: данные файла повреждены.");
        }
        return false;
    }

    if (outPixmap) {
        *outPixmap = QPixmap::fromImage(img);
    }
    return true;
}

bool CoverManager::validateAudioFile(const QString &filePath, QString *errorMessage)
{
    if (filePath.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Путь к аудиофайлу не указан.");
        }
        return false;
    }

    if (filePath.startsWith(QStringLiteral(":/")) || filePath.startsWith(QStringLiteral("qrc:/"))) {
        return true;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Аудиофайл не найден по указанному пути.");
        }
        return false;
    }

    if (!fileInfo.isFile()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Указанный путь не является файлом.");
        }
        return false;
    }

    if (fileInfo.size() <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Аудиофайл пуст (0 байт) или поврежден.");
        }
        return false;
    }

    QString ext = fileInfo.suffix().toLower();
    if (!supportedAudioExtensions().contains(ext)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Неподдерживаемый аудиоформат '%1'.\n"
                                           "Поддерживаются только форматы: .mp3, .wav, .ogg.")
                                .arg(fileInfo.suffix());
        }
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Не удалось открыть аудиофайл для чтения.");
        }
        return false;
    }

    qint64 fileSize = fileInfo.size();

    if (ext == QStringLiteral("wav")) {
        if (fileSize < 44) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл WAV поврежден: размер файла слишком мал для заголовка.");
            }
            return false;
        }
        QByteArray header = file.read(12);
        if (header.size() < 12 || !header.startsWith("RIFF") || header.mid(8, 4) != "WAVE") {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл имеет расширение .wav, но не содержит корректного заголовка RIFF WAVE или поврежден.");
            }
            return false;
        }
    } else if (ext == QStringLiteral("ogg")) {
        if (fileSize < 14) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл Ogg поврежден: размер файла слишком мал.");
            }
            return false;
        }
        QByteArray header = file.read(4);
        if (header != "OggS") {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл имеет расширение .ogg, но не содержит корректного контейнера Ogg или поврежден.");
            }
            return false;
        }
    } else if (ext == QStringLiteral("mp3")) {
        if (fileSize < 64) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл MP3 поврежден: размер файла слишком мал.");
            }
            return false;
        }
        QByteArray chunk = file.read(4096);
        bool hasId3 = chunk.startsWith("ID3");
        bool hasMpegSync = false;
        if (!hasId3) {
            for (int i = 0; i < chunk.size() - 1; ++i) {
                unsigned char b1 = static_cast<unsigned char>(chunk[i]);
                unsigned char b2 = static_cast<unsigned char>(chunk[i + 1]);
                if (b1 == 0xFF && (b2 & 0xE0) == 0xE0) {
                    hasMpegSync = true;
                    break;
                }
            }
        }
        if (!hasId3 && !hasMpegSync) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл имеет расширение .mp3, но не содержит корректных аудиофреймов MP3 или поврежден.");
            }
            return false;
        }
    }

    return true;
}

void CoverManager::showMediaErrorDialog(QWidget *parent,
                                        const QString &title,
                                        const QString &message)
{
    AudioManager::instance().playClickSound();

    QDialog dialog(parent);
    dialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    dialog.setAttribute(Qt::WA_TranslucentBackground);
    dialog.setModal(true);

    auto *outerLayout = new QVBoxLayout(&dialog);
    outerLayout->setContentsMargins(20, 20, 20, 20);

    auto *container = new QWidget(&dialog);
    container->setObjectName(QStringLiteral("dialogContainer"));
    container->setStyleSheet(Theme::exitDialogContainerStyle());

    auto *shadow = new QGraphicsDropShadowEffect(&dialog);
    shadow->setBlurRadius(25);
    shadow->setColor(QColor(0, 0, 0, 180));
    shadow->setOffset(0, 8);
    container->setGraphicsEffect(shadow);

    auto *innerLayout = new QVBoxLayout(container);
    innerLayout->setContentsMargins(24, 20, 24, 24);
    innerLayout->setSpacing(16);

    // Header with title and close cross
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);

    auto *spacerLeft = new QWidget(container);
    spacerLeft->setFixedSize(28, 28);
    headerLayout->addWidget(spacerLeft);

    auto *titleLabel = new QLabel(title, container);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(Theme::dialogErrorTitleStyle());
    headerLayout->addWidget(titleLabel, 1);

    auto *closeBtn = new QPushButton(QStringLiteral("✕"), container);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedSize(28, 28);
    closeBtn->setStyleSheet(Theme::dialogCloseButtonStyle());
    QObject::connect(closeBtn, &QPushButton::clicked, &dialog, [&dialog]() {
        AudioManager::instance().playClickSound();
        dialog.reject();
    });
    headerLayout->addWidget(closeBtn);
    innerLayout->addLayout(headerLayout);

    // Error message
    auto *msgLabel = new QLabel(message, container);
    msgLabel->setAlignment(Qt::AlignCenter);
    msgLabel->setWordWrap(true);
    msgLabel->setStyleSheet(Theme::dialogMessageStyle());
    innerLayout->addWidget(msgLabel);

    // Centered OK button
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);

    auto *okBtn = new QPushButton(QStringLiteral("ОК"), container);
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setStyleSheet(Theme::primaryButtonStyle());
    okBtn->setFixedHeight(40);
    okBtn->setMinimumWidth(130);
    QObject::connect(okBtn, &QPushButton::clicked, &dialog, [&dialog]() {
        AudioManager::instance().playClickSound();
        dialog.accept();
    });
    buttonLayout->addWidget(okBtn);
    innerLayout->addLayout(buttonLayout);

    outerLayout->addWidget(container);
    dialog.exec();
}
