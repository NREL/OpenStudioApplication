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

#include "OSGridController.hpp"

#include "OSCellWrapper.hpp"
#include "OSGridView.hpp"
#include "OSObjectSelector.hpp"

#include "../openstudio_lib/HorizontalTabWidget.hpp"
#include "../openstudio_lib/MainRightColumnController.hpp"
#include "../openstudio_lib/ModelObjectInspectorView.hpp"
#include "../openstudio_lib/ModelObjectItem.hpp"
//#include "../openstudio_lib/ModelSubTabView.hpp"
#include "../openstudio_lib/OSAppBase.hpp"
#include "../openstudio_lib/OSDocument.hpp"
#include "../openstudio_lib/OSDropZone.hpp"
#include "../openstudio_lib/OSItemSelector.hpp"
#include "../openstudio_lib/RenderingColorWidget.hpp"
#include "../openstudio_lib/SchedulesView.hpp"

#include <openstudio/model/Model_Impl.hpp>
#include <openstudio/model/ModelObject_Impl.hpp>

// Which is faster?
#include <openstudio/model/ThermalZone.hpp>
#include <openstudio/model/ThermalZone_Impl.hpp>
// or
// #include <utilities/idd/IddEnums.hxx>

#include <openstudio/utilities/core/Assert.hpp>

#include <QApplication>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QWidget>

#include <iostream>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <iterator>

namespace openstudio {

const std::vector<QColor> OSGridController::m_colors = SchedulesView::initializeColors();

OSGridController::OSGridController()
  : QObject(), m_hasHorizontalHeader(true), m_currentCategoryIndex(0), m_isIP(false), m_horizontalHeaderBtnGrp(nullptr) {
  m_objectSelector = new OSObjectSelector(this);
}

OSGridController::OSGridController(bool isIP, const QString& settingsKey, IddObjectType iddObjectType, const model::Model& model,
                                   const std::vector<model::ModelObject>& modelObjects)
  : QObject(),
    m_iddObjectType(iddObjectType),
    m_modelObjects(modelObjects),
    m_categoriesAndFields(std::vector<std::pair<QString, std::vector<QString>>>()),
    m_baseConcepts(std::vector<QSharedPointer<BaseConcept>>()),
    m_horizontalHeaders(std::vector<QWidget*>()),
    m_hasHorizontalHeader(true),
    m_currentCategory(QString()),
    m_currentCategoryIndex(0),
    m_currentFields(std::vector<QString>()),
    m_customFields(std::vector<QString>()),
    m_model(model),
    m_isIP(isIP),
    m_horizontalHeaderBtnGrp(nullptr),
    m_settingsKey(settingsKey) {
  loadQSettings();

  m_objectSelector = new OSObjectSelector(this);

  connect(m_objectSelector, &OSObjectSelector::inFocus, this, &OSGridController::onInFocus);
  connect(m_objectSelector, &OSObjectSelector::gridRowSelectionChanged, this, &OSGridController::gridRowSelectionChanged);
  connect(m_objectSelector, &OSObjectSelector::gridCellChanged, this, &OSGridController::gridCellChanged);
}

OSGridController::~OSGridController() {
  disconnectFromModelSignals();
  saveQSettings();
}

void OSGridController::loadQSettings() {
  QSettings settings("OpenStudio", m_settingsKey);
  auto temp = settings.value("customFields").toStringList().toVector();
  m_customFields = std::vector<QString>(temp.begin(), temp.end());
}

void OSGridController::saveQSettings() const {
  QSettings settings("OpenStudio", m_settingsKey);
  QVector<QVariant> vector;
  for (unsigned i = 0; i < m_customFields.size(); i++) {
    QVariant variant = m_customFields.at(i);
    vector.push_back(variant);
  }
  QList<QVariant> list = vector.toList();
  settings.setValue("customCategories", list);
}

IddObjectType OSGridController::iddObjectType() const {
  return m_iddObjectType;
}

void OSGridController::setIddObjectType(const IddObjectType& iddObjectType) {
  m_iddObjectType = iddObjectType;
}

bool OSGridController::isIP() const {
  return m_isIP;
}

bool OSGridController::hasHorizontalHeader() const {
  return m_hasHorizontalHeader;
}

std::vector<QWidget*> OSGridController::horizontalHeaders() const {
  return m_horizontalHeaders;
}

void OSGridController::setConstructionColumn(int constructionColumn) {
  m_constructionColumn = constructionColumn;
}

model::Model& OSGridController::model() {
  return m_model;
}

std::vector<model::ModelObject> OSGridController::modelObjects() const {
  return m_modelObjects;
}

void OSGridController::setModelObjects(const std::vector<model::ModelObject>& modelObjects) {
  m_modelObjects = modelObjects;
}
/*
std::vector<model::ModelObject> OSGridController::inheritedModelObjects() const {
  return m_inheritedModelObjects;
}

void OSGridController::setInheritedModelObjects(const std::vector<model::ModelObject>& inheritedModelObjects) {
  m_inheritedModelObjects = inheritedModelObjects;
}
*/
void OSGridController::addCategoryAndFields(const std::pair<QString, std::vector<QString>>& categoryAndFields) {
  m_categoriesAndFields.push_back(categoryAndFields);
}

void OSGridController::resetCategoryAndFields() {
  m_categoriesAndFields.clear();
}

void OSGridController::setCategoriesAndFields() {
  std::vector<QString> fields;
  std::pair<QString, std::vector<QString>> categoryAndFields = std::make_pair(QString("Custom"), fields);
  addCategoryAndFields(categoryAndFields);

  setCustomCategoryAndFields();
}

std::vector<QString> OSGridController::categories() {
  std::vector<QString> categories;

  for (unsigned i = 0; i < m_categoriesAndFields.size(); i++) {
    categories.push_back(m_categoriesAndFields.at(i).first);
  }

  return categories;
}

std::vector<std::pair<QString, std::vector<QString>>> OSGridController::categoriesAndFields() {
  return m_categoriesAndFields;
}

void OSGridController::onCategorySelected(int index) {
  m_objectSelector->clear();
  m_currentCategoryIndex = index;
  m_focusedCellLocation = std::make_tuple(-1, -1, -1);

  m_currentCategory = m_categoriesAndFields.at(index).first;

  m_currentFields = m_categoriesAndFields.at(index).second;

  addColumns(m_currentCategory, m_currentFields);

  // One of the only times we request a recreate all
  emit recreateAll();
}

void OSGridController::setHorizontalHeader(QWidget* gridView) {
  m_horizontalHeaders.clear();

  if (m_horizontalHeaderBtnGrp == nullptr) {
    m_horizontalHeaderBtnGrp = new QButtonGroup();
    m_horizontalHeaderBtnGrp->setExclusive(false);

    connect(m_horizontalHeaderBtnGrp, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::idClicked), this,
            &OSGridController::onHorizontalHeaderChecked);

  } else {
    QList<QAbstractButton*> buttons = m_horizontalHeaderBtnGrp->buttons();
    for (int i = 0; i < buttons.size(); i++) {
      m_horizontalHeaderBtnGrp->removeButton(buttons.at(i));
      OS_ASSERT(buttons.at(i));
      delete buttons.at(i);
    }
  }

