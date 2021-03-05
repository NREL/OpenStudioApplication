/***********************************************************************************************************************
*  OpenStudio(R), Copyright (c) 2020-2020, OpenStudio Coalition and other contributors. All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
*  following conditions are met:
*
*  (1) Redistributions of source code must retain the above copyright notice, this list of conditions and the following
*  disclaimer.
*
*  (2) Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
*  disclaimer in the documentation and/or other materials provided with the distribution.
*
*  (3) Neither the name of the copyright holder nor the names of any contributors may be used to endorse or promote products
*  derived from this software without specific prior written permission from the respective party.
*
*  (4) Other than as required in clauses (1) and (2), distributions in any form of modifications or other derivative works
*  may not use the "OpenStudio" trademark, "OS", "os", or any other confusingly similar designation without specific prior
*  written permission from Alliance for Sustainable Energy, LLC.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) AND ANY CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
*  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER(S), ANY CONTRIBUTORS, THE UNITED STATES GOVERNMENT, OR THE UNITED
*  STATES DEPARTMENT OF ENERGY, NOR ANY OF THEIR EMPLOYEES, BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
*  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
*  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
*  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
*  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***********************************************************************************************************************/

#include <gtest/gtest.h>

#include "OpenStudioLibFixture.hpp"

#include "../SpacesSpacesGridView.hpp"
#include "../SpacesSurfacesGridView.hpp"
#include "../OSDropZone.hpp"
#include "../shared_gui_components/OSGridController.hpp"
#include "../shared_gui_components/OSLineEdit.hpp"

#include <openstudio/model/Model.hpp>
#include <openstudio/model/Space.hpp>
#include <openstudio/model/Space_Impl.hpp>
#include <openstudio/model/SpaceType.hpp>
#include <openstudio/model/SpaceType_Impl.hpp>
#include <openstudio/model/Surface.hpp>
#include <openstudio/model/Surface_Impl.hpp>

#include <QComboBox>

#include <memory>

using namespace openstudio;

class TestGridController : public OSGridController
{

 public:
  TestGridController() 
  : OSGridController()
  {}

  TestGridController(bool isIP, const QString& headerText, IddObjectType iddObjectType, const model::Model& model,
                     const std::vector<model::ModelObject>& modelObjects)
  : OSGridController(isIP, headerText, iddObjectType, model, modelObjects)
  {}

  virtual ~TestGridController() {}

  virtual void refreshModelObjects() override {}

  virtual void onItemDropped(const OSItemId& itemId) override {}

 protected:

  virtual void addColumns(const QString& t_category, std::vector<QString>& fields) override {}

  virtual QString getColor(const model::ModelObject& modelObject) override {return "";}

};

TEST_F(OpenStudioLibFixture, OptionalAssumptions) {
  model::Model model;
  model::Space space1(model);
  model::Space space2(model);

  model::Space space1_copy = space1;

  EXPECT_EQ(2u, model.getConcreteModelObjects<model::Space>().size());

  boost::optional<model::Space> optSpace1;
  boost::optional<model::Space> optSpace2;

  EXPECT_TRUE(optSpace1 == optSpace1);
  EXPECT_TRUE(optSpace1 == optSpace2);
  EXPECT_TRUE(optSpace1 != space1);

  optSpace1 = space1;
  optSpace2 = space1_copy;

  EXPECT_TRUE(optSpace1 == optSpace1);
  EXPECT_TRUE(optSpace1 == optSpace2);
  EXPECT_TRUE(optSpace1 == space1);
  EXPECT_TRUE(optSpace1 == space1_copy);
  
  optSpace1 = space1;
  optSpace2 = space2;

  EXPECT_TRUE(optSpace1 == optSpace1);
  EXPECT_TRUE(optSpace1 != optSpace2);
}

TEST_F(OpenStudioLibFixture, ObjectSelector) {

  TestGridController grid;
  ObjectSelector selector(&grid);  
}

