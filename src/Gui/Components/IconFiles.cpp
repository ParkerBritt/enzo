#include "Gui/Components/IconFiles.h"
#include "Engine/Core/InstallPaths.h"
#include "Engine/Network/NodeTypeTable.h"

namespace enzo::ui {

namespace {

const QString nodeIconPrefix = "node/";
const QString defaultNodeIconName = "nodeDefault";

} // namespace

std::filesystem::path getIconFile(const QString& name)
{
    // Node icons
    if (name.startsWith(nodeIconPrefix))
    {
        const std::string nodeTypeName = name.mid(nodeIconPrefix.size()).toStdString();
        const nt::NodeType* nodeType = nt::NodeTypeTable::getNodeType(nodeTypeName);
        if (!nodeType) return {};
        return getNodeIconFile(*nodeType);
    }

    // Lucide icons, then Enzo's own
    const std::filesystem::path iconsDir = getStaticDir() / "icons";
    const std::string fileName = name.toStdString() + ".svg";

    const std::filesystem::path lucidePath = iconsDir / "lucide" / fileName;
    if (std::filesystem::exists(lucidePath)) return lucidePath;

    const std::filesystem::path enzoPath = iconsDir / "enzo" / fileName;
    if (std::filesystem::exists(enzoPath)) return enzoPath;

    return {};
}

std::filesystem::path getNodeIconFile(const nt::NodeType& nodeType)
{
    const std::filesystem::path declaredIconFile = nodeType.getIconFile();
    if (declaredIconFile.empty()) return getIconFile(defaultNodeIconName);
    return declaredIconFile;
}

QUrl getNodeIconUrl(const nt::NodeType& nodeType)
{
    return QUrl::fromLocalFile(QString::fromStdString(getNodeIconFile(nodeType).string()));
}

} // namespace enzo::ui