  QList<QAbstractButton*> buttons = m_horizontalHeaderBtnGrp->buttons();
  OS_ASSERT(buttons.size() == 0);

  for (const QString& field : m_currentFields) {
    auto horizontalHeaderWidget = new HorizontalHeaderWidget(field, gridView);
    m_horizontalHeaderBtnGrp->addButton(horizontalHeaderWidget->m_checkBox, m_horizontalHeaderBtnGrp->buttons().size());
    m_horizontalHeaders.push_back(horizontalHeaderWidget);
  }

  checkSelectedFields();
}

void OSGridController::setConceptValue(model::ModelObject t_setterMO, model::ModelObject t_getterMO,
                                       const QSharedPointer<BaseConcept>& t_baseConcept) {
  if (QSharedPointer<CheckBoxConcept> concept = t_baseConcept.dynamicCast<CheckBoxConcept>()) {
    auto setter = std::bind(&CheckBoxConcept::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&CheckBoxConcept::get, concept.data(), t_getterMO);
    auto temp = getter();
    setter(temp);
  } else if (QSharedPointer<CheckBoxConceptBoolReturn> concept = t_baseConcept.dynamicCast<CheckBoxConceptBoolReturn>()) {
    auto setter = std::bind(&CheckBoxConceptBoolReturn::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&CheckBoxConceptBoolReturn::get, concept.data(), t_getterMO);
    auto temp = getter();
    setter(temp);
  } else if (QSharedPointer<ComboBoxConcept> concept = t_baseConcept.dynamicCast<ComboBoxConcept>()) {
    auto getterChoiceConcept = concept->choiceConcept(t_getterMO);
    auto setterChoiceConcept = concept->choiceConcept(t_setterMO);
    auto getter = std::bind(&ChoiceConcept::get, getterChoiceConcept.get());
    auto setter = std::bind(&ChoiceConcept::set, setterChoiceConcept.get(), std::placeholders::_1);
    auto temp = getter();
    setter(temp);
  } else if (QSharedPointer<ValueEditConcept<double>> concept = t_baseConcept.dynamicCast<ValueEditConcept<double>>()) {
    auto setter = std::bind(&ValueEditConcept<double>::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&ValueEditConcept<double>::get, concept.data(), t_getterMO);
    auto temp = getter();
    setter(temp);
  } else if (QSharedPointer<OptionalValueEditConcept<double>> concept = t_baseConcept.dynamicCast<OptionalValueEditConcept<double>>()) {
    auto setter = std::bind(&OptionalValueEditConcept<double>::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&OptionalValueEditConcept<double>::get, concept.data(), t_getterMO);
    auto temp = getter();
    if (temp) setter(temp.get());
  } else if (QSharedPointer<ValueEditVoidReturnConcept<double>> concept = t_baseConcept.dynamicCast<ValueEditVoidReturnConcept<double>>()) {
    auto setter = std::bind(&ValueEditVoidReturnConcept<double>::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&ValueEditVoidReturnConcept<double>::get, concept.data(), t_getterMO);
    auto temp = getter();
    setter(temp);
  } else if (QSharedPointer<OptionalValueEditVoidReturnConcept<double>> concept =
               t_baseConcept.dynamicCast<OptionalValueEditVoidReturnConcept<double>>()) {
    auto setter = std::bind(&OptionalValueEditVoidReturnConcept<double>::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&OptionalValueEditVoidReturnConcept<double>::get, concept.data(), t_getterMO);
    auto temp = getter();
    if (temp) setter(temp.get());
  } else if (QSharedPointer<ValueEditConcept<int>> concept = t_baseConcept.dynamicCast<ValueEditConcept<int>>()) {
    auto setter = std::bind(&ValueEditConcept<int>::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&ValueEditConcept<int>::get, concept.data(), t_getterMO);
    auto temp = getter();
    setter(temp);
  } else if (QSharedPointer<ValueEditConcept<std::string>> concept = t_baseConcept.dynamicCast<ValueEditConcept<std::string>>()) {
    auto setter = std::bind(&ValueEditConcept<std::string>::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&ValueEditConcept<std::string>::get, concept.data(), t_getterMO);
    auto temp = getter();
    setter(temp);
  } else if (QSharedPointer<ValueEditVoidReturnConcept<std::string>> concept = t_baseConcept.dynamicCast<ValueEditVoidReturnConcept<std::string>>()) {
    auto setter = std::bind(&ValueEditVoidReturnConcept<std::string>::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&ValueEditVoidReturnConcept<std::string>::get, concept.data(), t_getterMO);
    auto temp = getter();
    setter(temp);
  } else if (QSharedPointer<NameLineEditConcept> concept = t_baseConcept.dynamicCast<NameLineEditConcept>()) {
    auto setter = std::bind(&NameLineEditConcept::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&NameLineEditConcept::get, concept.data(), t_getterMO, true);  // NOTE Evan: Do we always want true?
    auto temp = getter();
    if (temp) setter(temp.get());
  } else if (QSharedPointer<QuantityEditConcept<double>> concept = t_baseConcept.dynamicCast<QuantityEditConcept<double>>()) {
    auto setter = std::bind(&QuantityEditConcept<double>::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&QuantityEditConcept<double>::get, concept.data(), t_getterMO);
    auto temp = getter();
    setter(temp);
  } else if (QSharedPointer<OptionalQuantityEditConcept<double>> concept = t_baseConcept.dynamicCast<OptionalQuantityEditConcept<double>>()) {
    auto setter = std::bind(&OptionalQuantityEditConcept<double>::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&OptionalQuantityEditConcept<double>::get, concept.data(), t_getterMO);
    auto temp = getter();
    if (temp) setter(temp.get());
  } else if (QSharedPointer<QuantityEditVoidReturnConcept<double>> concept = t_baseConcept.dynamicCast<QuantityEditVoidReturnConcept<double>>()) {
    auto setter = std::bind(&QuantityEditVoidReturnConcept<double>::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&QuantityEditVoidReturnConcept<double>::get, concept.data(), t_getterMO);
    auto temp = getter();
    setter(temp);
  } else if (QSharedPointer<OptionalQuantityEditVoidReturnConcept<double>> concept =
               t_baseConcept.dynamicCast<OptionalQuantityEditVoidReturnConcept<double>>()) {
    auto setter = std::bind(&OptionalQuantityEditVoidReturnConcept<double>::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&OptionalQuantityEditVoidReturnConcept<double>::get, concept.data(), t_getterMO);
    auto temp = getter();
    if (temp) setter(temp.get());
  } else if (QSharedPointer<ValueEditConcept<unsigned>> concept = t_baseConcept.dynamicCast<ValueEditConcept<unsigned>>()) {
    auto setter = std::bind(&ValueEditConcept<unsigned>::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&ValueEditConcept<unsigned>::get, concept.data(), t_getterMO);
    auto temp = getter();
    setter(temp);
  } else if (QSharedPointer<DropZoneConcept> concept = t_baseConcept.dynamicCast<DropZoneConcept>()) {
    auto setter = std::bind(&DropZoneConcept::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&DropZoneConcept::get, concept.data(), t_getterMO);
    auto temp = getter();
    if (temp) setter(temp.get());
  } else if (QSharedPointer<RenderingColorConcept> concept = t_baseConcept.dynamicCast<RenderingColorConcept>()) {
    auto setter = std::bind(&RenderingColorConcept::set, concept.data(), t_setterMO, std::placeholders::_1);
    auto getter = std::bind(&RenderingColorConcept::get, concept.data(), t_getterMO);
    auto temp = getter();
    if (temp) setter(temp.get());
  } else {
    // Unknown type
    OS_ASSERT(false);
  }
}

