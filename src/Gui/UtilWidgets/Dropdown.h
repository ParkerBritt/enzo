#pragma once

#include "Gui/UtilWidgets/PopupList.h"

#include <QIcon>
#include <QString>
#include <QWidget>
#include <qtmetamacros.h>
#include <vector>

class QPropertyAnimation;

namespace enzo::ui {

class DropdownPopup;

/**
 * @brief Reusable dropdown styled to match the other util widgets with a rounded
 * border and dark palette. Clicking the box reveals a popup list below it that
 * unrolls vertically to its full height.
 */
class Dropdown : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal arrowRotation READ arrowRotation WRITE setArrowRotation)
  public:
    Dropdown(QWidget* parent = nullptr);

    // The data string is an opaque value carried alongside the visible text,
    // letting callers work in their own value space (such as parameter tokens)
    // rather than item indices.
    void addItem(const QString& text, const QString& data = QString());

    int currentIndex() const { return currentIndex_; }
    QString currentText() const;
    QString currentData() const;
    void setCurrentIndex(int index);
    void setCurrentData(const QString& data);

    QSize sizeHint() const override;

  Q_SIGNALS:
    void currentIndexChanged(int index);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

  private:
    using Item = PopupList::Item;

    void openPopup();
    void animateArrow(qreal target);

    qreal arrowRotation() const { return arrowRotation_; }
    void setArrowRotation(qreal degrees)
    {
        arrowRotation_ = degrees;
        update();
    }

    std::vector<Item> items_;
    int currentIndex_ = -1;
    bool popupOpen_ = false;
    qreal arrowRotation_ = 0;

    DropdownPopup* popup_ = nullptr;

    friend class DropdownPopup;
};

/**
 * @brief Floating list shown beneath a Dropdown box.
 *
 * The list is populated from the owning box and shows every item without
 * filtering.
 */
class DropdownPopup : public PopupList
{
    Q_OBJECT
  public:
    DropdownPopup(Dropdown* owner);

    void openBeneath(QWidget* anchor, int selectedIndex);

  private:
    Dropdown* owner_;
};

} // namespace enzo::ui
