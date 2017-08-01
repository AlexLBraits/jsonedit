#ifndef JSONVIEW_H
#define JSONVIEW_H

#include <QTreeView>
#include <QAction>

class JsonView : public QTreeView
{
    Q_OBJECT
public:
    JsonView(QWidget *parent = Q_NULLPTR);

public slots:
    void autoResize();

private:
    QAction* m_undoAction;
    QAction* m_redoAction;
    QAction* m_removeAction;
};

#endif // JSONVIEW_H