void OSGridController::setConceptValue(model::ModelObject t_setterMO, model::ModelObject t_getterMO,
                                       const QSharedPointer<BaseConcept>& t_setterBaseConcept,
                                       const QSharedPointer<BaseConcept>& t_getterBaseConcept) {
  if (t_getterBaseConcept.dynamicCast<NameLineEditConcept>()) {
    if (QSharedPointer<DropZoneConcept> setterConcept = t_setterBaseConcept.dynamicCast<DropZoneConcept>()) {
      auto mo = m_model.getModelObject<model::ModelObject>(t_getterMO.handle());
      OS_ASSERT(mo);
      auto setter = std::bind(&DropZoneConcept::set, setterConcept.data(), t_setterMO, std::placeholders::_1);
      setter(mo.get());
    }
  } else {
    // No other combination is currently in use
    // Should never get here
    OS_ASSERT(false);
  }
}

void OSGridController::resetConceptValue(model::ModelObject t_resetMO, const QSharedPointer<BaseConcept>& t_baseConcept) {
  if (QSharedPointer<ValueEditConcept<double>> concept = t_baseConcept.dynamicCast<ValueEditConcept<double>>()) {
    auto reset = std::bind(&ValueEditConcept<double>::reset, concept.data(), t_resetMO);
    reset();
  } else if (QSharedPointer<ValueEditVoidReturnConcept<double>> concept = t_baseConcept.dynamicCast<ValueEditVoidReturnConcept<double>>()) {
    auto reset = std::bind(&ValueEditVoidReturnConcept<double>::reset, concept.data(), t_resetMO);
    reset();
  } else if (QSharedPointer<ValueEditConcept<int>> concept = t_baseConcept.dynamicCast<ValueEditConcept<int>>()) {
    auto reset = std::bind(&ValueEditConcept<int>::reset, concept.data(), t_resetMO);
    reset();
  } else if (QSharedPointer<ValueEditConcept<std::string>> concept = t_baseConcept.dynamicCast<ValueEditConcept<std::string>>()) {
    auto reset = std::bind(&ValueEditConcept<std::string>::reset, concept.data(), t_resetMO);
    reset();
  } else if (QSharedPointer<NameLineEditConcept> concept = t_baseConcept.dynamicCast<NameLineEditConcept>()) {
    auto reset = std::bind(&NameLineEditConcept::reset, concept.data(), t_resetMO);
    reset();
  } else if (QSharedPointer<QuantityEditConcept<double>> concept = t_baseConcept.dynamicCast<QuantityEditConcept<double>>()) {
    auto reset = std::bind(&QuantityEditConcept<double>::reset, concept.data(), t_resetMO);
    reset();
  } else if (QSharedPointer<QuantityEditVoidReturnConcept<double>> concept = t_baseConcept.dynamicCast<QuantityEditVoidReturnConcept<double>>()) {
    auto reset = std::bind(&QuantityEditVoidReturnConcept<double>::reset, concept.data(), t_resetMO);
    reset();
  } else if (QSharedPointer<ValueEditConcept<unsigned>> concept = t_baseConcept.dynamicCast<ValueEditConcept<unsigned>>()) {
    auto reset = std::bind(&ValueEditConcept<unsigned>::reset, concept.data(), t_resetMO);
    reset();
  } else if (QSharedPointer<DropZoneConcept> concept = t_baseConcept.dynamicCast<DropZoneConcept>()) {
    auto reset = std::bind(&DropZoneConcept::reset, concept.data(), t_resetMO);
    reset();
  } else {
    // Unknown type
    OS_ASSERT(false);
  }
}

