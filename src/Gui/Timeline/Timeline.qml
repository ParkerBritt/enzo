import QtQuick
import Enzo
import "../Components"

// Bar along the bottom of the window holding the transport, the current frame,
// the frame scale, and the playback range.
Item {
    id: root

    property bool playing: false

    implicitHeight: 38

    // Moves the playhead and stops playback.
    function scrubToFrame(value) {
        root.playing = false;
        timeline.frame = value;
    }

    // Steps forward, wrapping round to the start at the end of the range.
    function advance(steps) {
        const span = timeline.endFrame - timeline.startFrame + 1;
        const offset = timeline.frame - timeline.startFrame + steps;
        timeline.frame = timeline.startFrame + offset % span;
    }

    // Advances the frame on each screen refresh while playing.
    FrameAnimation {
        id: playback

        // Part of a frame left over from the last update, carried into the next.
        property real carry: 0

        running: root.playing
        onRunningChanged: playback.carry = 0
        onTriggered: {
            playback.carry += playback.smoothFrameTime * timeline.fps;
            const steps = Math.floor(playback.carry);
            playback.carry -= steps;
            root.advance(steps);
        }
    }

    Row {
        id: leftControls

        anchors.left: parent.left
        anchors.leftMargin: 11
        anchors.verticalCenter: parent.verticalCenter
        spacing: 9

        Row {
            id: transport

            spacing: 1

            TransportButton {
                name: "skip-back"
                tooltip: "To start"
                iconSize: 13
                onClicked: root.scrubToFrame(timeline.startFrame)
            }
            TransportButton {
                name: "chevron-left"
                tooltip: "Step back"
                onClicked: root.scrubToFrame(timeline.frame - 1)
            }
            TransportButton {
                name: root.playing ? "pause" : "play"
                tooltip: root.playing ? "Pause" : "Play"
                width: 30
                surfaceColor: Theme.timeline.playColor
                hoverColor: Theme.timeline.playHoverColor
                iconColor: Theme.var.textStrong
                onClicked: root.playing = !root.playing
            }
            TransportButton {
                name: "chevron-right"
                tooltip: "Step forward"
                onClicked: root.scrubToFrame(timeline.frame + 1)
            }
            TransportButton {
                name: "skip-forward"
                tooltip: "To end"
                iconSize: 13
                onClicked: root.scrubToFrame(timeline.endFrame)
            }
        }

        // The current frame.
        Field {
            id: frameField

            width: frameLabel.width + frameValue.width + frameSteppers.width

            FieldLabel {
                id: frameLabel

                text: "Frame"
            }

            Text {
                id: frameValue

                anchors.left: frameLabel.right
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(42, contentWidth + 20)
                horizontalAlignment: Text.AlignHCenter
                text: timeline.frame
                color: Theme.var.textStrong
                font.family: Theme.var.fontMono
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }

            Item {
                id: frameSteppers

                anchors.left: frameValue.right
                width: 17
                height: parent.height

                IconButton {
                    anchors.top: parent.top
                    name: "chevron-up"
                    width: parent.width
                    height: 12
                    radius: 0
                    iconSize: 9
                    onClicked: root.scrubToFrame(timeline.frame + 1)
                }

                IconButton {
                    anchors.bottom: parent.bottom
                    name: "chevron-down"
                    width: parent.width
                    height: 12
                    radius: 0
                    iconSize: 9
                    onClicked: root.scrubToFrame(timeline.frame - 1)
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width
                    height: 1
                    color: Theme.var.border
                }

                Rectangle {
                    width: 1
                    height: parent.height
                    color: Theme.var.border
                }
            }
        }
    }

    TimelineRuler {
        anchors.left: leftControls.right
        anchors.right: rightControls.left
        anchors.leftMargin: 9
        anchors.rightMargin: 9
        anchors.verticalCenter: parent.verticalCenter
        startFrame: timeline.startFrame
        endFrame: timeline.endFrame
        frame: timeline.frame
        onFrameRequested: value => root.scrubToFrame(value)
    }

    Row {
        id: rightControls

        anchors.right: parent.right
        anchors.rightMargin: 11
        anchors.verticalCenter: parent.verticalCenter
        spacing: 9

        // The frames playback runs between.
        Field {
            id: rangeField

            width: rangeLabel.width + rangeIn.width + rangeDash.width + rangeOut.width

            FieldLabel {
                id: rangeLabel

                text: "Range"
            }

            RangeValue {
                id: rangeIn

                anchors.left: rangeLabel.right
                label: "in"
                value: timeline.startFrame
                onCommitted: value => timeline.startFrame = value
            }

            Text {
                id: rangeDash

                anchors.left: rangeIn.right
                anchors.verticalCenter: parent.verticalCenter
                text: "–"
                color: Theme.var.textFaint
                font.family: Theme.var.fontSans
                font.pixelSize: 11
            }

            RangeValue {
                id: rangeOut

                anchors.left: rangeDash.right
                label: "out"
                value: timeline.endFrame
                onCommitted: value => timeline.endFrame = value
            }
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: 18
            color: Theme.var.borderSoft
        }

        Field {
            id: rateField

            width: rateValue.contentWidth + rateUnit.contentWidth + 22

            NumberField {
                id: rateValue

                x: 9
                anchors.verticalCenter: parent.verticalCenter
                value: timeline.fps
                onCommitted: value => timeline.fps = value
            }

            Text {
                id: rateUnit

                anchors.left: rateValue.right
                anchors.leftMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                text: "fps"
                color: Theme.var.textLabel
                font.family: Theme.var.fontMono
                font.pixelSize: 11
            }
        }
    }

    // The chip a value on the bar sits in.
    component Field: Rectangle {
        height: 24
        radius: 7
        color: Theme.var.fieldSurface
        border.color: Theme.var.border
    }

    // One of the buttons that drive playback.
    component TransportButton: IconButton {
        width: 24
        height: 24
        radius: 6
        iconSize: 14
    }

    // The name at the head of a field.
    component FieldLabel: Item {
        property alias text: caption.text

        anchors.verticalCenter: parent.verticalCenter
        width: caption.contentWidth + 16
        height: parent.height

        Text {
            id: caption

            anchors.centerIn: parent
            color: Theme.var.textMuted
            font.family: Theme.var.fontSans
            font.pixelSize: 8
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 0.64
            font.weight: Font.DemiBold
        }

        Rectangle {
            anchors.right: parent.right
            width: 1
            height: parent.height
            color: Theme.var.border
        }
    }

    // A number the user can click into and type over.
    component NumberField: TextInput {
        id: field

        property int value: 0

        signal committed(int value)

        // Binds the text back to the value.
        function reset() {
            field.text = Qt.binding(() => field.value);
        }

        text: field.value
        color: Theme.var.text
        font.family: Theme.var.fontMono
        font.pixelSize: 11
        selectByMouse: true
        validator: IntValidator {
            bottom: 1
        }
        onEditingFinished: {
            field.committed(parseInt(field.text));
            field.reset();
        }
        Keys.onEscapePressed: {
            field.focus = false;
            field.reset();
        }
    }

    // One end of the playback range.
    component RangeValue: Item {
        id: rangeValue

        property alias label: caption.text
        property alias value: number.value

        signal committed(int value)

        anchors.verticalCenter: parent.verticalCenter
        width: caption.contentWidth + number.contentWidth + 23
        height: parent.height

        Text {
            id: caption

            x: 9
            anchors.baseline: number.baseline
            color: Theme.var.textMuted
            font.family: Theme.var.fontSans
            font.pixelSize: 8
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 0.4
        }

        NumberField {
            id: number

            anchors.left: caption.right
            anchors.leftMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            onCommitted: value => rangeValue.committed(value)
        }
    }
}
