#include "editorcommands.h"
#include "beatmapeditorwidget.h"

// ============================================================================
// AddNoteCommand
// ============================================================================

AddNoteCommand::AddNoteCommand(BeatmapEditorWidget *editor, const NoteData &note, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_editor(editor)
    , m_note(note)
{
    setText(QStringLiteral("Добавление ноты (%1 мс)").arg(note.timeMs));
}

void AddNoteCommand::undo()
{
    if (m_editor) {
        m_editor->executeRemoveNote(m_note);
    }
}

void AddNoteCommand::redo()
{
    if (m_editor) {
        m_editor->executeAddNote(m_note);
    }
}

// ============================================================================
// DeleteNotesCommand
// ============================================================================

DeleteNotesCommand::DeleteNotesCommand(BeatmapEditorWidget *editor, const QVector<NoteData> &notes, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_editor(editor)
    , m_notes(notes)
{
    setText(QStringLiteral("Удаление %1 нот(ы)").arg(notes.size()));
}

void DeleteNotesCommand::undo()
{
    if (m_editor) {
        m_editor->executeRestoreNotes(m_notes);
    }
}

void DeleteNotesCommand::redo()
{
    if (m_editor) {
        m_editor->executeDeleteNotes(m_notes);
    }
}

// ============================================================================
// MoveNotesCommand
// ============================================================================

MoveNotesCommand::MoveNotesCommand(BeatmapEditorWidget *editor, const QVector<int> &originalTimes, int deltaMs, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_editor(editor)
    , m_originalTimes(originalTimes)
    , m_deltaMs(deltaMs)
{
    setText(QStringLiteral("Перемещение нот на %1 мс").arg(deltaMs));
}

void MoveNotesCommand::undo()
{
    if (m_editor) {
        m_editor->executeMoveNotes(m_originalTimes, 0); // Revert to original
    }
}

void MoveNotesCommand::redo()
{
    if (m_editor) {
        m_editor->executeMoveNotes(m_originalTimes, m_deltaMs);
    }
}

// ============================================================================
// BatchAddNotesCommand (Tap Record)
// ============================================================================

BatchAddNotesCommand::BatchAddNotesCommand(BeatmapEditorWidget *editor, const QVector<NoteData> &notes, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_editor(editor)
    , m_notes(notes)
{
    setText(QStringLiteral("Запись (Tap Record: %1 нот)").arg(notes.size()));
}

void BatchAddNotesCommand::undo()
{
    if (m_editor) {
        m_editor->executeBatchRemoveNotes(m_notes);
    }
}

void BatchAddNotesCommand::redo()
{
    if (m_editor) {
        m_editor->executeBatchAddNotes(m_notes);
    }
}
