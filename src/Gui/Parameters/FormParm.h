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

protected Q_SLOTS:
    void changeValue(bt::intT value, unsigned int index=0);
    void changeValue(bt::floatT value, unsigned int index=0);
    void changeValue(bt::String value, unsigned int index=0);

private:
    QHBoxLayout* mainLayout_;
    QLabel* label_;
    std::weak_ptr<prm::Parameter> parameter_;

};

}