OSCellWrapper* OSGridController::createCellWrapper(int gridRow, int column, OSGridView* gridView) {
  OS_ASSERT(gridRow >= 0);
  OS_ASSERT(column >= 0);

  // Note: If there is a horizontal header row,  m_modelObjects[0] starts on gridLayout[1]
  int modelRow = modelRowFromGridRow(gridRow);  //m_hasHorizontalHeader ? gridRow - 1 : gridRow;

  OS_ASSERT(static_cast<int>(m_modelObjects.size()) > modelRow);
  OS_ASSERT(static_cast<int>(m_baseConcepts.size()) > column);

  QSharedPointer<BaseConcept> baseConcept = m_baseConcepts[column];

  auto wrapper = new OSCellWrapper(gridView, baseConcept, m_objectSelector, modelRow, gridRow, column);

  if (m_hasHorizontalHeader && gridRow == 0) {
    if (column == 0) {
      setHorizontalHeader(gridView);
      // Each concept should have its own column
      OS_ASSERT(m_horizontalHeaders.size() == m_baseConcepts.size());
    }
    const Heading& heading = baseConcept->heading();

    wrapper->layout()->setContentsMargins(0, 1, 1, 0);
    wrapper->addOSWidget(m_horizontalHeaders.at(column), boost::none, false, false);

    HorizontalHeaderWidget* horizontalHeaderWidget = qobject_cast<HorizontalHeaderWidget*>(m_horizontalHeaders.at(column));
    OS_ASSERT(horizontalHeaderWidget);
    if (!heading.showCheckbox()) {
      horizontalHeaderWidget->m_checkBox->hide();
    }
    if (!heading.showButton()) {
      horizontalHeaderWidget->m_pushButton->hide();
    }

    horizontalHeaderWidget->addWidget(heading.widget());
  } else {

    model::ModelObject modelObject = m_modelObjects[modelRow];

    wrapper->setGridController(this);
    wrapper->setModelObject(modelObject);
    wrapper->refresh();
  }

  return wrapper;
}

