#include "Gui/Spreadsheet/SpreadsheetViewModel.h"
#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/NodePacket.h"

namespace enzo::ui
{

SpreadsheetViewModel::SpreadsheetViewModel(QObject* parent) : QObject(parent)
{
    auto& network = nt::nm();

    // Selecting a node carries no geometry, so the packet is pulled from the
    // newly selected node.
    selectedNodesConnection_ = network.selectedNodesChanged.connect(
        [this](std::vector<nt::OpId> selectedNodeIds) {
            if (selectedNodeIds.empty())
            {
                showPacket(nullptr);
                return;
            }
            const nt::OpId selected = selectedNodeIds.back();
            showPacket(nt::nm().getGeoOperator(selected).getOutputPacket(0));
        }
    );

    // A recook of the selected node delivers fresh geometry directly.
    selectedGeoConnection_ = network.selectedGeoChanged.connect(
        [this](std::shared_ptr<const NodePacket> packet) { showPacket(packet); }
    );
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
