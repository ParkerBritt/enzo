#include <QVBoxLayout>
#include <QWidget>
#include <QMenuBar>

class HeaderBar
: public QWidget
{
public:
    HeaderBar();

private:
    void onFileNewClicked();
    void onFileOpenClicked();
    void onFileSaveAsClicked();

    QBoxLayout* mainLayout_;
};
