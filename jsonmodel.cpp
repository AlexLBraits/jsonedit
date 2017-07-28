#include "jsonmodel.h"

JsonModel::JsonModel(QObject *parent)
    : QAbstractItemModel(parent)
{

}

QModelIndex JsonModel::index(int row, int column, const QModelIndex &parent) const
{
    return QModelIndex();
}

QModelIndex JsonModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

int JsonModel::rowCount(const QModelIndex &parent) const
{
    return 10;
}

int JsonModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant JsonModel::data(const QModelIndex &index, int role) const
{
    return QVariant();
}
