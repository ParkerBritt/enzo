#pragma once

#include <QList>
#include <QObject>
#include <QStringList>
#include <QVariant>
#include <boost/signals2/connection.hpp>
#include <memory>

namespace enzo::prm {
class NodeParameter;
class Template;
} // namespace enzo::prm

namespace enzo::ui {

/// @brief One parameter exposed to QML, bound two ways to its engine parameter.
class ParameterItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString kind READ kind CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString label READ label CONSTANT)
    Q_PROPERTY(QString tooltip READ tooltip CONSTANT)
    Q_PROPERTY(int vectorSize READ vectorSize CONSTANT)
    Q_PROPERTY(qreal minimum READ minimum CONSTANT)
    Q_PROPERTY(qreal maximum READ maximum CONSTANT)
    Q_PROPERTY(QStringList options READ options CONSTANT)
    Q_PROPERTY(QStringList optionTokens READ optionTokens CONSTANT)
    Q_PROPERTY(QList<QObject*> children READ children CONSTANT)
    Q_PROPERTY(bool enabled READ enabled NOTIFY metaChanged)
    Q_PROPERTY(bool hidden READ hidden NOTIFY metaChanged)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

  public:
    /// @brief Builds an item from a template and the parameter it drives.
    /// @param parameter The value source, empty for a container or spacer.
    ParameterItem(
        const prm::Template& prmTemplate,
        std::weak_ptr<prm::NodeParameter> parameter,
        QObject* parent = nullptr
    );

    /// @brief The type as a lowercase token the QML delegate dispatches on.
    QString kind() const { return kind_; }
    QString name() const { return name_; }
    QString label() const { return label_; }
    QString tooltip() const { return tooltip_; }
    int vectorSize() const { return vectorSize_; }
    qreal minimum() const { return minimum_; }
    qreal maximum() const { return maximum_; }
    QStringList options() const { return options_; }
    QStringList optionTokens() const { return optionTokens_; }
    QList<QObject*> children() const { return children_; }

    bool enabled() const { return enabled_; }
    bool hidden() const { return hidden_; }

    QVariant value() const { return valueAt(0); }
    void setValue(const QVariant& value) { setValueAt(0, value); }

    /// @brief Reads one component of a vector parameter such as an XYZ axis.
    Q_INVOKABLE QVariant valueAt(int index) const;
    /// @brief Writes one component of a vector parameter.
    Q_INVOKABLE void setValueAt(int index, const QVariant& value);

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
    QString label_;
    QString tooltip_;
    int vectorSize_ = 1;
    qreal minimum_ = 0;
    qreal maximum_ = 0;
    QStringList options_;
    QStringList optionTokens_;
    QList<QObject*> children_;

    std::weak_ptr<prm::NodeParameter> parameter_;
    boost::signals2::scoped_connection valueSubscription_;

    bool enabled_ = true;
    bool hidden_ = false;
};

} // namespace enzo::ui
