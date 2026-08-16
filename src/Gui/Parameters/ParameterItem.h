#pragma once

#include "Engine/Serializer/ParameterSerializable.h"
#include <QList>
#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <boost/signals2/connection.hpp>
#include <memory>

namespace enzo::prm {
class NodeParameter;
class Template;
} // namespace enzo::prm

namespace enzo::nt {
class Node;
}

namespace enzo::ui {

/// @brief One parameter exposed to QML, bound two ways to its engine parameter.
class ParameterItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString kind READ kind CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString nodeName READ nodeName CONSTANT)
    Q_PROPERTY(QString label READ label CONSTANT)
    Q_PROPERTY(QString tooltip READ tooltip CONSTANT)
    Q_PROPERTY(int vectorSize READ vectorSize CONSTANT)
    Q_PROPERTY(qreal minimum READ minimum CONSTANT)
    Q_PROPERTY(qreal maximum READ maximum CONSTANT)
    Q_PROPERTY(bool minLocked READ minLocked CONSTANT)
    Q_PROPERTY(bool maxLocked READ maxLocked CONSTANT)
    Q_PROPERTY(QStringList options READ options CONSTANT)
    Q_PROPERTY(QStringList optionTokens READ optionTokens CONSTANT)
    Q_PROPERTY(QList<QObject*> children READ children CONSTANT)
    Q_PROPERTY(bool horizontal READ horizontal CONSTANT)
    Q_PROPERTY(bool labelHidden READ labelHidden CONSTANT)
    Q_PROPERTY(bool enabled READ enabled NOTIFY metaChanged)
    Q_PROPERTY(bool hidden READ hidden NOTIFY metaChanged)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(bool hasExpression READ hasExpression NOTIFY valueChanged)
    Q_PROPERTY(QString expression READ expression NOTIFY valueChanged)
    Q_PROPERTY(QString expressionError READ expressionError NOTIFY valueChanged)

  public:
    /// @brief Builds an item from a template and the parameter it drives.
    /// @param parameter The value source, empty for a container or spacer.
    ParameterItem(
        const prm::Template& prmTemplate,
        std::weak_ptr<prm::NodeParameter> parameter,
        nt::Node& node,
        QObject* parent = nullptr
    );

    /// @brief The type as a lowercase token the QML delegate dispatches on.
    QString kind() const { return kind_; }
    QString name() const { return name_; }
    /// @brief The owning node's name, for a prm() reference into this parameter.
    QString nodeName() const { return nodeName_; }
    QString label() const { return label_; }
    QString tooltip() const { return tooltip_; }
    int vectorSize() const { return vectorSize_; }
    qreal minimum() const { return minimum_; }
    qreal maximum() const { return maximum_; }

    /// @brief Whether a drag is hard clamped at the range end rather than free
    /// to exceed a soft hint.
    bool minLocked() const { return minLocked_; }
    bool maxLocked() const { return maxLocked_; }
    QStringList options() const { return options_; }
    QStringList optionTokens() const { return optionTokens_; }
    QList<QObject*> children() const { return children_; }

    /// @brief Whether a group lays its children side by side rather than stacked.
    bool horizontal() const { return horizontal_; }
    bool labelHidden() const { return labelHidden_; }

    bool enabled() const { return enabled_; }
    bool hidden() const { return hidden_; }

    QVariant value() const { return valueAt(0); }
    void setValue(const QVariant& value) { setValueAt(0, value); }
    bool hasExpression() const { return hasExpressionAt(0); }
    QString expression() const { return expressionAt(0); }
    QString expressionError() const { return expressionErrorAt(0); }

    /// @brief Reads one component of a vector parameter such as an XYZ axis.
    /// @note A multiparm reads as a list of instance maps keyed by field name.
    Q_INVOKABLE QVariant valueAt(int index) const;
    /// @brief Writes one component of a vector parameter.
    /// @note A multiparm writes from a list of instance maps keyed by field name.
    Q_INVOKABLE void setValueAt(int index, const QVariant& value);

    /// @brief Whether a component's value comes from an expression rather than a literal.
    Q_INVOKABLE bool hasExpressionAt(int index) const;
    /// @brief The raw expression source on a component, empty when it holds a literal.
    Q_INVOKABLE QString expressionAt(int index) const;
    /// @brief The error from a component's expression, empty when it evaluated cleanly.
    Q_INVOKABLE QString expressionErrorAt(int index) const;
    /// @brief Drives a component from an expression instead of a literal value.
    Q_INVOKABLE void setExpressionAt(int index, const QString& expression);
    /// @brief Drops a component's expression, reverting to its underlying literal.
    Q_INVOKABLE void clearExpressionAt(int index);
    /// @brief Evaluates arbitrary text as a live preview, without storing it.
    /// @return A map with value (the result) and invalid (whether it errored).
    Q_INVOKABLE QVariantMap previewExpressionAt(int index, const QString& expression) const;

    /// @brief Snapshots the parameter ahead of a gesture such as a handle drag.
    Q_INVOKABLE void beginEdit();
    /// @brief Pushes one undo step covering everything since beginEdit.
    Q_INVOKABLE void commitEdit();

    /// @brief Adopts a child item, used to assemble a GROUP's contents.
    void addChild(ParameterItem* child) { children_.append(child); }

    /// @brief Sets the enabled and hidden state, emitting only on a change.
    void setMeta(bool enabled, bool hidden);

  Q_SIGNALS:
    void valueChanged();
    void metaChanged();

  private:
    QString kind_;
    QString name_;
    QString nodeName_;
    QString label_;
    QString tooltip_;
    int vectorSize_ = 1;
    qreal minimum_ = 0;
    qreal maximum_ = 0;
    bool minLocked_ = false;
    bool maxLocked_ = false;
    QStringList options_;
    QStringList optionTokens_;
    QList<QObject*> children_;
    bool horizontal_ = false;
    bool labelHidden_ = false;

    std::weak_ptr<prm::NodeParameter> parameter_;
    boost::signals2::scoped_connection valueSubscription_;
    ParameterSerializable snapshotBeforeEdit_;

    bool enabled_ = true;
    bool hidden_ = false;
};

} // namespace enzo::ui
