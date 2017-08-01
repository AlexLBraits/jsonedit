#ifndef JSONMODEL_H
#define JSONMODEL_H

#include <QAbstractItemModel>
#include <QUndoCommand>
#include <QUndoStack>
#include <value.h>

class JsonModel;

class JsonUndoCommand : public QUndoCommand
{
public:
    JsonUndoCommand(JsonModel* model,
                    const std::string& pointer);
    const JsonValue& target() const;

protected:
    JsonModel* m_model;
    std::string m_pointer;
};

class SetValueCommand : public JsonUndoCommand
{
public:
    SetValueCommand(JsonModel* model,
                    const std::string& pointer,
                    const JsonValue& oldValue,
                    const JsonValue& newValue);

    void redo() override;
    void undo() override;

private:
    JsonValue m_oldValue;
    JsonValue m_newValue;
};

class RemoveCommand : public JsonUndoCommand
{
public:
    RemoveCommand(JsonModel* model,
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


class JsonModel : public QAbstractItemModel
{
    friend class SetValueCommand;
    friend class RemoveCommand;
public:
    JsonModel(QObject *parent = Q_NULLPTR);
    JsonModel(const JsonValue& jsonDocument);
    JsonModel(const char *jsonFileName);

    const JsonValue& json() {return m_jsonDocument;}
    QUndoStack& undoStask() {return m_undoStack;}

////////////////////////////////////////////////////////////////////////////////
/// интерфейс Model
public:
    /// R/O model
    QModelIndex index(int row, int column, const QModelIndex& index) const override;
    QModelIndex index(const JsonValue& value) const;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& index) const override;
    int columnCount(const QModelIndex& index) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // Editable model
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
//    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;

private:
    const JsonValue *getValue(const QModelIndex &index = QModelIndex()) const;

private:
    JsonValue m_jsonDocument;
    QUndoStack m_undoStack;
};

#endif // JSONMODEL_H
