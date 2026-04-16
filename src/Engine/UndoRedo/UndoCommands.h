#pragma once

enum class UndoCommandType {
    MoveNode,
    CreateNode,
    DeleteNode,
    ChangeParameter,
    ChangeDisplayFlag,
    ChangeSelection,
    ChangeConnection,
    UndoGroup,
};