void OSGridController::checkSelectedFields() {
  // If there is a header row, investigate which columns were previously checked
  // (and loaded into m_customFields by QSettings) and check the respective
  // header widgets
  if (!this->m_hasHorizontalHeader) return;

  std::vector<QString>::iterator it;
  for (unsigned j = 0; j < m_customFields.size(); j++) {
    it = std::find(m_currentFields.begin(), m_currentFields.end(), m_customFields.at(j));
    if (it != m_currentFields.end()) {
      int index = std::distance(m_currentFields.begin(), it);
      HorizontalHeaderWidget* horizontalHeaderWidget = qobject_cast<HorizontalHeaderWidget*>(m_horizontalHeaders.at(index));
      OS_ASSERT(horizontalHeaderWidget);
      horizontalHeaderWidget->m_checkBox->blockSignals(true);
      horizontalHeaderWidget->m_checkBox->setChecked(true);
      horizontalHeaderWidget->m_checkBox->blockSignals(false);
    }
  }
}

void OSGridController::setCustomCategoryAndFields() {
  // First, find and erase the old fields for custom
  std::vector<QString> categories = this->categories();
  std::vector<QString>::iterator it;
  it = std::find(categories.begin(), categories.end(), QString("Custom"));
  if (it != categories.end()) {
    int index = std::distance(categories.begin(), it);
    m_categoriesAndFields.erase(m_categoriesAndFields.begin() + index);
  }

  // Make a new set of fields for custom
  std::pair<QString, std::vector<QString>> categoryAndFields = std::make_pair(QString("Custom"), m_customFields);
  m_categoriesAndFields.push_back(categoryAndFields);
}

int OSGridController::rowCount() const {
  if (m_hasHorizontalHeader) {
    return m_modelObjects.size() + 1;
  } else {
    return m_modelObjects.size();
  }
}

int OSGridController::columnCount() const {
  return m_baseConcepts.size();
}
/*
QWidget* OSGridController::cell(int gridRow, int columnIndex) {
  QWidget* widget = nullptr;

  QLayoutItem* child = gridView()->itemAtPosition(gridRow, columnIndex);
  if (child) {
    widget = child->widget();
  }

  return widget;
}
*/
model::ModelObject OSGridController::modelObjectFromGridRow(int gridRow) {
  return m_modelObjects.at(modelRowFromGridRow(gridRow));
}

int OSGridController::gridRowFromModelRow(int modelRow) {
  if (m_hasHorizontalHeader) {
    return modelRow + 1;
  }
  return modelRow;
}

int OSGridController::modelRowFromGridRow(int gridRow) {
  if (m_hasHorizontalHeader) {
    return gridRow - 1;
  }
  return gridRow;
}

