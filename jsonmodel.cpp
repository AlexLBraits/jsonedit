#include "jsonmodel.h"
#include <QBrush>
#include <QPalette>

JsonUndoCommand::JsonUndoCommand(JsonDocumentAndModel *model, const std::string &pointer)
    : m_model(model),
      m_pointer(pointer)
{
}

const JsonValue& JsonUndoCommand::target() const
{
    return m_model->json().evalPointer(m_pointer);
}

SetKeyCommand::SetKeyCommand(
    JsonDocumentAndModel* model,
    const std::string& pointer,
    int position,
    const std::string& oldValue,
    const std::string& newValue
) : JsonUndoCommand(model, pointer),
    m_position(position),
    m_oldKey(oldValue),
    m_newKey(newValue)
{
}

void SetKeyCommand::redo()
{
    JsonValue& parent = (JsonValue&)this->target();
    if(parent.isDummyValue()) return;
    if(parent.hasKey(m_newKey)) return;

    JsonValue saveValue = parent[m_oldKey];

    m_model->beginRemoveRows(m_model->index(parent), m_position, m_position);
    parent.erase(m_oldKey);
    m_model->endRemoveRows();

    m_model->beginInsertRows(m_model->index(parent), m_position, m_position);
    parent.insert(m_position, m_newKey, saveValue);
    m_model->endInsertRows();
}

void SetKeyCommand::undo()
{
    JsonValue& parent = (JsonValue&)this->target();
    if(parent.isDummyValue()) return;

    JsonValue saveValue = parent[m_newKey];

    m_model->beginRemoveRows(m_model->index(parent), m_position, m_position);
    parent.erase(m_newKey);
    m_model->endRemoveRows();

    m_model->beginInsertRows(m_model->index(parent), m_position, m_position);
    parent.insert(m_position, m_oldKey, saveValue);
    m_model->endInsertRows();
}

SetValueCommand::SetValueCommand(
    JsonDocumentAndModel* model,
    const std::string& pointer,
    const JsonValue& oldValue,
    const JsonValue& newValue
) : JsonUndoCommand(model, pointer),
    m_oldValue(oldValue),
    m_newValue(newValue)
{
}

void SetValueCommand::redo()
{
    const JsonValue& obj = target();
    if(!obj.isDummyValue())
    {
        QModelIndex index = m_model->index(obj);
        if(m_newValue.size())
        {
            m_model->beginInsertRows(index, 0, m_newValue.size() - 1);
            (JsonValue&)obj = m_newValue;
            m_model->endInsertRows();
        }
        else
        {
            (JsonValue&)obj = m_newValue;
            emit m_model->dataChanged(
                index,
                index.sibling(
                    m_model->rowCount(index),
                    m_model->columnCount(index)
                )
            );
        }
    }
}

void SetValueCommand::undo()
{
    const JsonValue& obj = target();
    if(!obj.isDummyValue())
    {
        QModelIndex index = m_model->index(obj);
        if(m_newValue.size())
        {
            m_model->beginRemoveRows(index, 0, m_newValue.size() - 1);
            (JsonValue&)obj = m_oldValue;
            m_model->endRemoveRows();
        }
        else
        {
            (JsonValue&)obj = m_oldValue;
            emit m_model->dataChanged(
                index,
                index.sibling(
                    m_model->rowCount(index),
                    m_model->columnCount(index)
                )
            );
        }
    }
}

RemoveCommand::RemoveCommand(
    JsonDocumentAndModel *model,
    const std::string &pointer,
    int position,
    const JsonValue &key,
    const JsonValue &value
)
    : JsonUndoCommand(model, pointer),
      m_pos(position),
      m_key(key),
      m_value(value)
{
}

void RemoveCommand::redo()
{
    const JsonValue& obj = target();
    if(!obj.isDummyValue())
    {
        QModelIndex index = m_model->index(obj);
        m_model->beginRemoveRows(index, m_pos, m_pos);

        ((JsonValue&)obj).erase(m_key);

        m_model->endRemoveRows();
    }
}

void RemoveCommand::undo()
{
    const JsonValue& obj = target();
    if(!obj.isDummyValue())
    {
        QModelIndex index = m_model->index(obj);
        m_model->beginInsertRows(index, m_pos, m_pos);

        switch(obj.type())
        {
        case JsonValue::Type::OBJECT:
            ((JsonValue&)obj).insert(m_pos, m_key.asString(), m_value);
            break;

        case JsonValue::Type::ARRAY:
            ((JsonValue&)obj).insert(m_pos, m_value);
            break;

        default:
            break;
        }

        m_model->endInsertRows();
    }
}

InsertCommand::InsertCommand(
    JsonDocumentAndModel *model,
    const std::string &pointer,
    int position)
    : JsonUndoCommand(model, pointer),
      m_pos(position)
{
}

