#pragma once

#include "Engine/Operator/NodePacket.h"
#include "Engine/Operator/Primitive.h"
#include <QString>
#include <memory>
#include <qabstractitemmodel.h>
#include <qnamespace.h>
#include <qvariant.h>

class ViewportCamerasModel
: public QAbstractListModel
{
    public:
    ViewportCamerasModel() : QAbstractListModel() {};

    void setPacket(std::shared_ptr<const enzo::NodePacket> packet)
    {
        beginResetModel();
        packet_ = std::move(packet);
        endResetModel();
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        if(!packet_) return 0;
        return static_cast<int>(packet_->getPrimitives(enzo::geo::PrimType::CAMERA).size());
    }

    QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const override
    {
        if(!packet_ || !index.isValid()) return QVariant();
        if(role != Qt::DisplayRole) return QVariant();

        const auto cameras = packet_->getPrimitives(enzo::geo::PrimType::CAMERA);
        const int row = index.row();
        if(row < 0 || row >= static_cast<int>(cameras.size())) return QVariant();

        return QString::fromStdString(cameras[row]->getPath());
    }

    private:
        std::shared_ptr<const enzo::NodePacket> packet_;
};
