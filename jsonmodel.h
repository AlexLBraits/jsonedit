#ifndef JSONMODEL_H
#define JSONMODEL_H

#include <QAbstractItemModel>
#include <QTextDocument>
#include <QUndoCommand>
#include <QUndoStack>
#include <value.h>

class JsonDocumentAndModel;

////////////////////////////////////////////////////////////////////////////////
///
/// Команрды редактирования JsonModel
/// Нужны для обеспечения механизма Undo-Redo
///

///
/// \brief The JsonUndoCommand class - базовый класс для команд редактирования
/// json-модели
///
class JsonUndoCommand : public QUndoCommand
{
public:
    JsonUndoCommand(JsonDocumentAndModel* model,
                    const std::string& pointer);
    const JsonValue& target() const;

protected:
    JsonDocumentAndModel* m_model;
    std::string m_pointer;
};
///
/// \brief The SetKeyCommand class - команда установки нового значения для
/// ключа json-объекта
///
class SetKeyCommand : public JsonUndoCommand
{
public:
    SetKeyCommand(JsonDocumentAndModel* model,
                  const std::string& pointer,
                  int position,
                  const std::string& oldValue,
                  const std::string& newValue);

    void redo() override;
    void undo() override;

private:
    int m_position;
    std::string m_oldKey;
    std::string m_newKey;
};
///
/// \brief The SetValueCommand class - команда установки нового значения для
/// json-value
///
class SetValueCommand : public JsonUndoCommand
{
public:
    SetValueCommand(JsonDocumentAndModel* model,
                    const std::string& pointer,
                    const JsonValue& oldValue,
                    const JsonValue& newValue);

    void redo() override;
    void undo() override;

private:
    JsonValue m_oldValue;
    JsonValue m_newValue;
};
///
/// \brief The RemoveCommand class - команда удаления значения по ключу
///
class RemoveCommand : public JsonUndoCommand
{
public:
    RemoveCommand(JsonDocumentAndModel* model,
                  const std::string& pointer,
                  int position,
                  const JsonValue& key,
                  const JsonValue& value);

    void redo() override;
    void undo() override;

private:
    size_t m_pos;
    JsonValue m_key;
    JsonValue m_value;
};
///
/// \brief The InsertCommand class - команда вставки дефолтного ряда
///
class InsertCommand : public JsonUndoCommand
{
public:
    InsertCommand(JsonDocumentAndModel* model,
                  const std::string& pointer,
                  int position);

    void redo() override;
    void undo() override;

private:
    size_t m_pos;
};

////////////////////////////////////////////////////////////////////////////////
///
/// \brief The JsonModel class
///
class JsonDocumentAndModel : public QAbstractItemModel
{
    friend class SetKeyCommand;
    friend class SetValueCommand;
    friend class RemoveCommand;
    friend class InsertCommand;

    Q_OBJECT

public:
    JsonDocumentAndModel(QObject *parent = Q_NULLPTR);
    JsonDocumentAndModel(const JsonValue& jsonDocument);
    JsonDocumentAndModel(const std::string& jsonSource);
    JsonDocumentAndModel(const char *jsonFileName);

    const JsonValue& json();
    QUndoStack* undoStack();

public slots:
    void undo();
    void redo();

////////////////////////////////////////////////////////////////////////////////
/// интерфейс Document
public:
    void setModified(bool m = true);
    bool isModified();
    QString toPlainText() const;

signals:
    void contentsChanged();

////////////////////////////////////////////////////////////////////////////////
/// интерфейс Model
public:
    /// R/O model
    QModelIndex index(int row, int column,
                      const QModelIndex& index) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;

    /// Editable model
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    bool insertRows(int position, int rows,
                    const QModelIndex& parent = QModelIndex()) override;
    bool insertDefaultRow(int position, const QModelIndex& parent);
    bool removeRows(int position, int rows,
                    const QModelIndex& parent = QModelIndex()) override;

    /// дополнительные методы
    QModelIndex index(const JsonValue& value) const;
    bool itemIsContainer(const QModelIndex &index = QModelIndex()) const;

private:
    const JsonValue* getValue(const QModelIndex &index = QModelIndex()) const;

private:
    JsonValue m_jsonDocument;
    QUndoStack m_undoStack;
};

#endif // JSONMODEL_H
