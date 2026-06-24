#pragma once

#include "Engine/Core/Types.h"
#include "LegacyGui/Panels/Panel.h"
#include <QVBoxLayout>
#include <QWidget>
#include <boost/signals2/connection.hpp>
#include <vector>

namespace enzo::nt {
class GeometryOperator;
}
namespace enzo::prm {
class Template;
}
namespace enzo::ui {
class Parameter;
}

class QLabel;
class QScrollArea;

class ParametersPanel : public Panel
{
  public:
    ParametersPanel(QWidget* parent = nullptr);
  public Q_SLOTS:
    void selectionChanged(enzo::nt::OpId opId);
    void clearParameters();

  private:
    enzo::ui::Parameter* buildTemplateWidget(
        const enzo::prm::Template& templateEntry,
        enzo::nt::GeometryOperator& displayOp,
        std::vector<enzo::ui::Parameter*>& leafWidgets,
        int& maxLeftPadding
    );

    /// @brief Greys out each leaf whose disableWhen condition currently holds.
    void refreshEnabledStates(enzo::nt::GeometryOperator& op);

    QVBoxLayout* mainLayout_;
    QVBoxLayout* parametersLayout_;
    QScrollArea* scrollArea_;
    QWidget* bgWidget_;
    QLabel* noSelectionLabel_;

    // Leaf widgets of the selected node, kept so their enabled state can refresh.
    std::vector<enzo::ui::Parameter*> leafWidgets_;
    // Refreshes enabled state while the selected node's parameters change.
    boost::signals2::scoped_connection parameterChangedConnection_;
};
