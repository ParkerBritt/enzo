#pragma once

enum class UndoCommandType
{
    MoveNode,
    CreateNode,
    DeleteNode,
    ChangeParameter,
    ChangeDisplayFlag,
    ChangePrimaryNode,
    ChangeSelection,
    ChangeConnection,
    UndoGroup,
};
