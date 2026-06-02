#pragma once

#include "Engine/Core/Types.h"
#include "Gui/Panels/Panel.h"
#include <QVBoxLayout>
#include <QWidget>
#include <vector>

namespace enzo::nt { class GeometryOperator; }
namespace enzo::prm { class Template; }
namespace enzo::ui { class Parameter; }

class ParametersPanel
: public Panel
{
public:
    ParametersPanel(QWidget *parent = nullptr);
public Q_SLOTS:
    void selectionChanged(enzo::nt::OpId opId);
    void clearParameters();
private:
    enzo::ui::Parameter* buildTemplateWidget(const enzo::prm::Template& templateEntry,
                                             enzo::nt::GeometryOperator& displayOp,
                                             std::vector<enzo::ui::Parameter*>& leafWidgets,
                                             int& maxLeftPadding);

    QVBoxLayout* mainLayout_;
    QVBoxLayout* parametersLayout_;
    QWidget* bgWidget_;
};