void InsertCommand::redo()
{
    JsonValue& parentValue = (JsonValue&)target();

    m_model->beginInsertRows(m_model->index(parentValue), m_pos, m_pos);
    switch(parentValue.type())
    {
    case JsonValue::Type::ARRAY:
        parentValue.insert(m_pos, JsonValue());
        break;
    case JsonValue::Type::OBJECT:
    {
        std::string key = "New Key";
        if(parentValue.hasKey(key))
        {
            int n = 2;
            do
            {
                key = std::string("New Key (") + std::to_string(n++) + ")";
            }
            while(parentValue.hasKey(key));
        }
        parentValue.insert(m_pos, key, JsonValue());
        break;
    }
    default:
        break;
    }
    m_model->endInsertRows();
}

void InsertCommand::undo()
{
    JsonValue& parentValue = (JsonValue&)target();

    m_model->beginRemoveRows(m_model->index(parentValue), m_pos, m_pos);
    parentValue.erase(parentValue.at(m_pos).key());
    m_model->endRemoveRows();
}


////////////////////////////////////////////////////////////////////////////////
///
/// Class JsonModel implementation
///
#define JSON_MODEL_COLUMN_KEY   0
#define JSON_MODEL_COLUMN_TYPE  1
#define JSON_MODEL_COLUMN_VALUE 2

const QStringList g_columnHeaders = {"Key", "Type", "Value"};

JsonDocumentAndModel::JsonDocumentAndModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_jsonDocument = JsonValue(JsonValue::Type::OBJECT);

    connect(
        &m_undoStack,
        &QUndoStack::canUndoChanged,
        [this](bool canUndo) {
        if(canUndo)
            emit contentsChanged();
    });
}

JsonDocumentAndModel::JsonDocumentAndModel(const JsonValue& jsonDocument)
    : JsonDocumentAndModel()
{
    m_jsonDocument = jsonDocument;
}

JsonDocumentAndModel::JsonDocumentAndModel(const std::string& jsonSource)
    : JsonDocumentAndModel()
{
    m_jsonDocument = parse_string(jsonSource.c_str());
}

JsonDocumentAndModel::JsonDocumentAndModel(const char* jsonFileName)
    : JsonDocumentAndModel()
{
    m_jsonDocument = parse_file(jsonFileName);
}

const JsonValue &JsonDocumentAndModel::json()
{
    return m_jsonDocument;
}

QUndoStack *JsonDocumentAndModel::undoStack()
{
    return &m_undoStack;
}

void JsonDocumentAndModel::undo()
{
    m_undoStack.undo();
}

void JsonDocumentAndModel::redo()
{
    m_undoStack.redo();
}

////////////////////////////////////////////////////////////////////////////////
///
/// интерфейс Document
///
void JsonDocumentAndModel::setModified(bool m)
{

}

bool JsonDocumentAndModel::isModified()
{
    return m_undoStack.canUndo();
}

QString JsonDocumentAndModel::toPlainText() const
{
    return QString::fromStdString(stringify(m_jsonDocument));
}

////////////////////////////////////////////////////////////////////////////////
///
/// интерфейс Model
///
QModelIndex JsonDocumentAndModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();

    const JsonValue* parentItem = getValue(parent);
    const JsonValue& childItem = parentItem->at(row);

    if(childItem.isDummyValue())
        return QModelIndex();
    else
        return QAbstractItemModel::createIndex(row, column, (void*)&childItem);
}

QModelIndex JsonDocumentAndModel::index(const JsonValue& value) const
{
    if(value.root() != &m_jsonDocument) return QModelIndex();
    if(&value == &m_jsonDocument)  return QModelIndex();
    return createIndex(value.pos(), 0, (void*)&value);
}

bool JsonDocumentAndModel::itemIsContainer(const QModelIndex &index) const
{
    return getValue(index)->isObject() || getValue(index)->isArray();
}

QModelIndex JsonDocumentAndModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) return QModelIndex();

    const JsonValue* childItem = getValue(index);
    const JsonValue* parentItem = childItem->parent();

    if (parentItem == 0) return QModelIndex();
    if (parentItem == &m_jsonDocument) return QModelIndex();

    return createIndex(parentItem->pos(), 0, (void*)parentItem);
}

int JsonDocumentAndModel::rowCount(const QModelIndex& index) const
{
    if (index.column() > 0) return 0;
    const JsonValue* v = getValue(index);
    return v->size();
}

int JsonDocumentAndModel::columnCount(const QModelIndex& index) const
{
    (void)(index);
    return g_columnHeaders.size();
}