/*
std::vector<QWidget*> OSGridController::row(int gridRow) {
  std::vector<QWidget*> row;

  for (unsigned columnIndex = 0; columnIndex < m_currentFields.size(); columnIndex++) {
    row.push_back(cell(gridRow, columnIndex));
  }

  return row;
}

void OSGridController::selectRow(int gridRow, bool select) {
  std::vector<QWidget*> row = this->row(gridRow);
  for (auto widget : row) {
    auto button = qobject_cast<QPushButton*>(widget);
    if (!button) {
      return;
    }
    button->blockSignals(true);
    button->setChecked(select);
    button->blockSignals(false);
  }
}
*/
void OSGridController::onHorizontalHeaderChecked(int index) {
  // Push_back or erase the field from the user-selected fields
  auto checkBox = qobject_cast<QAbstractButton*>(m_horizontalHeaderBtnGrp->button(index));
  OS_ASSERT(checkBox);
  if (checkBox->isChecked()) {
    m_customFields.push_back(m_currentFields.at(index));
  } else {
    std::vector<QString>::iterator it;
    it = std::find(m_customFields.begin(), m_customFields.end(), m_currentFields.at(index));
    if (it != m_customFields.end()) {
      m_customFields.erase(it);
    }
  }

  // Update the user-selected fields
  setCustomCategoryAndFields();
}

void OSGridController::onToggleUnits(bool displayIP) {
  m_isIP = displayIP;
}

void OSGridController::onComboBoxIndexChanged(int index) {}

//bool OSGridController::selectRowByItem(OSItem * item, bool isSelected)
//{
//  auto success = false;
//  int i = 0;
//
//  for (auto modelObject : m_modelObjects){
//    OSItemId itemId = modelObjectToItemId(modelObject, false);
//    if (item->itemId() == itemId){
//      selectRow(gridRowFromModelIndex(i), isSelected);
//      success = true;
//      break;
//    }
//    i++;
//  }
//  return success;
//}

bool OSGridController::getgridRowByItem(OSItem* item, int& gridRow) {
  auto success = false;
  gridRow = -1;

  for (auto modelObject : m_modelObjects) {
    gridRow++;
    OSItemId itemId = modelObjectToItemId(modelObject, false);
    if (item->itemId() == itemId) {
      success = true;
      break;
    }
  }

  if (!success) {
    // At this point, none of the itemIds exactly matched,
    // let's try to match a subset.
    gridRow = -1;

    QString handle(""), handle2("");
    QStringList strings = item->itemId().otherData().split(",");
    if (strings.size() > 2) {
      QString temp = strings[2];
      strings = temp.split(";");
      if (strings.size() > 0) {
        handle = strings[0];
      }
    }

    for (auto modelObject : m_modelObjects) {
      gridRow++;
      OSItemId itemId = modelObjectToItemId(modelObject, false);
      strings = itemId.otherData().split(",");
      if (strings.size() > 2) {
        QString temp = strings[2];
        strings = temp.split(";");
        if (strings.size() > 0) {
          handle2 = strings[0];
        }
      }

      if (handle == handle2) {
        success = true;
        break;
      }
    }
  }

  if (success) {
    // We found the model index and must convert it to the row index
    gridRow = gridRowFromModelRow(gridRow);
  } else {
    // We could never find a valid index
    gridRow = -1;
  }

  return success;
}
/*
OSItem* OSGridController::getSelectedItemFromModelSubTabView() {
  OSItem* item = nullptr;

  auto modelSubTabView = gridView()->modelSubTabView();
  if (!modelSubTabView) return item;

  item = modelSubTabView->itemSelector()->selectedItem();

  return item;
}
*/
void OSGridController::connectToModelSignals() {
  m_model.getImpl<model::detail::Model_Impl>().get()->addWorkspaceObject.connect<OSGridController, &OSGridController::onAddWorkspaceObject>(this);
  m_model.getImpl<model::detail::Model_Impl>().get()->removeWorkspaceObject.connect<OSGridController, &OSGridController::onRemoveWorkspaceObject>(
    this);
}

void OSGridController::disconnectFromModelSignals() {
  m_model.getImpl<model::detail::Model_Impl>().get()->addWorkspaceObject.disconnect<OSGridController, &OSGridController::onAddWorkspaceObject>(this);
  m_model.getImpl<model::detail::Model_Impl>().get()->removeWorkspaceObject.disconnect<OSGridController, &OSGridController::onRemoveWorkspaceObject>(
    this);
}

void OSGridController::onSelectionCleared() {
  m_objectSelector->clearSelection();
}

void OSGridController::onRemoveWorkspaceObject(const WorkspaceObject& object, const openstudio::IddObjectType& iddObjectType,
                                               const openstudio::UUID& handle) {
  m_objectSelector->setObjectRemoved(handle);
}

