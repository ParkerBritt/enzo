#pragma once

#include <QAbstractListModel>
#include "Engine/Types.h"
#include "Engine/Operator/Geometry.h"

class GeometrySpreadsheetModel : public QAbstractListModel
{
    Q_OBJECT

public:
    GeometrySpreadsheetModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    int indexFromSection(unsigned int section) const;

    void geometryChanged(enzo::geo::Geometry& geometry);
    void setOwner(const enzo::ga::AttributeOwner owner);
    void initBuffers();



private:
    enzo::nt::OpId opId_;
    enzo::geo::Geometry geometry_;
    std::vector<unsigned int> attribSizes_;
    std::vector<unsigned int> sectionAttribMap_;
    const int attributeColumnPadding_ = 1;
    enzo::ga::AttributeOwner attributeOwner_=enzo::ga::AttributeOwner::POINT;

};
