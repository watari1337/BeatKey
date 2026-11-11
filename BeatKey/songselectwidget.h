#ifndef SONGSELECTWIDGET_H
#define SONGSELECTWIDGET_H

#include <QWidget>
#include <QVector>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QFrame>

#include "beatmap.h"

/**
 * @brief SongSelectWidget provides the song selection carousel and detail card.
 *
 * Implements empty state handling, importing .bkey files into the local
 * 'songs' folder located next to the executable, and track preview cards.
 */
class SongSelectWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SongSelectWidget(QWidget *parent = nullptr);
    ~SongSelectWidget() override = default;

    void refreshSongList();
    static QString songsDirectory();

signals:
    void backRequested();
    void playMapRequested(const Beatmap &beatmap);

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void handleImportFolder();
    void handleSongSelected(int row);

private:
    void setupUi();
    void updateDetailsCard(const Beatmap &map);
    void updateEmptyStateVisibility();
    void playPreview(const Beatmap &map);

    QVector<Beatmap> m_loadedBeatmaps;

    // Header controls
    QPushButton *m_backBtn;
    QPushButton *m_importFolderBtn;

    // Content container & Empty State
    QWidget *m_contentContainer;
    QWidget *m_emptyStateWidget;

    // Song List (Left column)
    QListWidget *m_songListWidget;

    // Large Details Card (Right column)
    QFrame *m_detailsCard;
    QLabel *m_coverArtLabel;
    QLabel *m_titleLabel;
    QLabel *m_artistLabel;
    QLabel *m_mapperLabel;
    QLabel *m_difficultyLabel;
    QLabel *m_bpmLabel;
    QLabel *m_notesCountLabel;
    QLabel *m_durationLabel;
    QPushButton *m_startPlayBtn;
};

#endif // SONGSELECTWIDGET_H