TEST_F(OpenStudioLibFixture, SpacesSpacesGridView) {

  model::Model model = model::exampleModel();
  auto spaces = model.getConcreteModelObjects<model::Space>();
  std::sort(spaces.begin(), spaces.end(), WorkspaceObjectNameLess());
  auto spaceTypes = model.getConcreteModelObjects<model::SpaceType>();

  ASSERT_EQ(4u, spaces.size());
  EXPECT_EQ(1u, spaceTypes.size());

  auto space1 = spaces[0];

  auto gridView = std::make_shared<SpacesSpacesGridView>(false, model);
  auto gridController = getGridController(gridView.get());
  auto objectSelector = gridController->getObjectSelector();

  processEvents();

  auto modelObjects = gridController->modelObjects();
  ASSERT_EQ(4u, modelObjects.size());
  EXPECT_EQ(space1.handle(), modelObjects[0].handle());

  auto selectableObjectsSet = objectSelector->selectableObjects();
  EXPECT_EQ(spaces.size(), selectableObjectsSet.size());

  auto selectedObjects = gridController->selectedObjects();
  EXPECT_EQ(0u, selectedObjects.size());

  auto selectedObjectsSet = objectSelector->selectedObjects();
  EXPECT_EQ(0u, selectedObjectsSet.size());
  
  objectSelector->setObjectSelected(space1, true);

  selectedObjects = gridController->selectedObjects();
  ASSERT_EQ(1u, selectedObjects.size());
  EXPECT_EQ(space1.handle(), selectedObjects[0].handle());

  selectedObjectsSet = objectSelector->selectedObjects();
  ASSERT_EQ(1u, selectedObjectsSet.size());
  EXPECT_EQ(space1.handle(), selectedObjectsSet.begin()->handle());

  // Row = 1, Col = 1 is Space Name header, No SubRow
  unsigned row = 1, col = 0;

  auto rowWidgets = gridController->row(row);
  auto cellWidget = gridController->cell(row, cell);

  ASSERT_EQ(8u, rowWidgets.size());
  ASSERT_TRUE(cellWidget);
  ASSERT_TRUE(rowWidgets[cell]);
  EXPECT_EQ(cellWidget, rowWidgets[cell]);

  Holder* holder = qobject_cast<Holder*>(cellWidget->children()[1]);
  ASSERT_TRUE(holder);
  OSLineEdit2* lineEdit = qobject_cast<OSLineEdit2*>(holder->widget);
  ASSERT_TRUE(lineEdit);
  auto modelObject = getModelObject(lineEdit);
  ASSERT_TRUE(modelObject);
  EXPECT_EQ(modelObject->handle(), modelObjects[0].handle());

  // Row = 1, Col = 4 is Space Type, No SubRow
  row = 1, col = 4;

  rowWidgets = gridController->row(row);
  cellWidget = gridController->cell(row, cell);

  ASSERT_EQ(8u, rowWidgets.size());
  ASSERT_TRUE(cellWidget);
  ASSERT_TRUE(rowWidgets[cell]);
  EXPECT_EQ(cellWidget, rowWidgets[cell]);

  holder = qobject_cast<Holder*>(cellWidget->children()[1]);
  ASSERT_TRUE(holder);
  OSDropZone2* dropZone = qobject_cast<OSDropZone2*>(holder->widget);
  ASSERT_TRUE(dropZone);
  modelObject = getModelObject(dropZone);
  ASSERT_TRUE(modelObject);
  EXPECT_EQ(modelObject->handle(), modelObjects[0].handle());


}


