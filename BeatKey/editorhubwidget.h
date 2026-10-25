#ifndef EDITORHUBWIDGET_H
#define EDITORHUBWIDGET_H

#include <QWidget>
#include <QVector>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QFrame>

#include "beatmap.h"

/**
 * @brief EditorHubWidget acts as the beatmap management hub for the editor.
 *
 * Allows creators to select an existing map for modification or initiate
 * creation of a new map via NewBeatmapDialog.
 */
class EditorHubWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditorHubWidget(QWidget *parent = nullptr);
    ~EditorHubWidget() override = default;

    void refreshMapList();

signals:
    void backRequested();
    void createMapRequested(const Beatmap &newBeatmap, const QString &audioPath, const QString &coverPath);
    void editMapRequested(const Beatmap &beatmap);

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void handleCreateNewMap();
    void handleImportFolder();
    void handleMapSelected(int row);

private:
    void setupUi();
    void updateDetailsCard(const Beatmap &map);
    void updateEmptyStateVisibility();

    QVector<Beatmap> m_loadedBeatmaps;

    // Header controls
    QPushButton *m_backBtn;
    QPushButton *m_importFolderBtn;
    QPushButton *m_createMapBtn;

    // Content container & Empty State
    QWidget *m_contentContainer;
    QWidget *m_emptyStateWidget;

    // Map List (Left column)
    QListWidget *m_mapListWidget;

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
    QPushButton *m_startEditBtn;
};

#endif // EDITORHUBWIDGET_H
