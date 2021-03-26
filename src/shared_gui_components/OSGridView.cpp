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

#include "OSGridView.hpp"

#include "HeaderViews.hpp"
#include "OSCollapsibleView.hpp"
#include "OSGridController.hpp"
#include "OSCheckBox.hpp"
#include "OSComboBox.hpp"
#include "OSDoubleEdit.hpp"
#include "OSGridView.hpp"
#include "OSIntegerEdit.hpp"
#include "OSLineEdit.hpp"
#include "OSLoadNamePixmapLineEdit.hpp"
#include "OSQuantityEdit.hpp"
#include "OSUnsignedEdit.hpp"

#include "../model_editor/Application.hpp"

#include "../openstudio_lib/ModelObjectInspectorView.hpp"
#include "../openstudio_lib/OSDropZone.hpp"
#include "../openstudio_lib/OSItem.hpp"

#include <openstudio/model/Model_Impl.hpp>
#include <openstudio/model/ModelObject_Impl.hpp>

#include <openstudio/utilities/core/Assert.hpp>
#include <openstudio/utilities/idd/IddObject.hpp>

#include <QApplication>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QHideEvent>
#include <QLabel>
#include <QPushButton>
//#include <QScrollArea>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStyle>

#ifdef Q_OS_DARWIN
#  define WIDTH 110
#  define HEIGHT 60
#  define WIDTH_DZ 98
#  define HEIGHT_DZ 48
#else
#  define WIDTH 80
#  define HEIGHT 60
#endif

namespace openstudio {

QGridLayout* OSGridView::makeGridLayout() {
  auto gridLayout = new QGridLayout();
  gridLayout->setSpacing(0);
  gridLayout->setContentsMargins(0, 0, 0, 0);
  //gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  return gridLayout;
}

OSGridView::OSGridView(OSGridController* gridController, const QString& headerText, const QString& dropZoneText, bool useHeader, QWidget* parent)
  : QWidget(parent),
    m_dropZone(nullptr),
    m_contentLayout(nullptr),
    m_gridLayout(nullptr),
    m_collapsibleView(nullptr),
    m_gridController(gridController) {

  // We use the headerText as the object name, will help in indentifying objects for any warnings
  setObjectName(headerText);

  m_gridController->setParent(this);
  connect(m_gridController, &OSGridController::recreateAll, this, &OSGridView::onRecreateAll);
  connect(m_gridController, &OSGridController::addRow, this, &OSGridView::onAddRow);
  connect(m_gridController, &OSGridController::gridCellChanged, this, &OSGridView::onGridCellChanged);
  connect(m_gridController, &OSGridController::gridRowSelectionChanged, this, &OSGridView::gridRowSelectionChanged);

  /** Set up buttons for Categories: eg: SpaceTypes tab: that's the dropzone "Drop Space Type", "General", "Loads", "Measure Tags", "Custom"
   * QHBoxLayout manages the visual representation: they are placed side by side
   * QButtonGroup manages the state of the buttons in the group. By default a QButtonGroup is exclusive (only one button can be checked at one time)
   */
  auto buttonGroup = new QButtonGroup();
  connect(buttonGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::idClicked), m_gridController, &OSGridController::onCategorySelected);

  auto buttonLayout = new QHBoxLayout();
  buttonLayout->setSpacing(3);
  buttonLayout->setContentsMargins(10, 10, 10, 10);
  buttonLayout->setAlignment(Qt::AlignCenter | Qt::AlignLeft);

  auto vectorController = new GridViewDropZoneVectorController();
  m_dropZone = new OSDropZone(vectorController, dropZoneText);
  m_dropZone->setMaxItems(1);

  connect(m_dropZone, &OSDropZone::itemDropped, m_gridController, &OSGridController::onItemDropped);

  buttonLayout->addWidget(m_dropZone);

  std::vector<QString> categories = m_gridController->categories();
  for (unsigned i = 0; i < categories.size(); i++) {
    auto button = new QPushButton(categories.at(i));
    button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    button->setCheckable(true);
    buttonLayout->addWidget(button);
    buttonGroup->addButton(button, buttonGroup->buttons().size());
  }

  /** QVBoxLayout is the main gridview layout, it's where we'll place vertically the different elements:
   * * buttonLayout at the top
   * * Filter
   * * Then the model-dependent data, such as Space Type Name, All, Rendering Color, Default Construction set, etc
   */
  auto layout = new QVBoxLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  setLayout(layout);