void OSGridController::onAddWorkspaceObject(const WorkspaceObject& object, const openstudio::IddObjectType& iddObjectType,
                                            const openstudio::UUID& handle) {
  if (iddObjectType == m_iddObjectType) {
    // note that object is not yet fully constructed
    m_newModelObjects.insert(object.cast<model::ModelObject>());
    QTimer::singleShot(0, this, &OSGridController::processNewModelObjects);
  }
}

void OSGridController::processNewModelObjects() {
  for (const model::ModelObject& newModelObject : m_newModelObjects) {
    m_modelObjects.push_back(newModelObject);
    emit addRow(rowCount() - 1);
  }
  m_newModelObjects.clear();
}

void OSGridController::onSelectAllStateChanged(const int newState) const {
  LOG(Debug, "Select all state changed: " << newState);

  if (newState == 0) {
    m_objectSelector->clearSelection();
  } else {
    m_objectSelector->selectAll();
  }
}

void OSGridController::onInFocus(bool inFocus, bool hasData, int modelRow, int gridRow, int column, boost::optional<int> subrow) {

  // First thing to do is to check if row is 0, because that means that the apply button was clicked
  if (gridRow == 0 && this->m_hasHorizontalHeader) {
    // Set selected objects to the focused object's value for given column
    auto focusedGridRow = std::get<0>(m_focusedCellLocation);
    auto focusedModelRow = modelRowFromGridRow(focusedGridRow);
    auto focusedColumn = std::get<1>(m_focusedCellLocation);
    auto focusedSubrow = std::get<2>(m_focusedCellLocation);

    if (focusedGridRow < 0) {
      // something has gone wrong
      return;
    }

    OS_ASSERT(focusedColumn == column);

    std::set<model::ModelObject> selectedObjects = this->m_objectSelector->selectedObjects();
    boost::optional<model::ModelObject> focusedObject =
      this->m_objectSelector->getObject(focusedModelRow, focusedGridRow, focusedColumn, focusedSubrow);
    if (!focusedObject) {
      // we don't have a focused object to apply values from
      return;
    }

    QSharedPointer<DataSourceAdapter> dataSource = m_baseConcepts[column].dynamicCast<DataSourceAdapter>();
    if (focusedSubrow && dataSource) {
      // Sub rows present, either in a widget, or in a row
      const DataSource& source = dataSource->source();
      QSharedPointer<BaseConcept> dropZoneConcept = source.dropZoneConcept();
      for (auto modelObject : selectedObjects) {
        // Don't set the chosen object when iterating through the selected objects
        if (modelObject != focusedObject.get()) {
          OS_ASSERT(dataSource.data()->innerConcept());
          if (dropZoneConcept) {
            // Widget has sub rows
            setConceptValue(modelObject, focusedObject.get(), dropZoneConcept, dataSource.data()->innerConcept());
          } else {
            // Row has sub rows
            setConceptValue(modelObject, focusedObject.get(), dataSource.data()->innerConcept());
          }
        }
      }
    } else if (!focusedSubrow) {
      for (auto modelObject : selectedObjects) {
        // Don't set the chosen object when iterating through the selected objects
        if (modelObject != focusedObject.get()) {
          setConceptValue(modelObject, focusedObject.get(), m_baseConcepts[column]);
        }
      }
    } else {
      // Should never get here
      OS_ASSERT(false);
    }

  } else {
    // not in a header row, an object was selected
    OS_ASSERT(gridRow >= 0);

    HorizontalHeaderWidget* horizontalHeaderWidget = qobject_cast<HorizontalHeaderWidget*>(m_horizontalHeaders.at(column));
    OS_ASSERT(horizontalHeaderWidget);
    auto button = horizontalHeaderWidget->m_pushButton;
    OS_ASSERT(button);

    if (inFocus) {
      m_focusedCellLocation = std::make_tuple(gridRow, column, subrow);
      button->setText("Apply to Selected");
    } else {
      // do not reset m_focusedCellLocation here because the focused cell goes out of focus when the apply button is clicked
      button->setText("Apply to Selected");
    }

    m_applyToButtonStates.push_back(std::make_pair(column, inFocus && hasData));

    QTimer::singleShot(0, this, &OSGridController::onSetApplyButtonState);
  }
}

void OSGridController::onSetApplyButtonState() {
  for (auto pair : m_applyToButtonStates) {
    HorizontalHeaderWidget* horizontalHeaderWidget = qobject_cast<HorizontalHeaderWidget*>(m_horizontalHeaders.at(pair.first));
    OS_ASSERT(horizontalHeaderWidget);
    auto button = horizontalHeaderWidget->m_pushButton;
    OS_ASSERT(button);
    button->setEnabled(pair.second);
  }

  m_applyToButtonStates.clear();
}

