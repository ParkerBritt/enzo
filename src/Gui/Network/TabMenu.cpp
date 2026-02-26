#include "Gui/Network/TabMenu.h"
#include "Engine/Operator/OperatorTable.h"
#include "Gui/Network/NetworkPanel.h"
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QLineEdit>
#include <QPainterPath>
#include <QPushButton>
#include <QStyleOptionFrame>
#include <QSvgRenderer>
#include <QSvgWidget>
#include <iostream>
#include <qapplication.h>
#include <qboxlayout.h>
#include <qevent.h>
#include <qlineedit.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpushbutton.h>
#include <qscrollarea.h>
#include <qwidget.h>
#include <stdexcept>
#include <string>

enzo::ui::TabMenu::TabMenu(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f) {
  setAttribute(Qt::WA_TranslucentBackground);
  setWindowFlag(Qt::Popup);

  mainLayout_ = new QVBoxLayout(this);

  searchBar_ = new QLineEdit();
  nodeHolder_ = new QWidget();
  nodeScrollArea_ = new QScrollArea();
  nodeHolderLayout_ = new QVBoxLayout();

  connect(searchBar_, &QLineEdit::textChanged, this, &TabMenu::textChanged);

  auto tableItems = enzo::op::OperatorTable::getData();
  for (auto tableItem : tableItems) {
    auto button = new TabMenuButton(tableItem.displayName.c_str());
    buttons_.push_back(button);
    button->nodeName = tableItem.internalName;
    button->setFocusPolicy(Qt::NoFocus);
    connect(button, &TabMenuButton::clicked, this,
            &enzo::ui::TabMenu::nodeClicked);
    nodeHolderLayout_->addWidget(button);
  }

  setLayout(mainLayout_);

  // set focus policy
  searchBar_->setFocusPolicy(Qt::NoFocus);
  nodeScrollArea_->setFocusPolicy(Qt::NoFocus);

  // disable frames
  searchBar_->setFrame(false);
  nodeScrollArea_->setFrameStyle(QStyleOptionFrame::None);

  mainLayout_->addWidget(searchBar_);
  mainLayout_->addWidget(nodeScrollArea_);

  nodeHolderLayout_->setSpacing(0);
  nodeHolderLayout_->setContentsMargins(2, 4, 2, 4);
  nodeHolder_->setLayout(nodeHolderLayout_);
  nodeHolderLayout_->setAlignment(Qt::AlignTop);

  nodeScrollArea_->setWidget(nodeHolder_);
  nodeScrollArea_->setWidgetResizable(true);
  nodeScrollArea_->setMinimumHeight(250);
  nodeScrollArea_->setMinimumWidth(200);

  // style
  searchBar_->setAlignment(Qt::AlignCenter);
  nodeHolder_->setProperty("shade", "dark");
  nodeHolder_->setObjectName("TabMenuNodeHolder");
  nodeHolder_->setStyleSheet(R"(
           QWidget#TabMenuNodeHolder {
                background-color: transparent;
           }
    )");
  nodeScrollArea_->setObjectName("TabMenuNodeScrollArea");
  nodeScrollArea_->setStyleSheet(R"(
           QWidget#TabMenuNodeScrollArea {
                background-color: #d9161616;
                border: 1px solid #363636;
               border-radius: 8px;
           }
    )");
  searchBar_->setObjectName("TabMenuSearch");
  searchBar_->setStyleSheet(R"(
           QWidget#TabMenuSearch {
                background-color: #d9161616;
              padding: 3px;
               border-radius: 6px;
                border: 1px solid #363636;
           }
    )");

  // drop shadow
  // auto dropShadow_ = new QGraphicsDropShadowEffect();
  // dropShadow_->setColor(QColor(0,0,0,80));
  // dropShadow_->setBlurRadius(15);
  // dropShadow_->setOffset(4);
  // setGraphicsEffect(dropShadow_);

  setDisabled(true);
}

void enzo::ui::TabMenu::textChanged(const QString &text) {
  selectionIndex_ = 0;
  bool selectionMade = false;
  visibleButtons_.clear();
  for (size_t i = 0; i < buttons_.size(); ++i) {
    auto button = buttons_.at(i);
    if (text == "" ||
        button->getDisplayText().toLower().contains(text.toLower())) {
      // make selection
      if (!selectionMade) {
        std::cout << "selecting: " << button->getDisplayText().toStdString()
                  << "\n";
        button->setSelected(true);
      } else {
        std::cout << "deselecting: " << button->getDisplayText().toStdString()
                  << "\n";
        button->setSelected(false);
      }
      visibleButtons_.push_back(button);
      button->setVisible(true);
      selectionMade = true;
    } else {
      button->setVisible(false);
    }
  }
}

void enzo::ui::TabMenu::createNode(std::string nodeName) {
  // get node info
  std::optional<op::OpInfo> opInfo = op::OperatorTable::getOpInfo(nodeName);
  // check valid result
  if (!opInfo.has_value()) {
    throw std::runtime_error("Couldn't find op info for: " + nodeName);
  }

  static_cast<NetworkPanel *>(parentWidget())->createNode(opInfo.value());
}

void enzo::ui::TabMenu::nodeClicked() {
  // get name of button clicked
  TabMenuButton *buttonClicked = static_cast<TabMenuButton *>(sender());

  createNode(buttonClicked->nodeName);

  doHide();
}

