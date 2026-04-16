#pragma once

#include "Engine/Operator/NodePacket.h"
#include "Engine/Operator/Primitive.h"
#include <QString>
#include <math.h>
#include <string.h>
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
        // Index 0 is always the "Free Cam", cameras start at index 1.
        int size = 1;
        if(packet_)
        {
            size += static_cast<int>(packet_->getPrimitives(enzo::geo::PrimType::CAMERA).size());
        }
        return size;
    }

    QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const override
    {
        const QVariant freeCamText = "Free Cam";

        if(!index.isValid()) return QVariant();

        if(role == Qt::DisplayRole)
        {
            const int row = index.row();
            if(row == 0) return freeCamText;

            if(!packet_) return QVariant();
            const auto cameras = packet_->getPrimitives(enzo::geo::PrimType::CAMERA);
            const int camIdx = row - 1;
            if(camIdx < 0 || camIdx >= static_cast<int>(cameras.size())) return QVariant();

            return QString::fromStdString(cameras[camIdx]->getPath());
        }

        return QVariant();
    }

    private:
        std::shared_ptr<const enzo::NodePacket> packet_;
};