std::set<model::ModelObject> OSGridController::selectorObjects() const {
  return m_objectSelector->selectorObjects();
}

std::set<model::ModelObject> OSGridController::selectableObjects() const {
  return m_objectSelector->selectableObjects();
}

std::set<model::ModelObject> OSGridController::selectedObjects() const {
  return m_objectSelector->selectedObjects();
}

void OSGridController::clearObjectSelector() {
  m_objectSelector->clear();
}

std::function<bool(const model::ModelObject&)> OSGridController::objectFilter() const {
  return m_objectSelector->objectFilter();
}

void OSGridController::setObjectFilter(const std::function<bool(const model::ModelObject&)>& t_filter) {
  m_objectSelector->setObjectFilter(t_filter);
}

std::function<bool(const model::ModelObject&)> OSGridController::objectIsLocked() const {
  return m_objectSelector->objectIsLocked();
}

void OSGridController::setObjectIsLocked(const std::function<bool(const model::ModelObject&)>& t_isLocked) {
  m_objectSelector->setObjectIsLocked(t_isLocked);
}

QSharedPointer<BaseConcept> OSGridController::makeDataSourceAdapter(const QSharedPointer<BaseConcept>& t_inner,
                                                                    const boost::optional<DataSource>& t_source) {
  if (t_source) {
    return QSharedPointer<BaseConcept>(new DataSourceAdapter(*t_source, t_inner));
  } else {
    // if there is no t_source passed in, we don't want to wrap, just pass through
    return t_inner;
  }
}

void OSGridController::resetBaseConcepts() {
  m_baseConcepts.clear();
}

HorizontalHeaderPushButton::HorizontalHeaderPushButton(QWidget* parent) : QPushButton() {
  QString style = "QPushButton {"
                  "    font-size: 8pt;"
                  "}";
  setStyleSheet(style);
  setFocusPolicy(Qt::StrongFocus);
}

HorizontalHeaderPushButton::~HorizontalHeaderPushButton() {}

void HorizontalHeaderPushButton::focusInEvent(QFocusEvent* e) {
  if (e->reason() == Qt::MouseFocusReason) {
    emit inFocus(true, true);  // TODO should hasData = true???
  }

  QPushButton::focusInEvent(e);
}

void HorizontalHeaderPushButton::focusOutEvent(QFocusEvent* e) {
  if (e->reason() == Qt::MouseFocusReason) {
    emit inFocus(false, false);
  }

  QPushButton::focusOutEvent(e);
}

HorizontalHeaderWidget::HorizontalHeaderWidget(const QString& fieldName, QWidget* parent)
  : QWidget(parent), m_label(new QLabel(fieldName, this)), m_checkBox(new QPushButton(this)), m_pushButton(new HorizontalHeaderPushButton(this)) {
  auto mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 5);
  mainLayout->setAlignment(Qt::AlignCenter);
  setLayout(mainLayout);

  mainLayout->addWidget(m_checkBox);
  m_checkBox->setToolTip("Check to add this column to \"Custom\"");
  m_checkBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  QString style = "\
    QPushButton {\
      border: none;\
      background: #808080;\
      padding: 0px;\
    }\
    QPushButton:checked {\
      background: #94b3de;\
    }\
  ";

  m_checkBox->setStyleSheet(style);
  m_checkBox->setCheckable(true);

  m_label->setWordWrap(true);
  m_label->setStyleSheet("QLabel { padding: 5px; }");
  m_label->setAlignment(Qt::AlignHCenter);
  mainLayout->addWidget(m_label);

  mainLayout->addStretch();

  m_pushButton->setText("Apply to Selected");
  m_pushButton->setMaximumWidth(150);
  m_pushButton->setEnabled(false);
  connect(m_pushButton, &HorizontalHeaderPushButton::inFocus, this, &HorizontalHeaderWidget::inFocus);
  mainLayout->addWidget(m_pushButton, 0, Qt::AlignCenter);
}

HorizontalHeaderWidget::~HorizontalHeaderWidget() {
  for (auto& widget : m_addedWidgets) {
    layout()->removeWidget(widget.data());
    widget->setVisible(false);
    widget->setParent(nullptr);
  }
}

void HorizontalHeaderWidget::addWidget(const QSharedPointer<QWidget>& t_widget) {
  if (!t_widget.isNull()) {
    m_addedWidgets.push_back(t_widget);
    auto hLayout = new QHBoxLayout();
    hLayout->setContentsMargins(5, 0, 5, 0);
    qobject_cast<QVBoxLayout*>(layout())->addLayout(hLayout);
    hLayout->addWidget(t_widget.data());
    t_widget->setVisible(true);
  }
}

}  // namespace openstudio
