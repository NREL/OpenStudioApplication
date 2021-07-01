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

#include "DesignDayGridView.hpp"

#include "../shared_gui_components/OSCheckBox.hpp"
#include "../shared_gui_components/OSGridView.hpp"
#include "../shared_gui_components/OSObjectSelector.hpp"

#include "ModelObjectItem.hpp"
#include "OSAppBase.hpp"
#include "OSDocument.hpp"
#include "OSDropZone.hpp"

#include <openstudio/model/DesignDay.hpp>
#include <openstudio/model/DesignDay_Impl.hpp>
#include <openstudio/model/ScheduleDay.hpp>
#include <openstudio/model/ScheduleDay_Impl.hpp>

#include <openstudio/utilities/core/Compare.hpp>
#include <openstudio/utilities/idd/IddEnums.hxx>
#include <openstudio/utilities/idd/SizingPeriod_DesignDay_FieldEnums.hxx>

#include <QBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QScrollArea>
#include <QSettings>
#include <QTimer>

// These defines provide a common area for field display names
// used on column headers, and other grid widgets

#define NAME openstudio::DesignDayGridView::tr("Design Day Name")
#define SELECTED openstudio::DesignDayGridView::tr("All")

// DATE
#define DAYOFMONTH openstudio::DesignDayGridView::tr("Day Of Month")
#define MONTH openstudio::DesignDayGridView::tr("Month")
#define DAYTYPE openstudio::DesignDayGridView::tr("Day Type")
#define DAYLIGHTSAVINGTIMEINDICATOR openstudio::DesignDayGridView::tr("Daylight Saving Time Indicator")

// TEMPERATURE
#define MAXIMUMDRYBULBTEMPERATURE openstudio::DesignDayGridView::tr("Maximum Dry Bulb Temperature")
#define DAILYDRYBULBTEMPERATURERANGE openstudio::DesignDayGridView::tr("Daily Dry Bulb Temperature Range")
#define DAILYWETBULBTEMPERATURERANGE openstudio::DesignDayGridView::tr("Daily Wet Bulb Temperature Range")
#define DRYBULBTEMPERATURERANGEMODIFIERTYPE openstudio::DesignDayGridView::tr("Dry Bulb Temperature Range Modifier Type")
#define DRYBULBTEMPERATURERANGEMODIFIERSCHEDULE openstudio::DesignDayGridView::tr("Dry Bulb Temperature Range Modifier Schedule")

// HUMIDITY
#define HUMIDITYINDICATINGCONDITIONSATMAXIMUMDRYBULB openstudio::DesignDayGridView::tr("Humidity Indicating Conditions At Maximum Dry Bulb")
#define HUMIDITYINDICATINGTYPE openstudio::DesignDayGridView::tr("Humidity Indicating Type")
#define HUMIDITYINDICATINGDAYSCHEDULE openstudio::DesignDayGridView::tr("Humidity Indicating Day Schedule")

// PRESSURE / WIND / PRECIP
#define BAROMETRICPRESSURE openstudio::DesignDayGridView::tr("Barometric Pressure")
#define WINDSPEED openstudio::DesignDayGridView::tr("Wind Speed")
#define WINDDIRECTION openstudio::DesignDayGridView::tr("Wind Direction")
#define RAININDICATOR openstudio::DesignDayGridView::tr("Rain Indicator")
#define SNOWINDICATOR openstudio::DesignDayGridView::tr("Snow Indicator")

// SOLAR
#define SOLARMODELINDICATOR openstudio::DesignDayGridView::tr("Solar Model Indicator")
#define BEAMSOLARDAYSCHEDULE openstudio::DesignDayGridView::tr("Beam Solar Day Schedule")
#define DIFFUSESOLARDAYSCHEDULE openstudio::DesignDayGridView::tr("Diffuse Solar Day Schedule")
#define ASHRAETAUB openstudio::DesignDayGridView::tr("ASHRAE Taub")
#define ASHRAETAUD openstudio::DesignDayGridView::tr("ASHRAE Taud")
#define SKYCLEARNESS openstudio::DesignDayGridView::tr("Sky Clearness")