  // Add the header first
  auto widget = new QWidget;

  if (useHeader) {
    // If we use the header, we place a blue to dark blue header with for eg: 'Space Types' as text
    // Its content is widget
    auto header = new DarkGradientHeader();
    header->label->setText(headerText);
    auto collabsibleView = new OSCollapsibleView(true);
    collabsibleView->setHeader(header);
    collabsibleView->setContent(widget);
    collabsibleView->setExpanded(true);
    layout->addWidget(collabsibleView);
  } else {
    // Otherwise we only place widget
    layout->addWidget(widget);
  }
  // Create a QLayout object with a parent of widget: This will set widget's layout to this QVBoxLayout already.
  // widget has a layout that is a QVBoxLayout
  m_contentLayout = new QVBoxLayout(widget);
  m_contentLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  m_contentLayout->setSpacing(0);
  m_contentLayout->setContentsMargins(0, 0, 0, 0);
  // widget->setLayout(m_contentLayout);
  //
  // We place the button Layout at the top
  m_contentLayout->addLayout(buttonLayout);
  widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

  // Make the first button checked by default
  QVector<QAbstractButton*> buttons = buttonGroup->buttons().toVector();
  if (buttons.size() > 0) {
    QPushButton* button = qobject_cast<QPushButton*>(buttons.at(0));
    OS_ASSERT(button);
    button->setChecked(true);
    m_gridController->blockSignals(true);
    m_gridController->onCategorySelected(0);  // would normally trigger refreshAll
    m_gridController->blockSignals(false);
  }

  QTimer::singleShot(0, this, &OSGridView::recreateAll);
}

OSGridView::~OSGridView(){};

/*
void OSGridView::requestAddRow(int row) {
  // std::cout << "REQUEST ADDROW CALLED " << std::endl;
  setEnabled(false);

  m_timer.start();

  m_rowToAdd = row;

  m_queueRequests.emplace_back(AddRow);
}

void OSGridView::requestRemoveRow(int row) {
  // std::cout << "REQUEST REMOVEROW CALLED " << std::endl;
  setEnabled(false);

  m_timer.start();

  m_rowToRemove = row;

  m_queueRequests.emplace_back(RemoveRow);
}
*/
//void OSGridView::refreshRow(int row)
//{
//  for( int j = 0; j < m_gridController->columnCount(); j++ )
//  {
//    refreshCell(row, j);
//  }
//}

QLayoutItem* OSGridView::itemAtPosition(int row, int column) {
  return m_gridLayout->itemAtPosition(row, column);
}

void OSGridView::showDropZone(bool visible) {
  m_dropZone->setVisible(visible);
}

void OSGridView::addLayoutToContentLayout(QLayout* layout) {
  m_contentLayout->addLayout(layout);
}

void OSGridView::addSpacingToContentLayout(int spacing) {
  m_contentLayout->addSpacing(spacing);
}

QString OSGridView::cellStyle() {
  const static QString style = "QWidget#TableCell[selected=\"true\"]{ border: none; background-color: #94b3de; border-top: 1px solid black;  "
                               "border-right: 1px solid black; border-bottom: 1px solid black;}"
                               "QWidget#TableCell[selected=\"false\"][even=\"true\"] { border: none; background-color: #ededed; border-top: 1px "
                               "solid black; border-right: 1px solid black; border-bottom: 1px solid black;}"
                               "QWidget#TableCell[selected=\"false\"][even=\"false\"] { border: none; background-color: #cecece; border-top: 1px "
                               "solid black; border-right: 1px solid black; border-bottom: 1px solid black;}";

  return style;
}

void OSGridView::setCellProperties(QWidget* wrapper, bool isSelector, int rowIndex, int columnIndex, boost::optional<int> subrow, bool isVisible,
                                   bool isSelected, bool isLocked) {
  bool isEven = ((rowIndex % 2) == 0);
  bool isChanged = false;

  wrapper->setVisible(isVisible);

  QVariant currentSelected = wrapper->property("selected");
  if (currentSelected.isNull() || currentSelected.toBool() != isSelected) {
    wrapper->setProperty("selected", isSelected);
    isChanged = true;
  }

  QVariant currentEven = wrapper->property("even");
  if (currentEven.isNull() || currentEven.toBool() != isSelected) {
    wrapper->setProperty("even", isEven);
    isChanged = true;
  }

  if (isChanged) {
    wrapper->style()->unpolish(wrapper);
    wrapper->style()->polish(wrapper);
  }
}

