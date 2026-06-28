#include "Gui/Spreadsheet/SpreadsheetViewModel.h"
#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodePacket.h"

namespace enzo::ui {

SpreadsheetViewModel::SpreadsheetViewModel(QObject* parent) : QObject(parent)
{
    auto& network = nt::nm();

    // Switching the primary node carries no geometry, so the packet is pulled
    // from the new primary node.
    primaryNodeSubscription_ =
        network.primaryNodeChanged.connect([this](std::optional<nt::OpId> primaryId) {
            if (!primaryId.has_value())
            {
                showPacket(nullptr);
                return;
            }
            showPacket(nt::nm().getGeoOperator(*primaryId).getOutputPacket(0));
        });

    // A recook of the primary node delivers fresh geometry directly.
    primaryGeoSubscription_ =
        network.primaryGeoChanged.connect([this](std::shared_ptr<const NodePacket> packet) {
            showPacket(packet);
        });
}

QAbstractItemModel* SpreadsheetViewModel::tableModel() { return &tableModel_; }

QAbstractItemModel* SpreadsheetViewModel::primitiveTree() { return &primitiveTree_; }

SpreadsheetViewModel::Mode SpreadsheetViewModel::mode() const { return mode_; }

void SpreadsheetViewModel::setMode(Mode mode)
{
    if (mode_ == mode) return;
    mode_ = mode;
    tableModel_.setOwner(static_cast<attr::AttributeOwner>(static_cast<int>(mode_)));
    Q_EMIT modeChanged();
    Q_EMIT tableChanged();
}

int SpreadsheetViewModel::primitiveCount() const
{
    return packet_ ? static_cast<int>(packet_->size()) : 0;
}

int SpreadsheetViewModel::elementCount() const { return tableModel_.rowCount(); }

QString SpreadsheetViewModel::elementNoun() const
{
    switch (mode_)
    {
    case Points:
        return "points";
    case Vertices:
        return "vertices";
    case Faces:
        return "faces";
    case Primitive:
        return "primitive attrs";
    }
    return "";
}

void SpreadsheetViewModel::selectPrimitive(int index)
{
    if (!packet_ || index < 0 || index >= static_cast<int>(packet_->size()))
    {
        tableModel_.setPrimitive(nullptr);
        return;
    }
    tableModel_.setPrimitive(packet_->getPrimitive(index));
    Q_EMIT tableChanged();
}

void SpreadsheetViewModel::showPacket(std::shared_ptr<const NodePacket> packet)
{
    packet_ = std::move(packet);
    primitiveTree_.setPacket(packet_);
    selectPrimitive(packet_ && packet_->size() > 0 ? 0 : -1);
    Q_EMIT geometryChanged();
}

} // namespace enzo::ui
