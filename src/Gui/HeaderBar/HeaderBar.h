#pragma once

#include "Gui/UtilWidgets/Menu.h"

#include <QBoxLayout>
#include <QString>
#include <QWidget>
#include <functional>
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

    /// @brief Adds a top bar button that opens a menu built fresh on each click.
    void addMenuButton(
        const QString& title,
        std::function<std::vector<enzo::ui::Menu::Entry>()> build
    );

    std::vector<enzo::ui::Menu::Entry> fileEntries();
    std::vector<enzo::ui::Menu::Entry> recentFileEntries();

    QBoxLayout* mainLayout_;
    enzo::ui::Menu* menu_;
    QString currentFilePath_;
};
