#include "jsonmodel.h"

JsonUndoCommand::JsonUndoCommand(JsonModel *model, const std::string &pointer)
    : m_model(model),
    m_pointer(pointer)
{
}

const JsonValue& JsonUndoCommand::target() const
{
    return m_model->json().evalPointer(m_pointer);
}

SetValueCommand::SetValueCommand(
    JsonModel* model,
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
    if(!obj.isEmptyValue())
    {
        (JsonValue&)obj = m_newValue;

        QModelIndex index = m_model->index(obj);
        emit m_model->dataChanged(
            index,
            index.sibling(
                m_model->rowCount(index),
                m_model->columnCount(index)
            )
        );
    }
}

void SetValueCommand::undo()
{
    const JsonValue& obj = target();
    if(!obj.isEmptyValue())
    {
        (JsonValue&)obj = m_oldValue;

        QModelIndex index = m_model->index(obj);
        emit m_model->dataChanged(
            index,
            index.sibling(
                m_model->rowCount(index),
                m_model->columnCount(index)
            )
        );
    }
}

RemoveCommand::RemoveCommand(JsonModel *model,
    const std::string &pointer,
    int position,
    const JsonValue &key,
    const JsonValue &value
) : JsonUndoCommand(model, pointer),
    m_pos(position),
    m_key(key),
    m_value(value)
{
}

void RemoveCommand::redo()
{
    const JsonValue& obj = target();
    if(!obj.isEmptyValue())
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
    if(!obj.isEmptyValue())
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

////////////////////////////////////////////////////////////////////////////////
///
/// Class JsonModel implementation
///
#define JSON_MODEL_COLUMN_KEY   0
#define JSON_MODEL_COLUMN_TYPE  1
#define JSON_MODEL_COLUMN_VALUE 2

const QStringList g_columnHeaders = {"Key", "Type", "Value"};

JsonModel::JsonModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_jsonDocument = parse_string("{\"A\":0}");
}

JsonModel::JsonModel(const JsonValue& jsonDocument)
{
    m_jsonDocument = jsonDocument;
}

JsonModel::JsonModel(const char* jsonFileName)
{
    m_jsonDocument = parse_file(jsonFileName);
}

////////////////////////////////////////////////////////////////////////////////
///
/// интерфейс Model
///
QModelIndex JsonModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    const JsonValue* v = getValue(parent);
    return QAbstractItemModel::createIndex(row, column, (void*)&(v->at(row)));
}

QModelIndex JsonModel::index(const JsonValue& value) const
{
    if(value.root() != &m_jsonDocument) return QModelIndex();
    if(&value == &m_jsonDocument)  return QModelIndex();
    return QAbstractItemModel::createIndex(value.pos(), 0, (void*)&(value));
}

QModelIndex JsonModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) return QModelIndex();

    const JsonValue* childItem = getValue(index);

    const JsonValue* parentItem = childItem->parent();
    if (parentItem == 0) return QModelIndex();
    if (parentItem == &m_jsonDocument) return QModelIndex();
    return QAbstractItemModel::createIndex(parentItem->pos(), 0, (void*)parentItem);
}

int JsonModel::rowCount(const QModelIndex& index) const
{
    if (index.column() > 0) return 0;
    const JsonValue* v = getValue(index);
    return v->size();
}

int JsonModel::columnCount(const QModelIndex& index) const
{
    (void)(index);
    return g_columnHeaders.size();
}

QVariant JsonModel::data(const QModelIndex &index, int role) const
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
            case JsonValue::Type::ARRAY:
                return QVariant(QString::fromStdString(v->asString()));
            case JsonValue::Type::STRING:
                return QVariant("string");
            case JsonValue::Type::NUMBER:
            case JsonValue::Type::INTEGER:
                return QVariant("number");
            default:
                return QVariant();
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
    else
    {
        return QVariant();
    }
}

QVariant JsonModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return g_columnHeaders[section];

    return QVariant();
}

Qt::ItemFlags JsonModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return 0;

    JsonValue& value = (JsonValue&)*getValue(index);
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    switch(index.column())
    {
    case JSON_MODEL_COLUMN_VALUE:
        switch(value.type())
        {
        case JsonValue::Type::OBJECT:
        case JsonValue::Type::ARRAY:
            return flags;
        default:
            return flags | Qt::ItemIsEditable;
        }
    default:
        return flags;
    }
}

bool JsonModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) return false;

    JsonValue& oldValue = (JsonValue&)*getValue(index);
    QString key = QString::fromStdString(oldValue.key().asString());
    JsonValue newValue = JsonValue(value.toString().toStdString().c_str(), 0);

    switch(oldValue.type())
    {
    case JsonValue::Type::UNDEFINED:
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

    return true;
}

bool JsonModel::removeRows(int position, int rows, const QModelIndex &index)
{
    (void)(rows);

    JsonValue* parentItem = (JsonValue*)getValue(index);
    const JsonValue& item = parentItem->at(position);

    m_undoStack.push(
        new RemoveCommand(
            this,
            parentItem->getPointer(),
            position,
            item.key(),
            item
        )
    );

    return true;
}

const JsonValue* JsonModel::getValue(const QModelIndex& index) const
{
    return index.isValid() ?
           (JsonValue*)(index.internalPointer()) :
           &m_jsonDocument;
}
