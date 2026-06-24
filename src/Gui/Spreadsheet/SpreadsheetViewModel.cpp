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

void SpreadsheetViewModel::selectPrimitive(int index)
{
    if (!packet_ || index < 0 || index >= static_cast<int>(packet_->size()))
    {
        tableModel_.setPrimitive(nullptr);
        return;
    }
    tableModel_.setPrimitive(packet_->getPrimitive(index));
}

void SpreadsheetViewModel::showPacket(std::shared_ptr<const NodePacket> packet)
{
    packet_ = std::move(packet);
    primitiveTree_.setPacket(packet_);
    selectPrimitive(packet_ && packet_->size() > 0 ? 0 : -1);
}

} // namespace enzo::ui
