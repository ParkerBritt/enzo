#pragma once

#include "Engine/Parameter/NodeParameter.h"
#include "Engine/Parameter/Style.h"
#include "Gui/Parameters/Parameter.h"
#include <QAbstractButton>
#include <QPixmap>
#include <QVariantAnimation>
#include <boost/signals2/connection.hpp>
#include <memory>

namespace enzo::ui
{

class SlashIconButton
: public QAbstractButton
{
    Q_OBJECT
public:
    SlashIconButton(QPixmap icon, int iconPx, QWidget* parent = nullptr);

    void setSlashProgress(float progress);
    float slashProgress() const { return slashProgress_; }

    void setTiltAngle(float degrees);

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize sizeHint() const override;

private:
    QPixmap icon_;
    int iconPx_;
    float slashProgress_ = 0.0f;
    float tiltAngle_    = 0.0f;
};

class BoolIconSlashParm
: public Parameter
{
    Q_OBJECT
public:
    BoolIconSlashParm(std::weak_ptr<prm::NodeParameter> parameter,
                      std::shared_ptr<prm::style::BoolIconSlash> style,
                      QWidget* parent = nullptr);

private:
    void onToggle(bool checked);
    void syncFromParameter();
    void animateTo(bool toggledOn);

    std::weak_ptr<prm::NodeParameter> parameter_;
    std::shared_ptr<prm::style::BoolIconSlash> style_;
    SlashIconButton* button_ = nullptr;
    QVariantAnimation* animation_ = nullptr;
    QVariantAnimation* tiltAnimation_ = nullptr;
    boost::signals2::scoped_connection valueChangedConnection_;
};

}