//void OSGridView::removeWidget(int row, int column)
//{
//  // Currently this is cruft code
//  OS_ASSERT(false);
//
//  auto layoutnum = row / ROWS_PER_LAYOUT;
//  auto relativerow = row % ROWS_PER_LAYOUT;
//
//  auto index = m_gridController->columnCount() * relativerow; // this assumes that you are always deleting from the zero index column
//
//  auto count = m_gridLayouts.at(layoutnum)->count();
//
//  QLayoutItem * item = m_gridLayouts.at(layoutnum)->itemAtPosition(relativerow, column);
//
//  if (!item) return;
//
//  QLayoutItem * item2 = m_gridLayouts.at(layoutnum)->takeAt(index);
//
//  OS_ASSERT(item2);
//
//  if (item == item2){
//    QString("Good");
//  }
//  else {
//    QString("Bad");
//  }
//
//  QWidget * widget = item->widget();
//
//  if (!widget) return;
//
//  auto indexOfWidget = m_gridLayouts.at(layoutnum)->indexOf(widget);
//
//  delete widget;
//
//  delete item;
//}

void OSGridView::onAddRow(int row) {
  setEnabled(false);
  addRow(row);
  setEnabled(true);
}

void OSGridView::onRecreateAll() {
  setEnabled(false);
  recreateAll();
  setEnabled(true);
}

void OSGridView::onGridCellChanged(const GridCellLocation& location, const GridCellInfo& info) {
  QLayoutItem* item = m_gridLayout->itemAtPosition(location.gridRow, location.column);
  if (item) {
    QWidget* wrapper = item->widget();
    QGridLayout* innerLayout = qobject_cast<QGridLayout*>(wrapper->layout());
    if (innerLayout) {
      QLayoutItem* innerItem;
      // If it is a subrow, we get the subrow
      if (location.subrow) {
        innerItem = innerLayout->itemAtPosition(location.subrow.get(), 0);
      } else {
        innerItem = innerLayout->itemAtPosition(0, 0);
      }
      OS_ASSERT(innerItem);
      Holder* holder = qobject_cast<Holder*>(innerItem->widget());
      OS_ASSERT(holder);
      QWidget* widget = holder->widget;

      if (info.isSelector) {
        auto check = qobject_cast<QCheckBox*>(widget);
        if (check) {
          check->blockSignals(true);
          check->setChecked(info.isSelected());
          check->blockSignals(false);
        }
      }

      // lock the widget if needed, probably a sign we need a base class with setLocked
      if (OSComboBox2* comboBox = qobject_cast<OSComboBox2*>(widget)) {
        comboBox->setLocked(info.isLocked());
      } else if (OSDoubleEdit2* doubleEdit = qobject_cast<OSDoubleEdit2*>(widget)) {
        doubleEdit->setLocked(info.isLocked());
      } else if (OSIntegerEdit2* integerEdit = qobject_cast<OSIntegerEdit2*>(widget)) {
        integerEdit->setLocked(info.isLocked());
      } else if (OSQuantityEdit2* quantityEdit = qobject_cast<OSQuantityEdit2*>(widget)) {
        quantityEdit->setLocked(info.isLocked());
      } else if (OSLineEdit2* lineEdit = qobject_cast<OSLineEdit2*>(widget)) {
        lineEdit->setLocked(info.isLocked());
      } else if (OSUnsignedEdit2* unsignedEdit = qobject_cast<OSUnsignedEdit2*>(widget)) {
        unsignedEdit->setLocked(info.isLocked());
      } else if (OSDropZone2* dropZone = qobject_cast<OSDropZone2*>(widget)) {
        dropZone->setLocked(info.isLocked());
      } else if (OSCheckBox3* checkBox = qobject_cast<OSCheckBox3*>(widget)) {
        checkBox->setLocked(info.isLocked());
        if (info.isSelector) {
          checkBox->blockSignals(true);
          checkBox->setChecked(info.isSelected());
          checkBox->blockSignals(false);
        }
      }

      // style the wrapper
      setCellProperties(wrapper, info.isSelector, location.gridRow, location.column, location.subrow, info.isVisible(), info.isSelected(),
                        info.isLocked());
    }
  }
}

