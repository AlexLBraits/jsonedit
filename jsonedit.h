#ifndef JSONEDIT_H
#define JSONEDIT_H

#include <QTreeView>
#include <QAction>

class JsonDocumentAndModel;
//class QTextDocument;

class JsonEdit : public QTreeView
{
    Q_OBJECT
public:
    JsonEdit(QWidget *parent = Q_NULLPTR);

    void setDocumentAndModel(JsonDocumentAndModel *documentAndModel);

public slots:
    void autoResize();

public slots:
    JsonDocumentAndModel* document() const;
    void clear();
    void cut();
    void copy();
    void paste();
    void setPlainText(const QString &text);
    QString toPlainText() const;
    void removeSelectedRow();
    void insertRow();
    void insertInsideRow();

signals:
    void copyAvailable(bool);

protected:
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event) override;
#endif // QT_NO_CONTEXTMENU

private:
    QAction* m_undoAction;
    QAction* m_redoAction;
    QAction* m_removeAction;
    QAction* m_insertAction;
    QAction* m_insertInsideAction;

    JsonDocumentAndModel* m_documentAndModel;
};

#endif // JSONEDIT_H
