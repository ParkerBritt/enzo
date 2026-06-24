#pragma once

#include "LegacyGui/UtilWidgets/Menu.h"

#include <QBoxLayout>
#include <QString>
#include <QWidget>
#include <vector>

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
    void saveFile(const QString& filePath);
    void openFile(const QString& filePath);

    std::vector<enzo::ui::Menu::Entry> fileEntries();
    std::vector<enzo::ui::Menu::Entry> recentFileEntries();

    QBoxLayout* mainLayout_;
    QString currentFilePath_;
};
