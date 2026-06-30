#include "Gui/Style/ThemeLoader.h"

#include <QColor>
#include <QFile>
#include <QSet>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace enzo::ui {

namespace {

constexpr QChar kColorMarker = u'#';
constexpr QChar kReferenceMarker = u'$';
constexpr QChar kOpacityMarker = u'@';

using Variables = QHash<QString, YAML::Node>;

QString toQString(const YAML::Node& scalar) { return QString::fromStdString(scalar.Scalar()); }

// Parses "#rrggbb" or "#rrggbbaa".
QColor parseColor(const QString& hex)
{
    if (hex.size() != 7 && hex.size() != 9)
        throw std::runtime_error("invalid colour " + hex.toStdString());

    int channel[4] = {0, 0, 0, 255};
    const int channelCount = hex.size() == 9 ? 4 : 3;
    for (int i = 0; i < channelCount; ++i)
    {
        bool ok = false;
        channel[i] = hex.mid(1 + 2 * i, 2).toInt(&ok, 16);
        if (!ok) throw std::runtime_error("invalid colour " + hex.toStdString());
    }
    return QColor(channel[0], channel[1], channel[2], channel[3]);
}

// Resolves one value into a colour, number, font name or list. A $name reads a
// variable, optionally faded with $name @ opacity.
QVariant resolveValue(const YAML::Node& value, const Variables& variables, QSet<QString>& resolving)
{
    if (value.IsSequence())
    {
        QVariantList list;
        for (const auto& item : value)
            list.append(resolveValue(item, variables, resolving));
        return list;
    }

    const QString text = toQString(value).trimmed();

    if (text.startsWith(kColorMarker)) return parseColor(text);

    if (!text.startsWith(kReferenceMarker))
    {
        bool isNumber = false;
        const double number = text.toDouble(&isNumber);
        return isNumber ? QVariant(number) : QVariant(text);
    }

    const QString body = text.mid(1).trimmed();
    const int opacityMark = body.indexOf(kOpacityMarker);
    const QString name = (opacityMark < 0 ? body : body.left(opacityMark)).trimmed();

    if (resolving.contains(name))
        throw std::runtime_error("variable cycle at " + name.toStdString());
    const auto definition = variables.constFind(name);
    if (definition == variables.constEnd())
        throw std::runtime_error("undefined variable " + name.toStdString());

    resolving.insert(name);
    const QVariant variable = resolveValue(*definition, variables, resolving);
    resolving.remove(name);

    if (opacityMark < 0) return variable;

    if (variable.typeId() != QMetaType::QColor)
        throw std::runtime_error("opacity needs a colour, not " + name.toStdString());
    bool ok = false;
    const double opacity = body.mid(opacityMark + 1).trimmed().toDouble(&ok);
    if (!ok) throw std::runtime_error("opacity is not a number");

    QColor faded = variable.value<QColor>();
    faded.setAlphaF(opacity);
    return faded;
}

// Reads the variables block into the lookup used for $name references.
void collectVariables(const YAML::Node& theme, Variables& variables)
{
    if (const YAML::Node block = theme["variables"])
        for (const auto& entry : block)
            variables.insert(toQString(entry.first), entry.second);
}

// Returns every value a theme defines, keyed group.name, with variables under var.
std::vector<std::pair<QString, YAML::Node>> getThemeValues(const YAML::Node& theme)
{
    std::vector<std::pair<QString, YAML::Node>> values;

    if (const YAML::Node variables = theme["variables"])
        for (const auto& entry : variables)
            values.emplace_back(QStringLiteral("var.") + toQString(entry.first), entry.second);

    if (const YAML::Node components = theme["components"])
        for (const auto& component : components)
            for (const auto& slot : component.second)
                values.emplace_back(
                    toQString(component.first) + QChar(u'.') + toQString(slot.first),
                    slot.second
                );

    return values;
}

// Resolves a whole theme into its tokens. Any fault throws, since the default
// theme must define every token the user theme falls back to.
QHash<QString, QVariant> resolveTheme(const YAML::Node& theme, const Variables& variables)
{
    if (!theme["components"]) throw std::runtime_error("theme has no components block");

    QHash<QString, QVariant> tokens;
    for (const auto& [token, value] : getThemeValues(theme))
    {
        QSet<QString> resolving;
        tokens.insert(token, resolveValue(value, variables, resolving));
    }
    return tokens;
}

// Overlays a theme onto the existing table. A token that is unknown, the wrong
// type, or unresolvable keeps its default value, so a user typo never blanks a slot.
void overlayTheme(
    const YAML::Node& theme,
    const Variables& variables,
    QHash<QString, QVariant>& tokens
)
{
    for (const auto& [token, value] : getThemeValues(theme))
    {
        if (!tokens.contains(token)) continue;
        try
        {
            QSet<QString> resolving;
            const QVariant resolved = resolveValue(value, variables, resolving);
            if (resolved.typeId() == tokens.value(token).typeId())
                tokens.insert(token, resolved);
            else
                qWarning(
                    "theme token %s has the wrong type, keeping the default",
                    qPrintable(token)
                );
        }
        catch (const std::exception& error)
        {
            qWarning("theme token %s ignored (%s)", qPrintable(token), error.what());
        }
    }
}

} // namespace

QHash<QString, QVariant>
ThemeLoader::loadFromString(const QString& defaultYaml, const QString& userYaml)
{
    const YAML::Node defaultTheme = YAML::Load(defaultYaml.toStdString());

    Variables variables;
    collectVariables(defaultTheme, variables);
    QHash<QString, QVariant> tokens = resolveTheme(defaultTheme, variables);

    if (userYaml.isEmpty()) return tokens;

    YAML::Node userTheme;
    try
    {
        userTheme = YAML::Load(userYaml.toStdString());
    }
    catch (const std::exception& error)
    {
        qWarning("user theme ignored, it did not parse (%s)", error.what());
        return tokens;
    }

    // The default is resolved again so its slots pick up any variable the user changed.
    collectVariables(userTheme, variables);
    overlayTheme(defaultTheme, variables, tokens);
    overlayTheme(userTheme, variables, tokens);
    return tokens;
}

QHash<QString, QVariant>
ThemeLoader::loadFromFile(const QString& defaultPath, const QString& userPath)
{
    QFile defaultFile(defaultPath);
    if (!defaultFile.open(QIODevice::ReadOnly | QIODevice::Text))
        throw std::runtime_error("cannot open default theme " + defaultPath.toStdString());

    QString userYaml;
    QFile userFile(userPath);
    if (!userPath.isEmpty() && userFile.open(QIODevice::ReadOnly | QIODevice::Text))
        userYaml = QString::fromUtf8(userFile.readAll());

    return loadFromString(QString::fromUtf8(defaultFile.readAll()), userYaml);
}

} // namespace enzo::ui
