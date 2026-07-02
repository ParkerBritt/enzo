import QtQuick
import QtQuick.Dialogs
import "../Components"

// The menu bar used at the top of the app.
MenuBar {
    FileDialog {
        id: openDialog
        title: "Open Scene"
        nameFilters: ["Enzo files (*.enzo)", "All files (*)"]
        onAccepted: scene.open(selectedFile)
    }
    FileDialog {
        id: saveDialog
        title: "Save Scene As"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "enzo"
        nameFilters: ["Enzo files (*.enzo)", "All files (*)"]
        onAccepted: scene.saveAs(selectedFile)
    }
    PlaceholderDialog {
        id: placeholder
    }

    // Reading scene.recentFiles inside the entries binding keeps the menu live.
    function recentEntries() {
        const files = scene.recentFiles;
        if (files.length === 0)
            return [
                {
                    text: "No Recent Files",
                    enabled: false
                }
            ];
        return files.map(filePath => ({
                    text: filePath.split("/").pop(),
                    action: () => scene.openPath(filePath)
                }));
    }

    menus: [
        {
            title: "File",
            entries: () => [
                    {
                        text: "New",
                        action: () => scene.newScene()
                    },
                    {
                        text: "Open…",
                        action: () => openDialog.open()
                    },
                    {
                        text: "Open Recent",
                        children: recentEntries()
                    },
                    {
                        text: "Save",
                        action: () => scene.hasCurrentFile ? scene.save() : saveDialog.open()
                    },
                    {
                        text: "Save As…",
                        action: () => saveDialog.open()
                    },
                    {
                        text: "Quit",
                        action: () => Qt.quit()
                    },
                ]
        },
        {
            title: "Edit",
            entries: () => [
                    {
                        text: "Undo",
                        action: () => network.undo()
                    },
                    {
                        text: "Redo",
                        action: () => network.redo()
                    },
                ]
        },
        {
            title: "Window",
            entries: () => []
        },
        {
            title: "Help",
            entries: () => [
                    {
                        text: "Start Tutorial",
                        action: () => placeholder.show("Tutorial")
                    },
                    {
                        text: "Examples…",
                        action: () => placeholder.show("Examples")
                    },
                    {
                        text: "About Enzo",
                        enabled: false
                    },
                ]
        },
    ]
}
