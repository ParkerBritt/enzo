#include "Gui/Parameters/ParametersViewModel.h"
#include "Engine/Network/GeometryOperator.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Parameter/Template.h"
#include "Gui/Parameters/ParameterItem.h"

namespace enzo::ui {

ParametersViewModel::ParametersViewModel(QObject* parent) : QObject(parent)
{
    primaryNodeSubscription_ =
        nt::nm().primaryNodeChanged.connect([this](std::optional<nt::OpId> primaryId) {
            showOperator(primaryId);
        });
}

void ParametersViewModel::showOperator(std::optional<nt::OpId> opId)
{
    opId_ = opId;

    // A disable or hide condition reads sibling values, so a parameter edit can
    // flip another's state. Follow the new operator's change signal to refresh.
    parameterChangedSubscription_.disconnect();
    if (opId_)
    {
        nt::GeometryOperator& op = nt::nm().getGeoOperator(*opId_);
        parameterChangedSubscription_ =
            op.parameterChanged.connect([this](const std::string&) { refreshConditions(); });
    }

    rebuild();
}

void ParametersViewModel::rebuild()
{
    clear();

    if (opId_)
    {
        nt::GeometryOperator& op = nt::nm().getGeoOperator(*opId_);
        nodeName_ = QString::fromStdString(op.getName());
        nodeType_ = QString::fromStdString(op.getType().getLabel());
        for (const prm::Template& prmTemplate : op.getTemplates())
            topLevel_.append(buildItem(prmTemplate, op));
    }

    Q_EMIT parametersChanged();
}

ParameterItem*
ParametersViewModel::buildItem(const prm::Template& prmTemplate, nt::GeometryOperator& op)
{
    std::weak_ptr<prm::NodeParameter> parameter;
    if (!prmTemplate.isContainer()) parameter = op.getParameter(prmTemplate.getName());

    auto* item = new ParameterItem(prmTemplate, parameter, this);
    item->setMeta(
        op.isParameterEnabled(prmTemplate.getName()),
        op.isParameterHidden(prmTemplate.getName())
    );
    allItems_.append(item);

    if (prmTemplate.isContainer())
        for (const prm::Template& child : prmTemplate.getChildren())
            item->addChild(buildItem(child, op));

    return item;
}

void ParametersViewModel::refreshConditions()
{
    if (!opId_) return;
    nt::GeometryOperator& op = nt::nm().getGeoOperator(*opId_);
    for (ParameterItem* item : allItems_)
    {
        std::string name = item->name().toStdString();
        item->setMeta(op.isParameterEnabled(name), op.isParameterHidden(name));
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
