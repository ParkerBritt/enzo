#include "Gui/Parameters/ParametersViewModel.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Parameter/Template.h"
#include "Gui/Parameters/ParameterItem.h"

namespace enzo::ui {

ParametersViewModel::ParametersViewModel(QObject* parent) : QObject(parent)
{
    primaryNodeSubscription_ =
        nt::nm().primaryNodeChanged.connect([this](std::optional<nt::NodeId> primaryId) {
            showNode(primaryId);
        });
}

void ParametersViewModel::showNode(std::optional<nt::NodeId> nodeId)
{
    nodeId_ = nodeId;

    // A disable or hide condition reads sibling values, so a parameter edit can
    // flip another's state. Follow the new node's change signal to refresh.
    parameterChangedSubscription_.disconnect();
    if (nodeId_)
    {
        nt::Node& node = nt::nm().getNode(*nodeId_);
        parameterChangedSubscription_ =
            node.parameterChanged.connect([this](const std::string&) { refreshConditions(); });
    }

    rebuild();
}

void ParametersViewModel::rebuild()
{
    clear();

    if (nodeId_)
    {
        nt::Node& node = nt::nm().getNode(*nodeId_);
        nodeName_ = QString::fromStdString(node.getName());
        nodeType_ = QString::fromStdString(node.getType().getLabel());
        for (const prm::Template& prmTemplate : node.getTemplates())
            topLevel_.append(buildItem(prmTemplate, node));
    }

    Q_EMIT parametersChanged();
}

ParameterItem* ParametersViewModel::buildItem(const prm::Template& prmTemplate, nt::Node& node)
{
    std::weak_ptr<prm::NodeParameter> parameter;
    if (!prmTemplate.isContainer()) parameter = node.getParameter(prmTemplate.getName());

    auto* item = new ParameterItem(prmTemplate, parameter, node, this);
    item->setMeta(
        node.isParameterEnabled(prmTemplate.getName()),
        node.isParameterHidden(prmTemplate.getName())
    );
    allItems_.append(item);

    if (prmTemplate.isContainer())
        for (const prm::Template& child : prmTemplate.getChildren())
            item->addChild(buildItem(child, node));

    return item;
}

void ParametersViewModel::refreshConditions()
{
    if (!nodeId_) return;
    nt::Node& node = nt::nm().getNode(*nodeId_);
    for (ParameterItem* item : allItems_)
    {
        std::string name = item->name().toStdString();
        item->setMeta(node.isParameterEnabled(name), node.isParameterHidden(name));
    }
}

void ParametersViewModel::clear()
{
    qDeleteAll(allItems_);
    allItems_.clear();
    topLevel_.clear();
    nodeName_.clear();
    nodeType_.clear();
}

} // namespace enzo::ui
