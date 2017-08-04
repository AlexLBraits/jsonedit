#include "jsonedit.h"
#include "jsonmodel.h"

#include <QContextMenuEvent>
#include <QMenu>

JsonEdit::JsonEdit(QWidget *parent)
    : QTreeView(parent),
      m_undoAction(0),
      m_redoAction(0)
{
    connect(this, SIGNAL(expanded(QModelIndex)), this, SLOT(autoResize()));
    connect(this, SIGNAL(collapsed(QModelIndex)), this, SLOT(autoResize()));

    setEditTriggers(
        QAbstractItemView::EditKeyPressed |
        QAbstractItemView::SelectedClicked
    );
    setSelectionBehavior (QAbstractItemView::SelectItems);

    m_undoAction = new QAction(tr("Undo"));
    m_undoAction->setShortcut(QKeySequence::Undo);
    addAction(m_undoAction);

    m_redoAction = new QAction(tr("Redo"));
    m_redoAction->setShortcut(QKeySequence::Redo);
    addAction(m_redoAction);

    m_removeAction = new QAction(tr("Remove"));
    m_removeAction->setShortcut(QKeySequence::Delete);
    connect(m_removeAction, SIGNAL(triggered(bool)), this, SLOT(removeSelectedRow()));
    addAction(m_removeAction);

    m_insertAction = new QAction(tr("Insert"));
    m_insertAction->setShortcut(Qt::Key_Space);
    connect(m_insertAction, SIGNAL(triggered(bool)), this, SLOT(insertRow()));
    addAction(m_insertAction);

    m_insertInsideAction = new QAction(tr("Insert Inside"));
    m_insertInsideAction->setShortcut(Qt::SHIFT + Qt::Key_Space);
    connect(m_insertInsideAction, SIGNAL(triggered(bool)), this, SLOT(insertInsideRow()));
    addAction(m_insertInsideAction);

    clear();
}

void JsonEdit::autoResize()
{
    resizeColumnToContents(0);
}

#ifndef QT_NO_CONTEXTMENU
void JsonEdit::contextMenuEvent(QContextMenuEvent *event)
{
    m_removeAction->setEnabled(selectionModel()->currentIndex().isValid());
    m_insertInsideAction->setEnabled(document()->itemIsContainer(selectionModel()->currentIndex()));

    QMenu menu(this);
    menu.addAction(m_undoAction);
    menu.addAction(m_redoAction);
    menu.addAction(m_removeAction);
    menu.addAction(m_insertAction);
    menu.addAction(m_insertInsideAction);

    menu.exec(event->globalPos());
}

void JsonEdit::setDocumentAndModel(JsonDocumentAndModel *documentAndModel)
{
    m_documentAndModel = documentAndModel;

    m_undoAction->setEnabled(m_documentAndModel->undoStack()->canUndo());
    connect(m_documentAndModel->undoStack(), &QUndoStack::canUndoChanged,
            [this](bool canUndo){m_undoAction->setEnabled(canUndo);});

    m_redoAction->setEnabled(m_documentAndModel->undoStack()->canRedo());
    connect(m_documentAndModel->undoStack(), &QUndoStack::canRedoChanged,
            [this](bool canRedo){m_redoAction->setEnabled(canRedo);});

    connect(m_undoAction, SIGNAL(triggered(bool)), m_documentAndModel, SLOT(undo()));
    connect(m_redoAction, SIGNAL(triggered(bool)), m_documentAndModel, SLOT(redo()));
}
#endif // QT_NO_CONTEXTMENU

////////////////////////////////////////////////////////////////////////////////
///
/// Методы PlainTextEdit
///
JsonDocumentAndModel *JsonEdit::document() const
{
    return m_documentAndModel;
}

void JsonEdit::clear()
{
    setModel(new JsonDocumentAndModel);
    setDocumentAndModel((JsonDocumentAndModel*)model());
}

void JsonEdit::cut()
{

}

void JsonEdit::copy()
{

}

void JsonEdit::paste()
{

}

void JsonEdit::setPlainText(const QString &text)
{
    setModel(new JsonDocumentAndModel(text.toStdString()));
    setDocumentAndModel((JsonDocumentAndModel*)model());
}

QString JsonEdit::toPlainText() const
{
    return document()->toPlainText();
}

void JsonEdit::removeSelectedRow()
{
    QModelIndex index = this->currentIndex();
    document()->removeRows(index.row(), 1, index.parent());
}

void JsonEdit::insertRow()
{
    QModelIndex index = this->currentIndex();
    if(index.isValid())
        document()->insertDefaultRow(index.row(), index.parent());
    else
        document()->insertDefaultRow(0, index);
}

void JsonEdit::insertInsideRow()
{
    document()->insertDefaultRow(0, this->currentIndex());
}
