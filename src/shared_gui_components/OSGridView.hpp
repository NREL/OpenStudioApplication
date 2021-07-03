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

#ifndef SHAREDGUICOMPONENTS_OSGRIDVIEW_HPP
#define SHAREDGUICOMPONENTS_OSGRIDVIEW_HPP

#include <QTimer>
#include <QWidget>

#include "../openstudio_lib/OSItem.hpp"

#include <openstudio/model/ModelObject.hpp>

class QGridLayout;
class QHideEvent;
class QVBoxLayout;
class QLabel;
class QShowEvent;
class QString;
class QLayoutItem;
class OpenStudioLibFixture;

namespace openstudio {

class OSCellWrapper;
class OSCollapsibleView;
class OSDropZone;
class OSGridController;
class OSItem;
class GridCellLocation;
class GridCellInfo;

class OSGridView : public QWidget
{
  Q_OBJECT

 public:
  OSGridView(OSGridController* gridController, const QString& headerText, const QString& dropZoneText, bool useHeader, QWidget* parent = nullptr);

  virtual ~OSGridView();

  // return the QLayoutItem at a particular row and column
  QLayoutItem* itemAtPosition(int row, int column);

  void showDropZone(bool visible);

  void addLayoutToContentLayout(QLayout* layout);

  void addSpacingToContentLayout(int spacing);

 protected:
  virtual void hideEvent(QHideEvent* event) override;

  virtual void showEvent(QShowEvent* event) override;

  virtual void resizeEvent(QResizeEvent* event) override;

 signals:

  void dropZoneItemClicked(OSItem* item);

  void gridRowSelectionChanged(int numSelected, int numSelectable);

 public slots:

  //void requestRemoveRow(int row);

  void onAddRow(int row);

  void onRecreateAll();

  void onGridCellChanged(const GridCellLocation& location, const GridCellInfo& info);

 private:
  // For testing
  friend class OpenStudioLibFixture;

  // uses the OSGridController to create the OSCellWrapper for row, column
  void createCellWrapper(int row, int column);

  // delete all widgets
  void deleteAll();

  // add a row
  void addRow(int row);

  // recreate all widgets
  void recreateAll();

  // convert row to layout index
  constexpr int layoutIndex(int row) const;

  // get row in layout
  constexpr int rowInLayout(int row) const;

  // update column widths
  void updateColumnWidths();

  // get width for column
  int widthForColumn(int column) const;

  // construct a grid layout to our specs
  QGridLayout* makeGridLayout();

  // Add a widget, adding a new layout if necessary
  void addCellWrapper(OSCellWrapper* w, int row, int column);

  OSDropZone* m_dropZone;

  QVBoxLayout* m_contentLayout;

  std::vector<QGridLayout*> m_gridLayouts;

  std::vector<int> m_columnWidths;

  OSCollapsibleView* m_collapsibleView;

  OSGridController* m_gridController;
};

}  // namespace openstudio

#endif  // SHAREDGUICOMPONENTS_OSGRIDVIEW_HPP
