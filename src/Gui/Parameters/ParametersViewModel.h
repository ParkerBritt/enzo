#pragma once

#include "Engine/Core/Types.h"
#include <QList>
#include <QObject>
#include <boost/signals2/connection.hpp>
#include <optional>

namespace enzo::nt {
class GeometryOperator;
}
namespace enzo::prm {
class Template;
}

namespace enzo::ui {

class ParameterItem;

/// @brief View-model backing the parameter panel.
///
/// Follows the primary node and exposes its parameters as a tree of items QML
/// binds to. This is the boundary that turns the engine boost signals into Qt
/// properties.
class ParametersViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QObject*> parameters READ parameters NOTIFY parametersChanged)
    Q_PROPERTY(bool hasNode READ hasNode NOTIFY parametersChanged)
    Q_PROPERTY(QString nodeName READ nodeName NOTIFY parametersChanged)
    Q_PROPERTY(QString nodeType READ nodeType NOTIFY parametersChanged)

  public:
    explicit ParametersViewModel(QObject* parent = nullptr);

    QList<QObject*> parameters() const { return topLevel_; }
    bool hasNode() const { return opId_.has_value(); }
    QString nodeName() const { return nodeName_; }
    QString nodeType() const { return nodeType_; }

  Q_SIGNALS:
    void parametersChanged();

  private:
    /// @brief Switches the panel to a node, or clears it when none.
    void showOperator(std::optional<nt::OpId> opId);

    /// @brief Rebuilds the item tree from the current operator.
    void rebuild();

    /// @brief Re-reads every item's enabled and hidden state.
    void refreshConditions();

    /// @brief Builds an item for a template and recurses into a group.
    ParameterItem* buildItem(const prm::Template& prmTemplate, nt::GeometryOperator& op);

    /// @brief Drops every item and resets the header.
    void clear();

    std::optional<nt::OpId> opId_;
    // The top level items QML iterates, and a flat list of every item used to
    // refresh conditions. Both hold the same objects, all parented to this.
    QList<QObject*> topLevel_;
    QList<ParameterItem*> allItems_;
    QString nodeName_;
    QString nodeType_;

    boost::signals2::scoped_connection primaryNodeSubscription_;
    boost::signals2::scoped_connection parameterChangedSubscription_;
};

} // namespace enzo::ui
