#pragma once

#include "Engine/Operator/NodePacket.h"
#include <memory>
#include <qabstractitemmodel.h>
#include <qnamespace.h>
#include <qvariant.h>

class ViewportCamerasModel
: public QAbstractListModel
{
    public:
    ViewportCamerasModel() : QAbstractListModel() {

    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return 5;
    }

    QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const override
    {
        if(role==Qt::DisplayRole)
        {
            return "test";
        }
        return QVariant();
    }

    private:
        std::shared_ptr<enzo::NodePacket> NodePacket();

};
