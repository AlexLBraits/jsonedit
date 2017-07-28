#ifndef JSONMODEL_H
#define JSONMODEL_H

#include <QAbstractItemModel>

class JsonModel : public QAbstractItemModel
{
public:
    JsonModel(QObject *parent = Q_NULLPTR);

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};

#endif // JSONMODEL_H
