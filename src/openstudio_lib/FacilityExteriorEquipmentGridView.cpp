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

#include "FacilityExteriorEquipmentGridView.hpp"

#include "OSItemSelectorButtons.hpp"

#include "../shared_gui_components/OSCheckBox.hpp"
#include "../shared_gui_components/OSGridView.hpp"
#include "../shared_gui_components/OSObjectSelector.hpp"

#include <openstudio/model/ExteriorLights.hpp>
#include <openstudio/model/ExteriorLights_Impl.hpp>
#include <openstudio/model/ExteriorLightsDefinition.hpp>
#include <openstudio/model/ExteriorLightsDefinition_Impl.hpp>
#include <openstudio/model/ExteriorFuelEquipment.hpp>
#include <openstudio/model/ExteriorFuelEquipment_Impl.hpp>
#include <openstudio/model/ExteriorFuelEquipmentDefinition.hpp>
#include <openstudio/model/ExteriorFuelEquipmentDefinition_Impl.hpp>
#include <openstudio/model/ExteriorWaterEquipment.hpp>
#include <openstudio/model/ExteriorWaterEquipment_Impl.hpp>
#include <openstudio/model/ExteriorWaterEquipmentDefinition.hpp>
#include <openstudio/model/ExteriorWaterEquipmentDefinition_Impl.hpp>
#include <openstudio/model/Model.hpp>
#include <openstudio/model/Model_Impl.hpp>
#include <openstudio/model/ModelObject.hpp>
#include <openstudio/model/ModelObject_Impl.hpp>
#include <openstudio/model/Schedule.hpp>
#include <openstudio/model/Schedule_Impl.hpp>

#include <openstudio/utilities/core/Assert.hpp>
#include <openstudio/utilities/core/Compare.hpp>
#include <openstudio/utilities/idd/IddEnums.hxx>
#include <openstudio/utilities/idd/OS_Exterior_Lights_FieldEnums.hxx>

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QScrollArea>

// These defines provide a common area for field display names
// used on column headers, and other grid widgets

#define NAME "Name"
#define SELECTED "All"

// EXTERIOR LIGHTS
#define EXTERIORLIGHTSDEFINITION "Exterior Lights Definition"
#define EXTERIORLIGHTSSCHEDULE "Schedule"
#define EXTERIORLIGHTSCONTROLOPTION "Control Option"
#define EXTERIORLIGHTSMULTIPLIER "Multiplier"
#define EXERIORLIGHTSENDUSESUBCATEGORY "End Use Subcategory"

// EXTERIOR FUEL EQUIPMENT
#define EXTERIORFUELEQUIPMENTDEFINITION "Exterior Fuel Equipment Definition"
#define EXTERIORFUELEQUIPMENTSCHEDULE "Schedule"
#define EXTERIORFUELEQUIPMENTFUELTYPE "Fuel Type"
#define EXTERIORFUELEQUIPMENTMULTIPLIER "Multiplier"
#define EXTERIORFUELEQUIPMENTSUBCATEGORY "End Use Subcategory"

// EXTERIOR WATER EQUIPMENT
#define EXTERIORWATEREQUIPMENTDEFINITION "Exterior Water Equipment Definition"
#define EXTERIORWATEREQUIPMENTSCHEDULE "Schedule"
#define EXTERIORWATEREQUIPMENTMULTIPLIER "Multiplier"
#define EXTERIORWATEREQUIPMENTSUBCATEGORY "End Use Subcategory"

