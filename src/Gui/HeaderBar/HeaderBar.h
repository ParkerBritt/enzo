#include <QVBoxLayout>
#include <QWidget>
#include <QMenuBar>

class HeaderBar
: public QWidget
{
public:
    HeaderBar();

private:
    void onSaveClicked();

    QBoxLayout* mainLayout_;
};
