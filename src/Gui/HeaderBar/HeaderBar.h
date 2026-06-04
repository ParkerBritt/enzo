#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QWidget>

class HeaderBar : public QWidget
{
  public:
    HeaderBar();

  private:
    void onFileNewClicked();
    void onFileOpenClicked();
    void onFileSaveClicked();
    void onFileSaveAsClicked();

    void addRecentFile(const QString& filePath);
    void updateRecentFilesMenu();
    void saveFile(const QString& filePath);
    void openFile(const QString& filePath);

    QBoxLayout* mainLayout_;
    QMenu* recentFilesMenu_;
    QString currentFilePath_;
};