void enzo::ui::TabMenu::showOnMouse(float dx, float dy) {
  setDisabled(false);
  std::cout << "showing\n";
  QPoint cursorPos = mapToParent(mapFromGlobal(QCursor::pos()));
  searchBar_->clear();
  textChanged("");
  move(cursorPos + QPoint(dx, dy));
  show();
  adjustSize();
  setFocus();
  raise();
}

void enzo::ui::TabMenu::doHide() {
  setDisabled(true);
  hide();
}

void enzo::ui::TabMenu::focusOutEvent(QFocusEvent *event) {
  QWidget::focusOutEvent(event);
  doHide();
}

bool enzo::ui::TabMenu::event(QEvent *event) {
  if ((event->type() == QEvent::KeyPress ||
       event->type() == QEvent::KeyRelease) &&
      event->spontaneous()) {
    auto *clone = static_cast<QKeyEvent *>(event)->clone();

    if (event->type() == QEvent::KeyPress) {
      auto key = static_cast<QKeyEvent *>(event)->key();
      if (key == Qt::Key_Tab || key == Qt::Key_Escape) {

        focusOutEvent(static_cast<QFocusEvent *>(event));
        return true;
      } else if (key == Qt::Key_Enter || key == Qt::Key_Return) {
        if (visibleButtons_.size() == 0)
          return true;
        if (selectionIndex_ >= visibleButtons_.size())
          selectionIndex_ = visibleButtons_.size() - 1;
        auto button = visibleButtons_.at(selectionIndex_);
        createNode(button->nodeName);
        doHide();
        return true;
      } else if (key == Qt::Key_Up) {
        moveSelection(SelectionDirection::DOWN);
      } else if (key == Qt::Key_Down) {
        moveSelection(SelectionDirection::UP);
      }
      // std::cout << "key pressed: " <<
      // static_cast<QKeyEvent*>(event)->text().toStdString() << "\n";
    }
    // else if(event->type() == QEvent::KeyRelease)
    // {
    //     std::cout << "key release: " <<
    //     static_cast<QKeyEvent*>(event)->text().toStdString() << "\n";
    // }
    QApplication::sendEvent(searchBar_, clone);
    return true;
  }
  return QWidget::event(event);
}

void enzo::ui::TabMenu::moveSelection(SelectionDirection direction) {
  if (direction == SelectionDirection::UP) {
    if (selectionIndex_ + 1 >= visibleButtons_.size()) {
      return;
    }
    selectionIndex_++;
  } else if (direction == SelectionDirection::DOWN) {
    if (selectionIndex_ <= 0) {
      return;
    }
    selectionIndex_--;
    std::cout << "selection index: " << selectionIndex_ << "\n";
  }

  for (size_t i = 0; i < visibleButtons_.size(); ++i) {
    TabMenuButton *button = visibleButtons_.at(i);
    if (i == selectionIndex_) {
      button->setSelected(true);
    } else {
      button->setSelected(false);
    }
  }
}

enzo::ui::TabMenuButton::TabMenuButton(const QString &text, QWidget *parent)
    : QPushButton(parent) {
  setObjectName("TabMenuButton");
  setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: 4px solid transparent;
            padding: 0px;
            margin: 0px;
            border-radius: 8px;
        }

        QPushButton:pressed { background-color: #b0b0b0; }
    )");

  displayText_ = text;
  setAttribute(Qt::WA_Hover, true);
  setMouseTracking(true);

  textLabel_ = new QLabel(text, this);
  textLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  textLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  textLabel_->setStyleSheet(
      "QLabel { color: white; background: transparent; }");
  this->setStyleSheet("#TabMenuButton {background: transparent; border: none}");

  setSelected(false);

  icon_ = new QSvgWidget(":/node-icons/grid.svg");
  icon_->renderer()->setAspectRatioMode(Qt::KeepAspectRatio);
  icon_->setStyleSheet("background-color: transparent;");
  icon_->setFixedSize(16, 16);

  mainLayout_ = new QHBoxLayout();
  constexpr int leftRightMargin = 10;
  mainLayout_->setContentsMargins(leftRightMargin, 0, leftRightMargin, 0);

  mainLayout_->addWidget(icon_);
  mainLayout_->addWidget(textLabel_);

  setLayout(mainLayout_);
}

void enzo::ui::TabMenuButton::enterEvent(QEnterEvent *event) {
  QPushButton::enterEvent(event);
  setSelected(true);
}

void enzo::ui::TabMenuButton::leaveEvent(QEvent *event) {
  QPushButton::leaveEvent(event);
  setSelected(false);
}

void enzo::ui::TabMenuButton::setSelected(bool selected) {
  if (selected) {
    textLabel_->setStyleSheet(
        "QLabel {color: black; background: transparent; }");
    this->setStyleSheet(
        R"(
        QPushButton {
            background-color: white;
            border: 4px solid transparent;
            padding: 0px;
            margin: 0px;
            border-radius: 8px;
        }
    )");
  } else {
    textLabel_->setStyleSheet(
        "QLabel { color: white;  background: transparent; }");
    this->setStyleSheet(
        R"(
        QPushButton {
            background-color: transparent;
            border: 4px solid transparent;
            padding: 0px;
            margin: 0px;
            border-radius: 8px;
        }
    )");
  }

  update();
}

// enzo::ui::TabMenuSearch::TabMenuSearch(QWidget *parent)
// : QLineEdit(parent)
// {

// }