namespace openstudio {

FacilityExteriorEquipmentGridView::FacilityExteriorEquipmentGridView(bool isIP, const model::Model& model, QWidget* parent)
  : GridViewSubTab(isIP, model, parent) {
  auto modelObjects = subsetCastVector<model::ModelObject>(model.getConcreteModelObjects<model::ExteriorLights>());
  std::sort(modelObjects.begin(), modelObjects.end(), openstudio::WorkspaceObjectNameLess());

  m_gridController = new FacilityExteriorEquipmentGridController(isIP, "Exterior Equipment", IddObjectType::OS_Exterior_Lights, model, modelObjects);
  auto gridView = new OSGridView(m_gridController, "Exterior Equipment", "Drop\nExterior Equipment", false, parent);

  setGridController(m_gridController);
  setGridView(gridView);

  onClearSelection();
}

void FacilityExteriorEquipmentGridView::addObject(const IddObjectType& iddObjectType) {
  if (IddObjectType::OS_Exterior_Lights == iddObjectType.value()) {
    model::ExteriorLights(model::ExteriorLightsDefinition(this->m_model));
  } else if (IddObjectType::OS_Exterior_FuelEquipment == iddObjectType.value()) {
    model::ExteriorFuelEquipment(model::ExteriorFuelEquipmentDefinition(this->m_model));
  } else if (IddObjectType::OS_Exterior_WaterEquipment == iddObjectType.value()) {
    model::ExteriorWaterEquipment(model::ExteriorWaterEquipmentDefinition(this->m_model));
  } else {
    // unhandled
    OS_ASSERT(false);
  }
}

void FacilityExteriorEquipmentGridView::purgeObjects(const IddObjectType& iddObjectType) {
  if (IddObjectType::OS_Exterior_Lights == iddObjectType.value()) {
    for (auto mo : this->m_model.getConcreteModelObjects<model::ExteriorLightsDefinition>()) {
      if (mo.instances().empty()) {
        mo.remove();
      }
    }
  } else if (IddObjectType::OS_Exterior_FuelEquipment == iddObjectType.value()) {
    for (auto mo : this->m_model.getConcreteModelObjects<model::ExteriorFuelEquipmentDefinition>()) {
      if (mo.instances().empty()) {
        mo.remove();
      }
    }
  } else if (IddObjectType::OS_Exterior_WaterEquipment == iddObjectType.value()) {
    for (auto mo : this->m_model.getConcreteModelObjects<model::ExteriorWaterEquipmentDefinition>()) {
      if (mo.instances().empty()) {
        mo.remove();
      }
    }
  } else {
    // unhandled
    OS_ASSERT(false);
  }
}

void FacilityExteriorEquipmentGridView::onSelectItem() {
  //m_itemSelectorButtons->enableAddButton();
  m_itemSelectorButtons->enableCopyButton();
  m_itemSelectorButtons->enableRemoveButton();
  //m_itemSelectorButtons->enablePurgeButton();
}

void FacilityExteriorEquipmentGridView::onClearSelection() {
  //m_itemSelectorButtons->disableAddButton();
  m_itemSelectorButtons->disableCopyButton();
  m_itemSelectorButtons->disableRemoveButton();
  //m_itemSelectorButtons->disablePurgeButton();
}

FacilityExteriorEquipmentGridController::FacilityExteriorEquipmentGridController(bool isIP, const QString& settingsText, IddObjectType iddObjectType,
                                                                                 const model::Model& model,
                                                                                 const std::vector<model::ModelObject>& modelObjects)
  : OSGridController(isIP, settingsText, iddObjectType, model, modelObjects) {
  setCategoriesAndFields();
}

void FacilityExteriorEquipmentGridController::setCategoriesAndFields() {
  {
    std::vector<QString> fields;
    fields.push_back(EXTERIORLIGHTSDEFINITION);
    fields.push_back(EXTERIORLIGHTSSCHEDULE);
    fields.push_back(EXTERIORLIGHTSCONTROLOPTION);
    fields.push_back(EXTERIORLIGHTSMULTIPLIER);
    fields.push_back(EXERIORLIGHTSENDUSESUBCATEGORY);
    std::pair<QString, std::vector<QString>> categoryAndFields = std::make_pair(QString("Exterior Lights"), fields);
    addCategoryAndFields(categoryAndFields);
  }

  {
    std::vector<QString> fields;
    fields.push_back(EXTERIORFUELEQUIPMENTDEFINITION);
    fields.push_back(EXTERIORFUELEQUIPMENTSCHEDULE);
    fields.push_back(EXTERIORFUELEQUIPMENTFUELTYPE);
    fields.push_back(EXTERIORFUELEQUIPMENTMULTIPLIER);
    fields.push_back(EXTERIORFUELEQUIPMENTSUBCATEGORY);
    std::pair<QString, std::vector<QString>> categoryAndFields = std::make_pair(QString("Exterior Fuel Equipment"), fields);
    addCategoryAndFields(categoryAndFields);
  }

  {
    std::vector<QString> fields;
    fields.push_back(EXTERIORWATEREQUIPMENTDEFINITION);
    fields.push_back(EXTERIORWATEREQUIPMENTSCHEDULE);
    fields.push_back(EXTERIORWATEREQUIPMENTMULTIPLIER);
    fields.push_back(EXTERIORWATEREQUIPMENTSUBCATEGORY);
    std::pair<QString, std::vector<QString>> categoryAndFields = std::make_pair(QString("Exterior Water Equipment"), fields);
    addCategoryAndFields(categoryAndFields);
  }

  OSGridController::setCategoriesAndFields();
}

void FacilityExteriorEquipmentGridController::onCategorySelected(int index) {
  if (index == 0) {
    setIddObjectType(IddObjectType::OS_Exterior_Lights);
  } else if (index == 1) {
    setIddObjectType(IddObjectType::OS_Exterior_FuelEquipment);
  } else if (index == 2) {
    setIddObjectType(IddObjectType::OS_Exterior_WaterEquipment);
  } else {
    // unhandled
    OS_ASSERT(false);
  }
  setModelObjects(std::vector<model::ModelObject>());
  OSGridController::onCategorySelected(index);
}

void FacilityExteriorEquipmentGridController::addColumns(const QString& category, std::vector<QString>& fields) {
  // always show name and selected columns
  fields.insert(fields.begin(), {NAME, SELECTED});

  resetBaseConcepts();

  for (const auto& field : fields) {

    if (field == NAME) {

      addParentNameLineEditColumn(Heading(QString(NAME), false, false), false, CastNullAdapter<model::ModelObject>(&model::ModelObject::name),
                                  CastNullAdapter<model::ModelObject>(&model::ModelObject::setName));

    } else if (field == SELECTED) {

      auto checkbox = QSharedPointer<OSSelectAllCheckBox>(new OSSelectAllCheckBox());
      checkbox->setToolTip("Check to select all rows");
      connect(checkbox.data(), &OSSelectAllCheckBox::stateChanged, this, &FacilityExteriorEquipmentGridController::onSelectAllStateChanged);
      connect(this, &FacilityExteriorEquipmentGridController::gridRowSelectionChanged, checkbox.data(),
              &OSSelectAllCheckBox::onGridRowSelectionChanged);
      addSelectColumn(Heading(QString(SELECTED), false, false, checkbox), "Check to select this row");

      // Exterior Lights
    } else if (IddObjectType::OS_Exterior_Lights == iddObjectType().value()) {

      if (field == EXTERIORLIGHTSDEFINITION) {

        std::function<boost::optional<model::ExteriorLightsDefinition>(model::ExteriorLights*)> get([](model::ExteriorLights* el) {
          boost::optional<model::ExteriorLightsDefinition> result;
          if (!el->handle().isNull()) {
            result = el->exteriorLightsDefinition();
          }
          return result;
        });

        addDropZoneColumn(Heading(QString(EXTERIORLIGHTSDEFINITION)), get,
                          CastNullAdapter<model::ExteriorLights>(&model::ExteriorLights::setExteriorLightsDefinition),
                          boost::optional<std::function<void(model::ExteriorLights*)>>());

      } else if (field == EXTERIORLIGHTSSCHEDULE) {

        std::function<bool(model::ExteriorLights*, const model::Schedule&)> set([](model::ExteriorLights* el, const model::Schedule& s) {
          model::Schedule copy = s;
          return el->setSchedule(copy);
        });

        addDropZoneColumn(Heading(QString(EXTERIORLIGHTSSCHEDULE)), CastNullAdapter<model::ExteriorLights>(&model::ExteriorLights::schedule), set,
                          boost::optional<std::function<void(model::ExteriorLights*)>>(
                            CastNullAdapter<model::ExteriorLights>(&model::ExteriorLights::resetSchedule)));

      } else if (field == EXTERIORLIGHTSCONTROLOPTION) {

        addComboBoxColumn<std::string, model::ExteriorLights>(
          Heading(QString(EXTERIORLIGHTSCONTROLOPTION)), static_cast<std::string (*)(const std::string&)>(&openstudio::toString),
          std::function<std::vector<std::string>()>(&model::ExteriorLights::controlOptionValues),
          CastNullAdapter<model::ExteriorLights>(&model::ExteriorLights::controlOption),
          CastNullAdapter<model::ExteriorLights>(&model::ExteriorLights::setControlOption),
          boost::optional<std::function<void(model::ExteriorLights*)>>(
            CastNullAdapter<model::ExteriorLights>(&model::ExteriorLights::resetControlOption)),
          boost::optional<std::function<bool(model::ExteriorLights*)>>(
            CastNullAdapter<model::ExteriorLights>(&model::ExteriorLights::isControlOptionDefaulted)),
          boost::optional<DataSource>());

      } else if (field == EXTERIORLIGHTSMULTIPLIER) {

        addValueEditColumn(Heading(QString(EXTERIORLIGHTSMULTIPLIER)), NullAdapter(&model::ExteriorLights::multiplier),
                           NullAdapter(&model::ExteriorLights::setMultiplier),
                           boost::optional<std::function<void(model::ExteriorLights*)>>(
                             CastNullAdapter<model::ExteriorLights>(&model::ExteriorLights::resetMultiplier)),
                           boost::optional<std::function<bool(model::ExteriorLights*)>>(
                             CastNullAdapter<model::ExteriorLights>(&model::ExteriorLights::isMultiplierDefaulted)),
                           boost::optional<DataSource>());

      } else if (field == EXERIORLIGHTSENDUSESUBCATEGORY) {

        addValueEditColumn(Heading(QString(EXERIORLIGHTSENDUSESUBCATEGORY)),
                           CastNullAdapter<model::ExteriorLights>(&model::ExteriorLights::endUseSubcategory),
                           CastNullAdapter<model::ExteriorLights>(&model::ExteriorLights::setEndUseSubcategory),
                           boost::optional<std::function<void(model::ExteriorLights*)>>(
                             CastNullAdapter<model::ExteriorLights>(&model::ExteriorLights::resetEndUseSubcategory)),
                           boost::optional<std::function<bool(model::ExteriorLights*)>>(
                             CastNullAdapter<model::ExteriorLights>(&model::ExteriorLights::isEndUseSubcategoryDefaulted)),
                           boost::optional<DataSource>());

      } else {
        // unhandled
        OS_ASSERT(false);
      }

      // Exterior Fuel Equipment
    } else if (IddObjectType::OS_Exterior_FuelEquipment == iddObjectType().value()) {

      if (field == EXTERIORFUELEQUIPMENTDEFINITION) {

        std::function<boost::optional<model::ExteriorFuelEquipmentDefinition>(model::ExteriorFuelEquipment*)> get(
          [](model::ExteriorFuelEquipment* el) {
            boost::optional<model::ExteriorFuelEquipmentDefinition> result;
            if (!el->handle().isNull()) {
              result = el->exteriorFuelEquipmentDefinition();
            }
            return result;
          });

        addDropZoneColumn(Heading(QString(EXTERIORFUELEQUIPMENTDEFINITION)), get,
                          CastNullAdapter<model::ExteriorFuelEquipment>(&model::ExteriorFuelEquipment::setExteriorFuelEquipmentDefinition),
                          boost::optional<std::function<void(model::ExteriorFuelEquipment*)>>());

      } else if (field == EXTERIORFUELEQUIPMENTSCHEDULE) {

        std::function<boost::optional<model::Schedule>(model::ExteriorFuelEquipment*)> get(
          [](model::ExteriorFuelEquipment* el) { return el->schedule(); });

        std::function<bool(model::ExteriorFuelEquipment*, const model::Schedule&)> set(
          [](model::ExteriorFuelEquipment* el, const model::Schedule& s) {
            model::Schedule copy = s;
            return el->setSchedule(copy);
          });

        addDropZoneColumn(Heading(QString(EXTERIORFUELEQUIPMENTSCHEDULE)), get, set,
                          boost::optional<std::function<void(model::ExteriorFuelEquipment*)>>());

      } else if (field == EXTERIORFUELEQUIPMENTFUELTYPE) {

        addComboBoxColumn<std::string, model::ExteriorFuelEquipment>(
          Heading(QString(EXTERIORFUELEQUIPMENTFUELTYPE)), static_cast<std::string (*)(const std::string&)>(&openstudio::toString),
          std::function<std::vector<std::string>()>(&model::ExteriorFuelEquipment::fuelTypeValues),
          CastNullAdapter<model::ExteriorFuelEquipment>(&model::ExteriorFuelEquipment::fuelType),
          CastNullAdapter<model::ExteriorFuelEquipment>(&model::ExteriorFuelEquipment::setFuelType),
          boost::optional<std::function<void(model::ExteriorFuelEquipment*)>>(), boost::optional<DataSource>());

      } else if (field == EXTERIORFUELEQUIPMENTMULTIPLIER) {

        addValueEditColumn(Heading(QString(EXTERIORFUELEQUIPMENTMULTIPLIER)), NullAdapter(&model::ExteriorFuelEquipment::multiplier),
                           NullAdapter(&model::ExteriorFuelEquipment::setMultiplier),
                           boost::optional<std::function<void(model::ExteriorFuelEquipment*)>>(
                             CastNullAdapter<model::ExteriorFuelEquipment>(&model::ExteriorFuelEquipment::resetMultiplier)),
                           boost::optional<std::function<bool(model::ExteriorFuelEquipment*)>>(
                             CastNullAdapter<model::ExteriorFuelEquipment>(&model::ExteriorFuelEquipment::isMultiplierDefaulted)),
                           boost::optional<DataSource>());

      } else if (field == EXTERIORFUELEQUIPMENTSUBCATEGORY) {

        addValueEditColumn(Heading(QString(EXTERIORFUELEQUIPMENTSUBCATEGORY)),
                           CastNullAdapter<model::ExteriorFuelEquipment>(&model::ExteriorFuelEquipment::endUseSubcategory),
                           CastNullAdapter<model::ExteriorFuelEquipment>(&model::ExteriorFuelEquipment::setEndUseSubcategory),
                           boost::optional<std::function<void(model::ExteriorFuelEquipment*)>>(
                             CastNullAdapter<model::ExteriorFuelEquipment>(&model::ExteriorFuelEquipment::resetEndUseSubcategory)),
                           boost::optional<std::function<bool(model::ExteriorFuelEquipment*)>>(
                             CastNullAdapter<model::ExteriorFuelEquipment>(&model::ExteriorFuelEquipment::isEndUseSubcategoryDefaulted)),
                           boost::optional<DataSource>());

      } else {
        // unhandled
        OS_ASSERT(false);
      }

      // Exterior Water Equipment
    } else if (IddObjectType::OS_Exterior_WaterEquipment == iddObjectType().value()) {

      if (field == EXTERIORWATEREQUIPMENTDEFINITION) {

        std::function<boost::optional<model::ExteriorWaterEquipmentDefinition>(model::ExteriorWaterEquipment*)> get(
          [](model::ExteriorWaterEquipment* el) {
            boost::optional<model::ExteriorWaterEquipmentDefinition> result;
            if (!el->handle().isNull()) {
              result = el->exteriorWaterEquipmentDefinition();
            }
            return result;
          });

        addDropZoneColumn(Heading(QString(EXTERIORWATEREQUIPMENTDEFINITION)), get,
                          CastNullAdapter<model::ExteriorWaterEquipment>(&model::ExteriorWaterEquipment::setExteriorWaterEquipmentDefinition),
                          boost::optional<std::function<void(model::ExteriorWaterEquipment*)>>());

      } else if (field == EXTERIORWATEREQUIPMENTSCHEDULE) {

        std::function<boost::optional<model::Schedule>(model::ExteriorWaterEquipment*)> get(
          [](model::ExteriorWaterEquipment* el) { return el->schedule(); });

        std::function<bool(model::ExteriorWaterEquipment*, const model::Schedule&)> set(
          [](model::ExteriorWaterEquipment* el, const model::Schedule& s) {
            model::Schedule copy = s;
            return el->setSchedule(copy);
          });

        addDropZoneColumn(Heading(QString(EXTERIORWATEREQUIPMENTSCHEDULE)), get, set,
                          boost::optional<std::function<void(model::ExteriorWaterEquipment*)>>());

      } else if (field == EXTERIORWATEREQUIPMENTMULTIPLIER) {

        addValueEditColumn(Heading(QString(EXTERIORWATEREQUIPMENTMULTIPLIER)), NullAdapter(&model::ExteriorWaterEquipment::multiplier),
                           NullAdapter(&model::ExteriorWaterEquipment::setMultiplier),
                           boost::optional<std::function<void(model::ExteriorWaterEquipment*)>>(
                             CastNullAdapter<model::ExteriorWaterEquipment>(&model::ExteriorWaterEquipment::resetMultiplier)),
                           boost::optional<std::function<bool(model::ExteriorWaterEquipment*)>>(
                             CastNullAdapter<model::ExteriorWaterEquipment>(&model::ExteriorWaterEquipment::isMultiplierDefaulted)),
                           boost::optional<DataSource>());

      } else if (field == EXTERIORWATEREQUIPMENTSUBCATEGORY) {

        addValueEditColumn(Heading(QString(EXTERIORWATEREQUIPMENTSUBCATEGORY)),
                           CastNullAdapter<model::ExteriorWaterEquipment>(&model::ExteriorWaterEquipment::endUseSubcategory),
                           CastNullAdapter<model::ExteriorWaterEquipment>(&model::ExteriorWaterEquipment::setEndUseSubcategory),
                           boost::optional<std::function<void(model::ExteriorWaterEquipment*)>>(
                             CastNullAdapter<model::ExteriorWaterEquipment>(&model::ExteriorWaterEquipment::resetEndUseSubcategory)),
                           boost::optional<std::function<bool(model::ExteriorWaterEquipment*)>>(
                             CastNullAdapter<model::ExteriorWaterEquipment>(&model::ExteriorWaterEquipment::isEndUseSubcategoryDefaulted)),
                           boost::optional<DataSource>());

      } else {
        // unhandled
        OS_ASSERT(false);
      }

    } else {
      // unhandled
      OS_ASSERT(false);
    }
  }
}

QString FacilityExteriorEquipmentGridController::getColor(const model::ModelObject& modelObject) {
  QColor defaultColor(Qt::lightGray);
  return defaultColor.name();
}

void FacilityExteriorEquipmentGridController::checkSelectedFields() {
  if (!this->hasHorizontalHeader()) return;

  OSGridController::checkSelectedFields();
}

void FacilityExteriorEquipmentGridController::onItemDropped(const OSItemId& itemId) {}

void FacilityExteriorEquipmentGridController::refreshModelObjects() {
  if (IddObjectType::OS_Exterior_Lights == iddObjectType().value()) {
    auto lights = model().getConcreteModelObjects<model::ExteriorLights>();
    std::sort(lights.begin(), lights.end(), openstudio::WorkspaceObjectNameLess());
    setModelObjects(subsetCastVector<model::ModelObject>(lights));
  } else if (IddObjectType::OS_Exterior_FuelEquipment == iddObjectType().value()) {
    auto equip = model().getConcreteModelObjects<model::ExteriorFuelEquipment>();
    std::sort(equip.begin(), equip.end(), openstudio::WorkspaceObjectNameLess());
    setModelObjects(subsetCastVector<model::ModelObject>(equip));
  } else if (IddObjectType::OS_Exterior_WaterEquipment == iddObjectType().value()) {
    auto equip = model().getConcreteModelObjects<model::ExteriorWaterEquipment>();
    std::sort(equip.begin(), equip.end(), openstudio::WorkspaceObjectNameLess());
    setModelObjects(subsetCastVector<model::ModelObject>(equip));
  } else {
    // unhandled
    OS_ASSERT(false);
  }
}

void FacilityExteriorEquipmentGridController::onComboBoxIndexChanged(int index) {}

}  // namespace openstudio
