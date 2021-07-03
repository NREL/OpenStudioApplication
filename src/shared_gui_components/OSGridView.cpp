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
#include "OSCellWrapper.hpp"
#include "OSCheckBox.hpp"
#include "OSComboBox.hpp"
#include "OSDoubleEdit.hpp"
#include "OSGridView.hpp"
#include "OSIntegerEdit.hpp"
#include "OSLineEdit.hpp"
#include "OSLoadNamePixmapLineEdit.hpp"
#include "OSObjectSelector.hpp"
#include "OSQuantityEdit.hpp"
#include "OSUnsignedEdit.hpp"
#include "OSWidgetHolder.hpp"

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

constexpr int NUM_ROWS_PER_GRIDLAYOUT = 51;

namespace openstudio {

QGridLayout* OSGridView::makeGridLayout() {
  auto gridLayout = new QGridLayout();
  gridLayout->setSpacing(0);
  gridLayout->setContentsMargins(0, 0, 0, 0);
  gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  return gridLayout;
}

OSGridView::OSGridView(OSGridController* gridController, const QString& headerText, const QString& dropZoneText, bool useHeader, QWidget* parent)
  : QWidget(parent), m_dropZone(nullptr), m_contentLayout(nullptr), m_collapsibleView(nullptr), m_gridController(gridController) {

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
  int li = layoutIndex(row);
  int ri = rowInLayout(row);
  OS_ASSERT(li < m_gridLayouts.size());
  return m_gridLayouts[li]->itemAtPosition(ri, column);
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
  int li = layoutIndex(location.gridRow);
  int ri = rowInLayout(location.gridRow);
  OS_ASSERT(li < m_gridLayouts.size());
  QLayoutItem* item = m_gridLayouts[li]->itemAtPosition(ri, location.column);
  if (item) {
    OSCellWrapper* wrapper = qobject_cast<OSCellWrapper*>(item->widget());
    OS_ASSERT(wrapper);

    // style the wrapper and/or any subrows
    wrapper->setCellProperties(location, info);
  }
}

void OSGridView::deleteAll() {
  QLayoutItem* child;
  for (auto gridLayout : m_gridLayouts) {
    m_contentLayout->removeItem(gridLayout);
    while ((child = gridLayout->takeAt(0)) != nullptr) {
      QWidget* widget = child->widget();

      OS_ASSERT(widget);

      delete widget;
      // Using deleteLater is actually slower than calling delete directly on the widget
      // deleteLater also introduces a strange redraw issue where the select all check box
      // is not redrawn, after being checked.
      //widget->deleteLater();

      delete child;
    }
    delete gridLayout;
  }
  m_gridLayouts.clear();
  m_columnWidths.clear();
}

void OSGridView::addRow(int row) {
  setUpdatesEnabled(false);

  int li = layoutIndex(row);
  while (li >= m_gridLayouts.size()) {
    auto gridLayout = makeGridLayout();
    m_gridLayouts.push_back(gridLayout);
    m_contentLayout->addLayout(gridLayout);
  }

  const auto numRows = m_gridController->rowCount();
  OS_ASSERT(row < numRows);
  const auto numColumns = m_gridController->columnCount();
  for (int j = 0; j < numColumns; j++) {
    createCellWrapper(row, j);
  }

  setUpdatesEnabled(true);
}

void OSGridView::recreateAll() {
  setUpdatesEnabled(false);

  deleteAll();

  if (m_gridController) {
    m_gridController->refreshModelObjects();
    auto objectFilter = m_gridController->objectFilter();
    auto objectIsLocked = m_gridController->objectIsLocked();
    m_gridController->clearObjectSelector();

    const auto numRows = m_gridController->rowCount();
    const auto numColumns = m_gridController->columnCount();
    for (int i = 0; i < numRows; i++) {
      for (int j = 0; j < numColumns; j++) {
        createCellWrapper(i, j);
      }
    }

    m_gridController->setObjectFilter(objectFilter);
    m_gridController->setObjectIsLocked(objectIsLocked);

    setUpdatesEnabled(true);

    //QTimer::singleShot(0, this, SLOT(selectRowDeterminedByModelSubTabView()));
  }
}

constexpr int OSGridView::layoutIndex(int row) const {
  return row / NUM_ROWS_PER_GRIDLAYOUT;
}

constexpr int OSGridView::rowInLayout(int row) const {
  return row % NUM_ROWS_PER_GRIDLAYOUT;
}

void OSGridView::updateColumnWidths() {
  m_columnWidths.clear();

  if (m_gridLayouts.size() > 0) {
    if (m_gridLayouts[0]->rowCount() > 0) {
      int numColumns = m_gridLayouts[0]->columnCount();
      for (int column = 0; column < numColumns; ++column) {
        QLayoutItem* item = m_gridLayouts[0]->itemAtPosition(0, column);
        OS_ASSERT(item);
        OSCellWrapper* wrapper = qobject_cast<OSCellWrapper*>(item->widget());
        OS_ASSERT(wrapper);
        m_columnWidths.push_back(wrapper->width());
      }
    }
  }
}

int OSGridView::widthForColumn(int column) const {
  OS_ASSERT(column < m_columnWidths.size());
  return m_columnWidths[column];
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
void OSGridView::createCellWrapper(int row, int column) {
  OS_ASSERT(m_gridController);

  OSCellWrapper* widget = m_gridController->createCellWrapper(row, column, this);
  LOG_FREE(Debug, "OSGridView", "Creating cell wrapper row=" << row << ", col=" << column);

  addCellWrapper(widget, row, column);
}

void OSGridView::addCellWrapper(OSCellWrapper* w, int row, int column) {

  int li = layoutIndex(row);
  int ri = rowInLayout(row);
  while (li >= m_gridLayouts.size()) {
    auto gridLayout = makeGridLayout();
    m_gridLayouts.push_back(gridLayout);
    m_contentLayout->addLayout(gridLayout);
  }

  if (li > 0) {
    if (m_columnWidths.empty()) {
      updateColumnWidths();
    }
    w->setFixedWidth(widthForColumn(column));
  }

  m_gridLayouts[li]->addWidget(w, ri, column);
}

void OSGridView::hideEvent(QHideEvent* event) {
  m_gridController->disconnectFromModelSignals();

  QWidget::hideEvent(event);
}

void OSGridView::showEvent(QShowEvent* event) {
  m_gridController->connectToModelSignals();

  QWidget::showEvent(event);
}

void OSGridView::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);

  updateColumnWidths();

  if (m_gridLayouts.size() > 0) {
    const auto numRows = m_gridController->rowCount();
    const auto numColumns = m_gridController->columnCount();
    for (int i = NUM_ROWS_PER_GRIDLAYOUT; i < numRows; i++) {
      int li = layoutIndex(i);
      int ri = rowInLayout(i);
      for (int j = 0; j < numColumns; j++) {
        QLayoutItem* item = m_gridLayouts[li]->itemAtPosition(ri, j);
        if (item) {
          OSCellWrapper* wrapper = qobject_cast<OSCellWrapper*>(item->widget());
          if (wrapper) {
            wrapper->setFixedWidth(widthForColumn(j));
          }
        }
      }
    }
  }
}

}  // namespace openstudio