TEST_F(OpenStudioLibFixture, SpacesSurfacesGridView) {

  model::Model model = model::exampleModel();
  auto spaces = model.getConcreteModelObjects<model::Space>();
  auto spaceTypes = model.getConcreteModelObjects<model::SpaceType>();
  auto surfaces = model.getConcreteModelObjects<model::Surface>();

  EXPECT_EQ(4u, spaces.size());
  EXPECT_EQ(1u, spaceTypes.size());
  EXPECT_EQ(24u, surfaces.size());

  auto gridView = std::make_shared<SpacesSurfacesGridView>(false, model);
  auto gridController = getGridController(gridView.get());
  auto objectSelector = gridController->getObjectSelector();
  
  processEvents();

  auto selectableObjects = objectSelector->selectableObjects();
  EXPECT_EQ(surfaces.size(), selectableObjects.size());

  auto selectedObjects = objectSelector->selectedObjects();
  EXPECT_EQ(0u, selectedObjects.size());

  for (const auto &surface: surfaces) {
    EXPECT_TRUE(objectSelector->containsObject(surface));
  }
  for (const auto &space: spaces) {
    EXPECT_TRUE(objectSelector->containsObject(space));
  }

  // Headers on row = 0
  unsigned row = 0;
  unsigned col = 0;
  unsigned i = 0;

  // Row = 0, Col = 0 is Space Name header, No SubRow
  row = 0;
  col = 0;

  auto obj = objectSelector->getObject(row, col, boost::none);
  EXPECT_FALSE(obj);

  auto widget = objectSelector->getWidget(row, col, boost::none);
  ASSERT_TRUE(widget);

  obj = objectSelector->getObject(row, col, 0);
  EXPECT_FALSE(obj);

  widget = objectSelector->getWidget(row, col, 0);
  EXPECT_FALSE(widget);

  // Row = 0, Col = 1 is All header, No SubRow
  row = 0;
  col = 1;

  obj = objectSelector->getObject(row, col, boost::none);
  EXPECT_FALSE(obj);

  widget = objectSelector->getWidget(row, col, boost::none);
  ASSERT_TRUE(widget);

  obj = objectSelector->getObject(row, col, 0);
  EXPECT_FALSE(obj);

  widget = objectSelector->getWidget(row, col, 0);
  EXPECT_FALSE(widget);

  // First row of objects on row = 1

  // Row = 1, Col = 0 is Space, No SubRow
  row = 1;
  col = 0;

  obj = objectSelector->getObject(row, col, boost::none);
  ASSERT_TRUE(obj);
  ASSERT_TRUE(obj->optionalCast<model::Space>());
  auto spaceObj = obj->cast<model::Space>();

  widget = objectSelector->getWidget(row, col, boost::none);
  ASSERT_TRUE(widget);

  // Row = 1, Col = 1 is Surface Select Check Boxes, Has SubRow
  row = 1;
  col = 1;

  obj = objectSelector->getObject(row, col, boost::none);
  EXPECT_FALSE(obj);

  widget = objectSelector->getWidget(row, col, boost::none);
  EXPECT_FALSE(widget);

  i = 0;
  for (const auto& surface : spaceObj.surfaces()) {
    obj = objectSelector->getObject(row, col, i);
    ASSERT_TRUE(obj);
    ASSERT_TRUE(obj->optionalCast<model::Surface>());
    auto surfaceObj = obj->cast<model::Surface>();
    ASSERT_TRUE(surfaceObj.space());
    EXPECT_EQ(spaceObj, surfaceObj.space());

    widget = objectSelector->getWidget(row, col, i);
    ASSERT_TRUE(widget);

    i += 1;
  }

  // Row = 1, Col = 1 is Surface Name Line Edits, Has SubRow
  row = 1;
  col = 2;

 obj = objectSelector->getObject(row, col, boost::none);
  EXPECT_FALSE(obj);

  widget = objectSelector->getWidget(row, col, boost::none);
  EXPECT_FALSE(widget);

  i = 0;
  for (const auto& surface : spaceObj.surfaces()) {
    obj = objectSelector->getObject(row, col, i);
    ASSERT_TRUE(obj);
    ASSERT_TRUE(obj->optionalCast<model::Surface>());
    auto surfaceObj = obj->cast<model::Surface>();
    ASSERT_TRUE(surfaceObj.space());
    EXPECT_EQ(spaceObj, surfaceObj.space());

    widget = objectSelector->getWidget(row, col, i);
    ASSERT_TRUE(widget);

    i += 1;
  }

  // selection
  objectSelector->selectAllVisible();

  processEvents();

  selectableObjects = objectSelector->selectableObjects();
  EXPECT_EQ(surfaces.size(), selectableObjects.size());

  selectedObjects = objectSelector->selectedObjects();
  EXPECT_EQ(surfaces.size(), selectedObjects.size());

  processEvents();

  EXPECT_EQ("All", gridView->m_surfaceTypeFilter->currentText());
  gridView->m_surfaceTypeFilter->setCurrentText("Wall");
  EXPECT_EQ("Wall", gridView->m_surfaceTypeFilter->currentText());

  processEvents();

  selectableObjects = objectSelector->selectableObjects();
  EXPECT_EQ(surfaces.size(), selectableObjects.size());

  selectedObjects = objectSelector->selectedObjects();
  EXPECT_EQ(surfaces.size(), selectedObjects.size());

  i = 0;
  for (const auto& surface : spaceObj.surfaces()) {
    obj = objectSelector->getObject(row, col, i);
    ASSERT_TRUE(obj);
    ASSERT_TRUE(obj->optionalCast<model::Surface>());
    auto surfaceObj = obj->cast<model::Surface>();
    ASSERT_TRUE(surfaceObj.space());
    EXPECT_EQ(spaceObj, surfaceObj.space());

    bool isVisible = surfaceObj.surfaceType() == "Wall";

    EXPECT_EQ(true, objectSelector->containsObject(surfaceObj));
    EXPECT_EQ(isVisible, objectSelector->getObjectVisible(surfaceObj));

    widget = objectSelector->getWidget(row, col, i);
    ASSERT_TRUE(widget);
    //EXPECT_EQ(isVisible, widget->isVisible());

    i += 1;
  }
}