#ifndef EDITORCOMMANDS_H
#define EDITORCOMMANDS_H

#include <QUndoCommand>
#include <QVector>
#include "beatmap.h"

class BeatmapEditorWidget;

class AddNoteCommand : public QUndoCommand
{
public:
    AddNoteCommand(BeatmapEditorWidget *editor, const NoteData &note, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    BeatmapEditorWidget *m_editor;
    NoteData m_note;
};

class DeleteNotesCommand : public QUndoCommand
{
public:
    DeleteNotesCommand(BeatmapEditorWidget *editor, const QVector<NoteData> &notes, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    BeatmapEditorWidget *m_editor;
    QVector<NoteData> m_notes;
};

class MoveNotesCommand : public QUndoCommand
{
public:
    MoveNotesCommand(BeatmapEditorWidget *editor, const QVector<int> &originalTimes, int deltaMs, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    BeatmapEditorWidget *m_editor;
    QVector<int> m_originalTimes;
    int m_deltaMs;
};

class BatchAddNotesCommand : public QUndoCommand
{
public:
    BatchAddNotesCommand(BeatmapEditorWidget *editor, const QVector<NoteData> &notes, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    BeatmapEditorWidget *m_editor;
    QVector<NoteData> m_notes;
};

#endif // EDITORCOMMANDS_H
