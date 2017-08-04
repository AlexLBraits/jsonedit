#include "jsonview.h"
#include "jsonmodel.h"

JsonView::JsonView(QWidget *parent)
    : QTreeView(parent)
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
    connect(m_undoAction, &QAction::triggered, [this]()
    {
        JsonModel* model = dynamic_cast<JsonModel*>(this->model());
        if(model) model->undoStask().undo();
    });
    addAction(m_undoAction);

    m_redoAction = new QAction(tr("Redo"));
    m_redoAction->setShortcut(QKeySequence::Redo);
    connect(m_redoAction, &QAction::triggered, [this]()
    {
        JsonModel* model = dynamic_cast<JsonModel*>(this->model());
        if(model) model->undoStask().redo();
    });
    addAction(m_redoAction);

    m_removeAction = new QAction(tr("Remove"));
    m_removeAction->setShortcut(QKeySequence::Delete);
    connect(m_removeAction, &QAction::triggered, [this]()
    {
        JsonModel* model = dynamic_cast<JsonModel*>(this->model());
        if(model)
        {
            QModelIndex index = this->currentIndex();
            model->removeRows(index.row(), 1, index.parent());
        }
    });
    addAction(m_removeAction);

    m_insertAction = new QAction(tr("Insert"));
    m_insertAction->setShortcut(Qt::Key_Space);
    connect(m_insertAction, &QAction::triggered, [this]()
    {
        JsonModel* model = dynamic_cast<JsonModel*>(this->model());
        if(model)
        {
            QModelIndex index = this->currentIndex();
            model->insertDefaultRow(index.row(), index.parent());
        }
    });
    addAction(m_insertAction);

    m_insertInsideAction = new QAction(tr("Insert Inside"));
    m_insertInsideAction->setShortcut(Qt::SHIFT + Qt::Key_Space);
    connect(m_insertInsideAction, &QAction::triggered, [this]()
    {
        JsonModel* model = dynamic_cast<JsonModel*>(this->model());
        if(model)
        {
            model->insertDefaultRow(0, this->currentIndex());
        }
    });
    addAction(m_insertInsideAction);

}

void JsonView::autoResize()
{
    resizeColumnToContents(0);
}