namespace openstudio {

DesignDayGridView::DesignDayGridView(bool isIP, const model::Model& model, QWidget* parent) : QWidget(parent), m_isIP(isIP) {
  auto layout = new QVBoxLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  setLayout(layout);

  auto designDays = model.getConcreteModelObjects<model::DesignDay>();
  auto designDayModelObjects = subsetCastVector<model::ModelObject>(designDays);

  m_gridController = new DesignDayGridController(m_isIP, tr("Design Days"), IddObjectType::OS_SizingPeriod_DesignDay, model, designDayModelObjects);
  m_gridView = new OSGridView(m_gridController, tr("Design Days"), tr("Drop\nZone"), true, parent);

  connect(m_gridView, &OSGridView::dropZoneItemClicked, this, &DesignDayGridView::dropZoneItemClicked);

  //isConnected = connect(this, SIGNAL(itemSelected(OSItem *)), gridView, SIGNAL(itemSelected(OSItem*)));
  //OS_ASSERT(isConnected);
  //isConnected = connect(this, SIGNAL(selectionCleared()), gridView, SLOT(onSelectionCleared()));
  //OS_ASSERT(isConnected);
  //isConnected = connect(gridView, SIGNAL(gridRowSelected(OSItem*)), this, SIGNAL(gridRowSelected(OSItem*)));
  //OS_ASSERT(isConnected);

  m_gridView->showDropZone(false);

  layout->addWidget(m_gridView, 0, Qt::AlignTop);

  layout->addStretch(1);

  connect(this, &DesignDayGridView::toggleUnitsClicked, m_gridController, &DesignDayGridController::toggleUnitsClicked);

  connect(this, &DesignDayGridView::toggleUnitsClicked, m_gridController, &DesignDayGridController::onToggleUnits);
}

void DesignDayGridView::onAddClicked() {
  // Always make at least one
  if (!m_gridController->selectedObjects().size()) {
    model::DesignDay(m_gridController->model());
  } else {
    for (auto& obj : m_gridController->selectedObjects()) {
      // TODO: copy properties of selected to new?
      addObject(obj);
    }
  }
}

void DesignDayGridView::onCopyClicked() {
  for (auto& obj : m_gridController->selectedObjects()) {
    if (!obj.handle().isNull()) {
      copyObject(obj);
    }
  }
}

void DesignDayGridView::onRemoveClicked() {
  for (auto& obj : m_gridController->selectedObjects()) {
    removeObject(obj);
  }
}

void DesignDayGridView::onPurgeClicked() {
  purgeObjects(IddObjectType::OS_SizingPeriod_DesignDay);
}

void DesignDayGridView::addObject(const model::ModelObject& modelObject) {
  model::DesignDay(m_gridController->model());
}

void DesignDayGridView::copyObject(const openstudio::model::ModelObject& modelObject) {
  modelObject.clone(m_gridController->model());
}

void DesignDayGridView::removeObject(openstudio::model::ModelObject modelObject) {
  modelObject.remove();
}

void DesignDayGridView::purgeObjects(const IddObjectType& iddObjectType) {
  for (auto mo : m_gridController->model().getConcreteModelObjects<model::DesignDay>()) {
    mo.remove();
  }
}

std::set<model::ModelObject> DesignDayGridView::selectedObjects() const {
  return m_gridController->selectedObjects();
}

DesignDayGridController::DesignDayGridController(bool isIP, const QString& headerText, IddObjectType iddObjectType, const model::Model& model,
                                                 const std::vector<model::ModelObject>& modelObjects)
  : OSGridController(isIP, headerText, iddObjectType, model, modelObjects) {
  setCategoriesAndFields();
}

void DesignDayGridController::setCategoriesAndFields() {
  {
    std::vector<QString> fields;
    fields.push_back(DAYOFMONTH);
    fields.push_back(MONTH);
    fields.push_back(DAYTYPE);
    fields.push_back(DAYLIGHTSAVINGTIMEINDICATOR);
    std::pair<QString, std::vector<QString>> categoryAndFields = std::make_pair(tr("Date"), fields);
    addCategoryAndFields(categoryAndFields);
  }

  {
    std::vector<QString> fields;
    fields.push_back(MAXIMUMDRYBULBTEMPERATURE);
    fields.push_back(DAILYDRYBULBTEMPERATURERANGE);
    fields.push_back(DAILYWETBULBTEMPERATURERANGE);
    fields.push_back(DRYBULBTEMPERATURERANGEMODIFIERTYPE);
    fields.push_back(DRYBULBTEMPERATURERANGEMODIFIERSCHEDULE);
    std::pair<QString, std::vector<QString>> categoryAndFields = std::make_pair(tr("Temperature"), fields);
    addCategoryAndFields(categoryAndFields);
  }

  {
    std::vector<QString> fields;
    fields.push_back(HUMIDITYINDICATINGCONDITIONSATMAXIMUMDRYBULB);
    fields.push_back(HUMIDITYINDICATINGTYPE);
    fields.push_back(HUMIDITYINDICATINGDAYSCHEDULE);
    std::pair<QString, std::vector<QString>> categoryAndFields = std::make_pair(tr("Humidity"), fields);
    addCategoryAndFields(categoryAndFields);
  }

  {
    std::vector<QString> fields;
    fields.push_back(BAROMETRICPRESSURE);
    fields.push_back(WINDSPEED);
    fields.push_back(WINDDIRECTION);
    fields.push_back(RAININDICATOR);
    fields.push_back(SNOWINDICATOR);
    std::pair<QString, std::vector<QString>> categoryAndFields = std::make_pair(tr("Pressure\nWind\nPrecipitation"), fields);
    addCategoryAndFields(categoryAndFields);
  }

  {
    std::vector<QString> fields;
    fields.push_back(SOLARMODELINDICATOR);
    fields.push_back(BEAMSOLARDAYSCHEDULE);
    fields.push_back(DIFFUSESOLARDAYSCHEDULE);
    fields.push_back(ASHRAETAUB);
    fields.push_back(ASHRAETAUD);
    fields.push_back(SKYCLEARNESS);
    std::pair<QString, std::vector<QString>> categoryAndFields = std::make_pair(tr("Solar"), fields);
    addCategoryAndFields(categoryAndFields);
  }

  OSGridController::setCategoriesAndFields();
}

void DesignDayGridController::addColumns(const QString& /*category*/, std::vector<QString>& fields) {
  // always show name column
  fields.insert(fields.begin(), {NAME, SELECTED});

  resetBaseConcepts();

  for (const QString& field : fields) {
    // Evan note: addCheckBoxColumn does not yet handle reset and default
    if (field == DAYLIGHTSAVINGTIMEINDICATOR) {
      // We add the "Apply Selected" button to this column by passing 3rd arg, t_showColumnButton=true
      addCheckBoxColumn(Heading(DAYLIGHTSAVINGTIMEINDICATOR, true, true), std::string("Check to enable daylight saving time indicator."),
                        NullAdapter(&model::DesignDay::daylightSavingTimeIndicator), NullAdapter(&model::DesignDay::setDaylightSavingTimeIndicator));
    } else if (field == RAININDICATOR) {
      // We add the "Apply Selected" button to this column by passing 3rd arg, t_showColumnButton=true
      addCheckBoxColumn(Heading(RAININDICATOR, true, true), std::string("Check to enable rain indicator."),
                        NullAdapter(&model::DesignDay::rainIndicator), NullAdapter(&model::DesignDay::setRainIndicator));
    } else if (field == SNOWINDICATOR) {
      // We add the "Apply Selected" button to this column by passing 3rd arg, t_showColumnButton=true
      addCheckBoxColumn(Heading(SNOWINDICATOR, true, true), std::string("Check to enable snow indicator."),
                        NullAdapter(&model::DesignDay::snowIndicator), NullAdapter(&model::DesignDay::setSnowIndicator));
    } else if (field == SELECTED) {
      auto checkbox = QSharedPointer<OSSelectAllCheckBox>(new OSSelectAllCheckBox());
      checkbox->setToolTip(tr("Check to select all rows"));
      connect(checkbox.data(), &OSSelectAllCheckBox::stateChanged, this, &DesignDayGridController::onSelectAllStateChanged);
      connect(this, &DesignDayGridController::gridRowSelectionChanged, checkbox.data(), &OSSelectAllCheckBox::onGridRowSelectionChanged);
      addSelectColumn(Heading(SELECTED, false, false, checkbox), "Check to select this row");
    }
    // INTEGER
    else if (field == DAYOFMONTH) {
      addValueEditColumn(
        Heading(DAYOFMONTH), NullAdapter(&model::DesignDay::dayOfMonth), NullAdapter(&model::DesignDay::setDayOfMonth),
        boost::optional<std::function<void(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::resetDayOfMonth)),
        boost::optional<std::function<bool(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::isDayOfMonthDefaulted)),
        boost::optional<DataSource>());
    } else if (field == MONTH) {
      addValueEditColumn(
        Heading(MONTH), NullAdapter(&model::DesignDay::month), NullAdapter(&model::DesignDay::setMonth),
        boost::optional<std::function<void(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::resetMonth)),
        boost::optional<std::function<bool(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::isMonthDefaulted)),
        boost::optional<DataSource>());
    }
    // DOUBLE
    else if (field == SKYCLEARNESS) {
      addValueEditColumn(
        Heading(SKYCLEARNESS), NullAdapter(&model::DesignDay::skyClearness), NullAdapter(&model::DesignDay::setSkyClearness),
        boost::optional<std::function<void(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::resetSkyClearness)),
        boost::optional<std::function<bool(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::isSkyClearnessDefaulted)),
        boost::optional<DataSource>());
    } else if (field == ASHRAETAUB) {
      addValueEditColumn(
        Heading(ASHRAETAUB), NullAdapter(&model::DesignDay::ashraeTaub), NullAdapter(&model::DesignDay::setAshraeTaub),
        boost::optional<std::function<void(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::resetAshraeTaub)),
        boost::optional<std::function<bool(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::isAshraeTaubDefaulted)),
        boost::optional<DataSource>());
    } else if (field == ASHRAETAUD) {
      addValueEditColumn(
        Heading(ASHRAETAUD), NullAdapter(&model::DesignDay::ashraeTaud), NullAdapter(&model::DesignDay::setAshraeTaud),
        boost::optional<std::function<void(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::resetAshraeTaud)),
        boost::optional<std::function<bool(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::isAshraeTaudDefaulted)),
        boost::optional<DataSource>());
    } else if (field == WINDDIRECTION) {
      addValueEditColumn(
        Heading(WINDDIRECTION), NullAdapter(&model::DesignDay::windDirection), NullAdapter(&model::DesignDay::setWindDirection),
        boost::optional<std::function<void(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::resetWindDirection)),
        boost::optional<std::function<bool(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::isWindDirectionDefaulted)),
        boost::optional<DataSource>());
    }
    // STRING
    else if (field == NAME) {
      addParentNameLineEditColumn(Heading(NAME, false, false), false, CastNullAdapter<model::DesignDay>(&model::DesignDay::name),
                                  CastNullAdapter<model::DesignDay>(&model::DesignDay::setName));
    } else if (field == MAXIMUMDRYBULBTEMPERATURE) {
      addQuantityEditColumn(
        Heading(MAXIMUMDRYBULBTEMPERATURE), QString("C"), QString("C"), QString("F"), isIP(),
        NullAdapter(&model::DesignDay::maximumDryBulbTemperature), NullAdapter(&model::DesignDay::setMaximumDryBulbTemperature),
        boost::optional<std::function<void(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::resetMaximumDryBulbTemperature)),
        boost::optional<std::function<bool(model::DesignDay*)>>(
          CastNullAdapter<model::DesignDay>(&model::DesignDay::isMaximumDryBulbTemperatureDefaulted)),
        boost::optional<DataSource>());
    } else if (field == DAILYDRYBULBTEMPERATURERANGE) {
      addQuantityEditColumn(Heading(DAILYDRYBULBTEMPERATURERANGE), QString("K"), QString("K"), QString("R"), isIP(),
                            NullAdapter(&model::DesignDay::dailyDryBulbTemperatureRange),
                            NullAdapter(&model::DesignDay::setDailyDryBulbTemperatureRange),
                            boost::optional<std::function<void(model::DesignDay*)>>(
                              CastNullAdapter<model::DesignDay>(&model::DesignDay::resetDailyDryBulbTemperatureRange)),
                            boost::optional<std::function<bool(model::DesignDay*)>>(
                              CastNullAdapter<model::DesignDay>(&model::DesignDay::isDailyDryBulbTemperatureRangeDefaulted)),
                            boost::optional<DataSource>());
    }

    // This should be reset when the humidity indication condition type is changed to something incompatible
    else if (field == HUMIDITYINDICATINGCONDITIONSATMAXIMUMDRYBULB) {
      addQuantityEditColumn(Heading(HUMIDITYINDICATINGCONDITIONSATMAXIMUMDRYBULB), QString("C"), QString("C"), QString("F"), isIP(),
                            NullAdapter(&model::DesignDay::humidityIndicatingConditionsAtMaximumDryBulb),
                            NullAdapter(&model::DesignDay::setHumidityIndicatingConditionsAtMaximumDryBulb),
                            boost::optional<std::function<void(model::DesignDay*)>>(
                              CastNullAdapter<model::DesignDay>(&model::DesignDay::resetHumidityIndicatingConditionsAtMaximumDryBulb)),
                            boost::optional<std::function<bool(model::DesignDay*)>>(
                              CastNullAdapter<model::DesignDay>(&model::DesignDay::isHumidityIndicatingConditionsAtMaximumDryBulbDefaulted)),
                            boost::optional<DataSource>());
    }

    else if (field == BAROMETRICPRESSURE) {
      addQuantityEditColumn(
        Heading(BAROMETRICPRESSURE), QString("Pa"), QString("Pa"), QString("inHg"), isIP(), NullAdapter(&model::DesignDay::barometricPressure),
        NullAdapter(&model::DesignDay::setBarometricPressure),
        boost::optional<std::function<void(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::resetBarometricPressure)),
        boost::optional<std::function<bool(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::isBarometricPressureDefaulted)),
        boost::optional<DataSource>());
    } else if (field == WINDSPEED) {
      addQuantityEditColumn(
        Heading(WINDSPEED), QString("m/s"), QString("m/s"), QString("miles/hr"), isIP(), NullAdapter(&model::DesignDay::windSpeed),
        NullAdapter(&model::DesignDay::setWindSpeed),
        boost::optional<std::function<void(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::resetWindSpeed)),
        boost::optional<std::function<bool(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::isWindSpeedDefaulted)),
        boost::optional<DataSource>());
    } else if (field == DAILYWETBULBTEMPERATURERANGE) {
      addQuantityEditColumn(Heading(DAILYWETBULBTEMPERATURERANGE), QString("K"), QString("K"), QString("R"), isIP(),
                            NullAdapter(&model::DesignDay::dailyWetBulbTemperatureRange),
                            NullAdapter(&model::DesignDay::setDailyWetBulbTemperatureRange));
    } else if (field == DAYTYPE) {
      addComboBoxColumn<std::string, model::DesignDay>(
        Heading(DAYTYPE), static_cast<std::string (*)(const std::string&)>(&openstudio::toString),
        std::function<std::vector<std::string>()>(&model::DesignDay::validDayTypeValues),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::dayType), CastNullAdapter<model::DesignDay>(&model::DesignDay::setDayType),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::resetDayType), CastNullAdapter<model::DesignDay>(&model::DesignDay::isDayTypeDefaulted),
        boost::optional<DataSource>());
    } else if (field == DRYBULBTEMPERATURERANGEMODIFIERTYPE) {
      addComboBoxColumn<std::string, model::DesignDay>(
        Heading(DRYBULBTEMPERATURERANGEMODIFIERTYPE), static_cast<std::string (*)(const std::string&)>(&openstudio::toString),
        std::function<std::vector<std::string>()>(&model::DesignDay::validDryBulbTemperatureRangeModifierTypeValues),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::dryBulbTemperatureRangeModifierType),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::setDryBulbTemperatureRangeModifierType),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::resetDryBulbTemperatureRangeModifierType),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::isDryBulbTemperatureRangeModifierTypeDefaulted), boost::optional<DataSource>());
    } else if (field == HUMIDITYINDICATINGTYPE) {
      // I don't see any problem here, yet the box stays empty. Checked the ReverseTranslator, it does its job too
      addComboBoxColumn<std::string, model::DesignDay>(
        Heading(HUMIDITYINDICATINGTYPE), static_cast<std::string (*)(const std::string&)>(&openstudio::toString),
        std::function<std::vector<std::string>()>(&model::DesignDay::validHumidityIndicatingTypeValues),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::humidityIndicatingType),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::setHumidityIndicatingType),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::resetHumidityIndicatingType),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::isHumidityIndicatingTypeDefaulted), boost::optional<DataSource>());
    } else if (field == SOLARMODELINDICATOR) {
      addComboBoxColumn<std::string, model::DesignDay>(
        Heading(SOLARMODELINDICATOR), static_cast<std::string (*)(const std::string&)>(&openstudio::toString),
        std::function<std::vector<std::string>()>(&model::DesignDay::validSolarModelIndicatorValues),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::solarModelIndicator),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::setSolarModelIndicator),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::resetSolarModelIndicator),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::isSolarModelIndicatorDefaulted), boost::optional<DataSource>());
    } else if (field == DRYBULBTEMPERATURERANGEMODIFIERSCHEDULE) {
      addDropZoneColumn(Heading(DRYBULBTEMPERATURERANGEMODIFIERSCHEDULE),
                        CastNullAdapter<model::DesignDay>(&model::DesignDay::dryBulbTemperatureRangeModifierSchedule),
                        CastNullAdapter<model::DesignDay>(&model::DesignDay::setDryBulbTemperatureRangeModifierSchedule),
                        boost::optional<std::function<void(model::DesignDay*)>>(
                          CastNullAdapter<model::DesignDay>(&model::DesignDay::resetDryBulbTemperatureRangeModifierSchedule)));
    }

    else if (field == BEAMSOLARDAYSCHEDULE) {
      addDropZoneColumn(
        Heading(BEAMSOLARDAYSCHEDULE), CastNullAdapter<model::DesignDay>(&model::DesignDay::beamSolarDaySchedule),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::setBeamSolarDaySchedule),
        boost::optional<std::function<void(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::resetBeamSolarDaySchedule)));
    }

    else if (field == HUMIDITYINDICATINGDAYSCHEDULE) {
      addDropZoneColumn(Heading(HUMIDITYINDICATINGDAYSCHEDULE), CastNullAdapter<model::DesignDay>(&model::DesignDay::humidityIndicatingDaySchedule),
                        CastNullAdapter<model::DesignDay>(&model::DesignDay::setHumidityIndicatingDaySchedule),
                        boost::optional<std::function<void(model::DesignDay*)>>(
                          CastNullAdapter<model::DesignDay>(&model::DesignDay::resetHumidityIndicatingDaySchedule)));
    }

    else if (field == DIFFUSESOLARDAYSCHEDULE) {
      addDropZoneColumn(
        Heading(DIFFUSESOLARDAYSCHEDULE), CastNullAdapter<model::DesignDay>(&model::DesignDay::diffuseSolarDaySchedule),
        CastNullAdapter<model::DesignDay>(&model::DesignDay::setDiffuseSolarDaySchedule),
        boost::optional<std::function<void(model::DesignDay*)>>(CastNullAdapter<model::DesignDay>(&model::DesignDay::resetDiffuseSolarDaySchedule)));
    } else {
      // unhandled
      OS_ASSERT(false);
    }
  }
}

QString DesignDayGridController::getColor(const model::ModelObject& modelObject) {
  QColor defaultColor(Qt::lightGray);
  QString color(defaultColor.name());

  return color;
}

void DesignDayGridController::checkSelectedFields() {
  if (!this->hasHorizontalHeader()) return;

  OSGridController::checkSelectedFields();
}

void DesignDayGridController::onItemDropped(const OSItemId& itemId) {
  boost::optional<model::ModelObject> modelObject = OSAppBase::instance()->currentDocument()->getModelObject(itemId);
  if (modelObject) {
    if (modelObject->optionalCast<model::DesignDay>()) {
      modelObject->clone(model());
    }
  }
}

void DesignDayGridController::refreshModelObjects() {
  auto designDays = model().getConcreteModelObjects<model::DesignDay>();
  std::sort(designDays.begin(), designDays.end(), openstudio::WorkspaceObjectNameLess());
  setModelObjects(subsetCastVector<model::ModelObject>(designDays));
}

void DesignDayGridController::onComboBoxIndexChanged(int index) {}

}  // namespace openstudio