void OSGridView::deleteAll() {
  QLayoutItem* child;
  while ((child = m_gridLayout->takeAt(0)) != nullptr) {
    QWidget* widget = child->widget();

    OS_ASSERT(widget);

    delete widget;
    // Using deleteLater is actually slower than calling delete directly on the widget
    // deleteLater also introduces a strange redraw issue where the select all check box
    // is not redrawn, after being checked.
    //widget->deleteLater();

    delete child;
  }
}

void OSGridView::addRow(int row) {
  setUpdatesEnabled(false);

  OS_ASSERT(m_gridLayout);
  OS_ASSERT(m_gridController);

  const auto numRows = m_gridController->rowCount();
  OS_ASSERT(row < numRows);
  const auto numColumns = m_gridController->columnCount();
  for (int j = 0; j < numColumns; j++) {
    createWidget(row, j);
  }

  setUpdatesEnabled(true);
}

void OSGridView::recreateAll() {
  setUpdatesEnabled(false);

  if (!m_gridLayout) {
    // create grid layout here so it is underneath the filters or other things added by concreate gridviews in m_contentLayout
    m_gridLayout = makeGridLayout();
    m_contentLayout->addLayout(m_gridLayout);
  }

  deleteAll();

  if (m_gridController) {
    m_gridController->refreshModelObjects();
    m_gridController->m_objectSelector->clear();

    const auto numRows = m_gridController->rowCount();
    const auto numColumns = m_gridController->columnCount();
    for (int i = 0; i < numRows; i++) {
      for (int j = 0; j < numColumns; j++) {
        createWidget(i, j);
      }
    }

    setUpdatesEnabled(true);

    //QTimer::singleShot(0, this, SLOT(selectRowDeterminedByModelSubTabView()));
  }
}

//void OSGridView::refreshRow(int row) {
// TODO: fix
//  this->m_gridController->getObjectSelector()->updateWidgetsForRow(row);
//}
/*
void OSGridView::selectRowDeterminedByModelSubTabView() {
  // Get selected item
  auto selectedItem = m_gridController->getSelectedItemFromModelSubTabView();
  if (!selectedItem) return;

  // Get new index
  int newIndex;
  if (m_gridController->getRowIndexByItem(selectedItem, newIndex)) {
    // Update the old index
    m_gridController->m_oldIndex = newIndex;
  }

  // If the index is valid, call slot
  //if (m_gridController->m_oldIndex > -1) {
  //  QTimer::singleShot(0, this, SLOT(doRowSelect()));
  //}
}
*/
/*
void OSGridView::doRowSelect() {
  // If the index is valid, do some work
  if (m_gridController->m_oldIndex > -1) {
    m_gridController->selectRow(m_gridController->m_oldIndex, true);
  }
}
*/
void OSGridView::createWidget(int row, int column) {
  OS_ASSERT(m_gridController);

  QWidget* widget = m_gridController->createWidget(row, column, this);

  addWidget(widget, row, column);
}

void OSGridView::addWidget(QWidget* w, int row, int column) {
  m_gridLayout->addWidget(w, row, column);
}

void OSGridView::updateWidget(int row, int column) {
  // todo
}

/*
ModelSubTabView* OSGridView::modelSubTabView() {
  ModelSubTabView* modelSubTabView = nullptr;

  if (!this->parent() || !this->parent()->parent()) return modelSubTabView;

  auto stackedWidget = qobject_cast<QStackedWidget*>(this->parent()->parent());
  if (!stackedWidget) return modelSubTabView;

  auto widget = qobject_cast<QWidget*>(stackedWidget->parent());
  if (!widget) return modelSubTabView;

  auto scrollArea = qobject_cast<QScrollArea*>(widget->parent());
  if (!scrollArea) return modelSubTabView;

  auto modelObjectInspectorView = qobject_cast<ModelObjectInspectorView*>(scrollArea->parent());
  if (!modelObjectInspectorView) return modelSubTabView;

  auto object = qobject_cast<QObject*>(modelObjectInspectorView->parent());
  if (!object) return modelSubTabView;

  modelSubTabView = qobject_cast<ModelSubTabView*>(object->parent());
  return modelSubTabView;
}
*/
void OSGridView::hideEvent(QHideEvent* event) {
  m_gridController->disconnectFromModelSignals();

  QWidget::hideEvent(event);
}

void OSGridView::showEvent(QShowEvent* event) {
  m_gridController->connectToModelSignals();

  QWidget::showEvent(event);
}

}  // namespace openstudio
