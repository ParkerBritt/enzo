#pragma once

#include <QAbstractListModel>
#include "Engine/Types.h"
#include "Engine/Operator/Mesh.h"

class AttributeSpreadsheetModel : public QAbstractListModel
{
    Q_OBJECT

public:
    AttributeSpreadsheetModel(QObject *parent = nullptr);

    void clear();
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    int indexFromSection(unsigned int section) const;

    void primitiveChanged(std::shared_ptr<const enzo::geo::Primitive> primitive);
    void setOwner(const enzo::ga::AttributeOwner owner);
    void initBuffers();



private:
    enzo::nt::OpId opId_;
    std::shared_ptr<const enzo::geo::Primitive> primitive_;
    std::vector<unsigned int> attribSizes_;
    std::vector<unsigned int> sectionAttribMap_;
    const int attributeColumnPadding_ = 1;
    enzo::ga::AttributeOwner attributeOwner_=enzo::ga::AttributeOwner::POINT;

};