QVariant JsonDocumentAndModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        const JsonValue* v = getValue(index);

        switch(index.column())
        {
        case JSON_MODEL_COLUMN_KEY:
            return QVariant(QString::fromStdString(v->key().asString()));
        case JSON_MODEL_COLUMN_TYPE:
            switch(v->type())
            {
            case JsonValue::Type::OBJECT:
                return QString("object{%1}").arg(v->size());
            case JsonValue::Type::ARRAY:
                return QString("array[%1]").arg(v->size());
            case JsonValue::Type::STRING:
                return QVariant("string");
            case JsonValue::Type::NUMBER:
            case JsonValue::Type::INTEGER:
                return QVariant("number");
            case JsonValue::Type::BOOLEAN:
                return QVariant("bool");
            default:
                return QVariant("?");
            }
        case JSON_MODEL_COLUMN_VALUE:
            switch(v->type())
            {
            case JsonValue::Type::OBJECT:
            case JsonValue::Type::ARRAY:
                return QVariant();
            default:
                return QVariant(QString::fromStdString(v->asString()));
            }
        default:
            return QVariant();
        }
    }
//    else if(role == Qt::BackgroundRole)
//    {
//        QPalette pal;
//        return QVariant(QBrush(pal.brush(QPalette::ToolTipBase)));
//    }
    else
    {
        return QVariant();
    }
}

QVariant JsonDocumentAndModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return g_columnHeaders[section];

    return QVariant();
}

Qt::ItemFlags JsonDocumentAndModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return 0;

    JsonValue& value = (JsonValue&)*getValue(index);
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    switch(index.column())
    {
    case JSON_MODEL_COLUMN_KEY:
    {
        JsonValue& parent = (JsonValue&)*getValue(this->parent(index));
        if(parent.type() == JsonValue::Type::OBJECT)
        {
            flags |= Qt::ItemIsEditable;
        }
    }
    break;

    case JSON_MODEL_COLUMN_VALUE:
        switch(value.type())
        {
        case JsonValue::Type::OBJECT:
        case JsonValue::Type::ARRAY:
            break;
        default:
            flags |= Qt::ItemIsEditable;
            break;
        }
    default:
        break;
    }

    return flags;
}

bool JsonDocumentAndModel::setData(const QModelIndex &index, const QVariant &value,
                                   int role)
{
    if (role != Qt::EditRole) return false;

    JsonValue& oldValue = (JsonValue&)*getValue(index);
    JsonValue newValue = JsonValue(value.toString().toStdString().c_str(), 0);

    switch(index.column())
    {
    case JSON_MODEL_COLUMN_KEY:
    {
        ///
        /// переименование ключа у члена объекта
        ///
        const JsonValue* parentValue = oldValue.parent();
        ///
        /// если член с ключом newValue уже есть в объекте, то уходим
        ///
        if(parentValue->hasKey(newValue.asString())) return false;
        m_undoStack.push(
            new SetKeyCommand(
                this,
                parentValue->getPointer(),
                oldValue.pos(),
                oldValue.key().asString(),
                newValue.asString()
            )
        );
    }
    break;

    case JSON_MODEL_COLUMN_VALUE:
        switch(oldValue.type())
        {
        case JsonValue::Type::UNDEFINED:
            newValue = parse_string(value.toString().toStdString().c_str());
            if(newValue.isUndefined())
                newValue = JsonValue(value.toString().toStdString().c_str(), 0);
            break;
        case JsonValue::Type::BOOLEAN:
            newValue = newValue.asBoolean();
            break;
        case JsonValue::Type::NUMBER:
            newValue = newValue.asNumber();
            break;
        case JsonValue::Type::INTEGER:
            newValue = newValue.asInt();
            break;
        case JsonValue::Type::STRING:
            newValue = newValue.asString();
            break;
        default:
            break;
        }

        m_undoStack.push(
            new SetValueCommand(
                this,
                oldValue.getPointer(),
                oldValue,
                newValue
            )
        );
        break;

    default:
        break;
    }

    return true;
}

bool JsonDocumentAndModel::insertRows(int position, int rows, const QModelIndex& parent)
{
    (void)(position);
    (void)(rows);
    (void)(parent);

    return false;
}

bool JsonDocumentAndModel::insertDefaultRow(int position, const QModelIndex &parent)
{
    const JsonValue* parentItem = getValue(parent);

    m_undoStack.push(
        new InsertCommand(
            this,
            parentItem->getPointer(),
            position
        )
    );

    return true;
}

bool JsonDocumentAndModel::removeRows(int position, int rows, const QModelIndex& parent)
{
    (void)(rows);

    const JsonValue* parentItem = getValue(parent);
    const JsonValue& childItem = parentItem->at(position);

    m_undoStack.push(
        new RemoveCommand(
            this,
            parentItem->getPointer(),
            position,
            childItem.key(),
            childItem
        )
    );

    return true;
}

const JsonValue* JsonDocumentAndModel::getValue(const QModelIndex& index) const
{
    return index.isValid() && index.internalPointer() ?
           (JsonValue*)(index.internalPointer()) :
           &m_jsonDocument;
}
