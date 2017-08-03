#include "jsonview.h"
#include "jsonmodel.h"

JsonView::JsonView(QWidget *parent)
    : QTreeView(parent)
{
    connect(this, SIGNAL(expanded(QModelIndex)), this, SLOT(autoResize()));
    connect(this, SIGNAL(collapsed(QModelIndex)), this, SLOT(autoResize()));

    setEditTriggers(QAbstractItemView::EditKeyPressed|QAbstractItemView::AnyKeyPressed);

    m_undoAction = new QAction(tr("Undo"));
    m_undoAction->setShortcut(QKeySequence::Undo);
    connect(m_undoAction, &QAction::triggered, [this](){
        JsonModel* model = dynamic_cast<JsonModel*>(this->model());
        if(model) model->undoStask().undo();
    });
    addAction(m_undoAction);

    m_redoAction = new QAction(tr("Redo"));
    m_redoAction->setShortcut(QKeySequence::Redo);
    connect(m_redoAction, &QAction::triggered, [this](){
        JsonModel* model = dynamic_cast<JsonModel*>(this->model());
        if(model) model->undoStask().redo();
    });
    addAction(m_redoAction);

    m_removeAction = new QAction(tr("Remove"));
    m_removeAction->setShortcut(QKeySequence::Delete);
    connect(m_removeAction, &QAction::triggered, [this](){
        JsonModel* model = dynamic_cast<JsonModel*>(this->model());
        if(model)
        {
            QModelIndex index = this->currentIndex();
            model->removeRows(index.row(), 1, index.parent());
        }
    });
    addAction(m_removeAction);

    setSelectionBehavior (QAbstractItemView::SelectItems);
}

void JsonView::autoResize()
{
    resizeColumnToContents(0);
}
