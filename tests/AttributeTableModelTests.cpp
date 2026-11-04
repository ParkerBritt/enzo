#include "Engine/Primitives/Camera.h"
#include "Gui/Spreadsheet/AttributeTableModel.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>

using namespace enzo;

TEST_CASE("Showing vertices of a camera gives an empty table")
{
    ui::AttributeTableModel model;
    model.setOwner(attr::AttributeOwner::VERTEX);

    model.setPrimitive(std::make_shared<geo::Camera>());

    REQUIRE(model.rowCount() == 0);
    REQUIRE(model.columnCount() == 0);
}

TEST_CASE("Switching a camera's table to faces gives an empty table")
{
    ui::AttributeTableModel model;
    model.setPrimitive(std::make_shared<geo::Camera>());

    model.setOwner(attr::AttributeOwner::FACE);

    REQUIRE(model.rowCount() == 0);
    REQUIRE(model.columnCount() == 0);
}
