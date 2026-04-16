#include "Engine/Parameter/Parameter.h"
#include <QWidget>
#include <QHBoxLayout>
#include <memory>
#include <qtmetamacros.h>
#include <QLabel>

namespace enzo::ui
{

class FormParm
: public QWidget
{
    Q_OBJECT
public:
    FormParm(std::weak_ptr<prm::Parameter> parameter);
    int getLeftPadding();
    void setLeftPadding(int padding);

private:
    QHBoxLayout* mainLayout_;
    QLabel* label_;

};

}
