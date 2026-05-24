#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <vector>
#include "Engine/Types.h"
#include "Gui/Panels/Panel.h"

namespace enzo::nt { class GeometryOperator; }
namespace enzo::prm { class Template; }
namespace enzo::ui { class FormParm; }

class ParametersPanel
: public Panel
{
public:
    ParametersPanel(QWidget *parent = nullptr);
public Q_SLOTS:
    void selectionChanged(enzo::nt::OpId opId);
    void clearParameters();
private:
    QWidget* buildTemplateWidget(const enzo::prm::Template& templateEntry,
                                 enzo::nt::GeometryOperator& displayOp,
                                 std::vector<enzo::ui::FormParm*>& leafWidgets,
                                 int& maxLeftPadding);

    QVBoxLayout* mainLayout_;
    QVBoxLayout* parametersLayout_;
    QWidget* bgWidget_;

};
