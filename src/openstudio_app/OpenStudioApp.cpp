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

#include "OpenStudioApp.hpp"
#include <openstudio_app/AboutBox.hpp>
#include "StartupMenu.hpp"
#include "StartupView.hpp"
#include "LibraryDialog.hpp"
#include "ExternalToolsDialog.hpp"
#include "../openstudio_lib/MainWindow.hpp"
#include "../openstudio_lib/OSDocument.hpp"

#include "../model_editor/AccessPolicyStore.hpp"
#include "../model_editor/GithubReleases.hpp"
#include "../model_editor/Utilities.hpp"

#include "../shared_gui_components/WaitDialog.hpp"
#include "../shared_gui_components/MeasureManager.hpp"

// Call the OS App specific version, not the core one
#include "../utilities/OpenStudioApplicationPathHelpers.hpp"
#include <openstudio/utilities/core/PathHelpers.hpp>

#include <openstudio/utilities/core/Assert.hpp>
#include <openstudio/utilities/core/Compare.hpp>
#include <openstudio/utilities/core/Filesystem.hpp>

#include <openstudio/utilities/idf/IdfFile.hpp>
#include <openstudio/utilities/idf/IdfObject.hpp>
#include <openstudio/utilities/idf/ValidityReport.hpp>
#include <openstudio/utilities/idf/Workspace.hpp>

#include <openstudio/model/EvaporativeFluidCoolerSingleSpeed.hpp>
#include <openstudio/model/AirLoopHVACOutdoorAirSystem.hpp>
#include <openstudio/model/AirLoopHVACUnitaryHeatPumpAirToAir.hpp>
#include <openstudio/model/AirLoopHVACUnitarySystem.hpp>
#include <openstudio/model/AirTerminalSingleDuctConstantVolumeCooledBeam.hpp>
#include <openstudio/model/AirTerminalSingleDuctConstantVolumeReheat.hpp>
#include <openstudio/model/AirTerminalSingleDuctParallelPIUReheat.hpp>
#include <openstudio/model/AirTerminalSingleDuctConstantVolumeNoReheat.hpp>
#include <openstudio/model/AirTerminalSingleDuctVAVReheat.hpp>
#include <openstudio/model/AirTerminalSingleDuctVAVNoReheat.hpp>
#include <openstudio/model/BuildingStory.hpp>
#include <openstudio/model/CoilCoolingCooledBeam.hpp>
#include <openstudio/model/CoilCoolingDXSingleSpeed.hpp>
#include <openstudio/model/CoilCoolingDXTwoSpeed.hpp>
#include <openstudio/model/CoilCoolingWaterToAirHeatPumpEquationFit.hpp>
#include <openstudio/model/CoilHeatingDXSingleSpeed.hpp>
#include <openstudio/model/CoilHeatingElectric.hpp>
#include <openstudio/model/CoilHeatingGas.hpp>
#include <openstudio/model/CoilHeatingGas_Impl.hpp>
#include <openstudio/model/CoilHeatingWaterBaseboard.hpp>
#include <openstudio/model/CoilHeatingWaterToAirHeatPumpEquationFit.hpp>
#include <openstudio/model/ControllerOutdoorAir.hpp>
#include <openstudio/model/CurveBiquadratic.hpp>
#include <openstudio/model/CurveQuadratic.hpp>
#include <openstudio/model/DistrictCooling.hpp>
#include <openstudio/model/DistrictHeating.hpp>
#include <openstudio/model/EvaporativeCoolerDirectResearchSpecial.hpp>
#include <openstudio/model/FanConstantVolume.hpp>
#include <openstudio/model/FanOnOff.hpp>
#include <openstudio/model/FanVariableVolume.hpp>
#include <openstudio/model/FanZoneExhaust.hpp>
#include <openstudio/model/GeneratorFuelCellExhaustGasToWaterHeatExchanger.hpp>
#include <openstudio/model/GeneratorMicroTurbineHeatRecovery.hpp>
#include <openstudio/model/Model.hpp>
#include <openstudio/model/ScheduleCompact.hpp>
#include <openstudio/model/SetpointManagerMixedAir.hpp>
#include <openstudio/model/SetpointManagerScheduled.hpp>
#include <openstudio/model/SetpointManagerSingleZoneReheat.hpp>
#include <openstudio/model/PlantComponentUserDefined.hpp>
#include <openstudio/model/ZoneHVACBaseboardConvectiveWater.hpp>
#include <openstudio/model/ZoneHVACFourPipeFanCoil.hpp>
#include <openstudio/model/ZoneHVACLowTempRadiantConstFlow.hpp>
#include <openstudio/model/ZoneHVACLowTemperatureRadiantElectric.hpp>
#include <openstudio/model/ZoneHVACLowTempRadiantVarFlow.hpp>
#include <openstudio/model/ZoneHVACPackagedTerminalHeatPump.hpp>
#include <openstudio/model/ZoneHVACPackagedTerminalAirConditioner.hpp>
#include <openstudio/model/ZoneHVACUnitHeater.hpp>
#include <openstudio/model/ZoneHVACWaterToAirHeatPump.hpp>

#include <openstudio/OpenStudio.hxx>

#include <openstudio/osversion/VersionTranslator.hpp>

#include <openstudio/energyplus/ForwardTranslator.hpp>
#include <openstudio/energyplus/ReverseTranslator.hpp>

#include <openstudio/gbxml/ReverseTranslator.hpp>
#include <openstudio/sdd/ReverseTranslator.hpp>
#include "../bimserver/BIMserverConnection.hpp"
#include "../bimserver/ProjectImporter.hpp"

#include <QAbstractButton>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileOpenEvent>
#include <QMessageBox>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include <QProcess>
#include <QTcpServer>
#include <QtConcurrent>
#include <QtGlobal>
#include <QSettings>
#include <QTranslator>

#include <openstudio/OpenStudio.hxx>
#include <openstudio/utilities/idd/IddEnums.hxx>
#include <sstream>
#include <cstdlib>

using namespace openstudio::model;

namespace openstudio {

bool TouchEater::eventFilter(QObject* obj, QEvent* event) {
  if (event->type() == QEvent::TouchBegin) {
    return true;
  } else {
    // standard event processing
    return QObject::eventFilter(obj, event);
  }
}

OpenStudioApp::OpenStudioApp(int& argc, char** argv)
  : OSAppBase(argc, argv, QSharedPointer<MeasureManager>(new MeasureManager(this))), m_measureManagerProcess(nullptr) {
  setOrganizationName("OpenStudioCoalition");
  QCoreApplication::setOrganizationDomain("openstudiocoalition.org");
  setApplicationName("OpenStudioApp");

  auto eater = new TouchEater();
  installEventFilter(eater);

  // Don't use native menu bar, necessary on Ubuntu 16.04
  // TODO: check for adverse side effects on other OSes
  QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);

  readSettings();

  // Need to set the first translator early on
  switchLanguage(m_currLang);

  QFile f(":/library/OpenStudioPolicy.xml");
  if (f.open(QFile::ReadOnly)) {
    const auto data = f.readAll();
    openstudio::model::AccessPolicyStore::Instance().loadFile(std::vector<char>{data.begin(), data.end()});
  }

  QFile data(":/openstudiolib.qss");
  QString style;
  if (data.open(QFile::ReadOnly)) {
    QTextStream styleIn(&data);
    style = styleIn.readAll();
    data.close();
    setStyleSheet(style);
  }

#ifdef Q_OS_DARWIN
  std::stringstream webenginePath;
  webenginePath << QCoreApplication::applicationDirPath().toStdString();
  webenginePath << "/../Frameworks/QtWebEngineCore.framework/Versions/5/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess";
  if (filesystem::exists(filesystem::path(webenginePath.str()))) {
    setenv("QTWEBENGINEPROCESS_PATH", webenginePath.str().c_str(), true);
  }

  setQuitOnLastWindowClosed(false);

  m_startupMenu = std::shared_ptr<StartupMenu>(new StartupMenu());
  connect(m_startupMenu.get(), &StartupMenu::exitClicked, this, &OpenStudioApp::quit, Qt::QueuedConnection);
  connect(m_startupMenu.get(), &StartupMenu::importClicked, this, &OpenStudioApp::importIdf);
  connect(m_startupMenu.get(), &StartupMenu::importgbXMLClicked, this, &OpenStudioApp::importgbXML);
  connect(m_startupMenu.get(), &StartupMenu::importSDDClicked, this, &OpenStudioApp::importSDD);
  connect(m_startupMenu.get(), &StartupMenu::importIFCClicked, this, &OpenStudioApp::importIFC);
  connect(m_startupMenu.get(), &StartupMenu::loadFileClicked, this, &OpenStudioApp::open);
  connect(m_startupMenu.get(), &StartupMenu::newClicked, this, &OpenStudioApp::newModel);
  connect(m_startupMenu.get(), &StartupMenu::helpClicked, this, &OpenStudioApp::showHelp);
  connect(m_startupMenu.get(), &StartupMenu::checkForUpdateClicked, this, &OpenStudioApp::checkForUpdate);
  connect(m_startupMenu.get(), &StartupMenu::aboutClicked, this, &OpenStudioApp::showAbout);
#endif

  waitDialog()->show();

  // We are using the wait dialog to lock out the app so
  // use processEvents to make sure the dialog is up before we
  // proceed to startMeasureManagerProcess
  processEvents();

  // Non blocking
  startMeasureManagerProcess();

  auto waitForMeasureManagerFuture = QtConcurrent::run(&measureManager(), &MeasureManager::waitForStarted, 10000);
  m_waitForMeasureManagerWatcher.setFuture(waitForMeasureManagerFuture);
  connect(&m_waitForMeasureManagerWatcher, &QFutureWatcher<void>::finished, this, &OpenStudioApp::onMeasureManagerAndLibraryReady);

  auto buildCompLibrariesFuture = QtConcurrent::run(this, &OpenStudioApp::buildCompLibraries);
  m_buildCompLibWatcher.setFuture(buildCompLibrariesFuture);
  connect(&m_buildCompLibWatcher, &QFutureWatcher<std::vector<std::string>>::finished, this, &OpenStudioApp::onMeasureManagerAndLibraryReady);
}

OpenStudioApp::~OpenStudioApp() {
  if (m_measureManagerProcess) {
    m_measureManagerProcess->disconnect();
    m_measureManagerProcess->kill();
    m_measureManagerProcess->waitForFinished();
    delete m_measureManagerProcess;
    m_measureManagerProcess = nullptr;
  }
}

void OpenStudioApp::onMeasureManagerAndLibraryReady() {
  if (m_buildCompLibWatcher.isFinished() && m_waitForMeasureManagerWatcher.isFinished()) {
    // m_waitForMeasureManagerWatcher might be finished, but that might be because it timeout'ed
    // So you do want to check the result of the future now that it catches the return type
    // (or call measureManager().isStarted() equivalently)
    if (!m_waitForMeasureManagerWatcher.future().result()) {
      int currentTry = 1;
      while (!measureManager().isStarted()) {
        LOG(Fatal, "Failed to start the Measure Manager on try " << currentTry << ", timeout reached.");
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Timeout"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("Failed to start the Measure Manager. Would you like to retry?"));
        msgBox.setStandardButtons(QMessageBox::Retry | QMessageBox::Close);
        if (msgBox.exec() == QMessageBox::Close) {
          LOG_AND_THROW("Exit after measure manager failed to start in given time.");
          ;
        } else {
          measureManager().waitForStarted(10000);
          ++currentTry;
        }
      }
      LOG(Info, "Recovered from Measure Manager problem, managed to start it on try " << currentTry
                                                                                      << " at: " << toString(measureManager().url().toString()));
    }

    auto failed = m_buildCompLibWatcher.result();
    showFailedLibraryDialog(failed);

    bool openedCommandLine = false;

    QStringList args = QApplication::arguments();
    args.removeFirst();  // application name

    if (args.size() == 1 || args.size() == 2) {

      // look for file path in args 0
      QFileInfo info(args.at(0));  // handles windows links and "\"
      QString fileName = info.absoluteFilePath();

      osversion::VersionTranslator versionTranslator;
      versionTranslator.setAllowNewerVersions(false);

      boost::optional<openstudio::model::Model> model = versionTranslator.loadModel(toPath(fileName));
      if (model) {

        m_osDocument = std::shared_ptr<OSDocument>(new OSDocument(componentLibrary(), resourcesPath(), model, fileName, false, startTabIndex()));

        connectOSDocumentSignals();

        if (args.size() == 2) {
          // check for 'noSavePath'
          if (args.at(1) == QString("noSavePath")) {
            m_osDocument->setSavePath("");
            QTimer::singleShot(0, m_osDocument.get(), &OSDocument::markAsModified);
          } else {
            LOG_FREE(Warn, "OpenStudio", "Incorrect second argument '" << toString(args.at(1)) << "'");
          }
        }

        openedCommandLine = true;

        versionUpdateMessageBox(versionTranslator, true, fileName, openstudio::toPath(m_osDocument->modelTempDir()));

      } else {
        LOG_FREE(Warn, "OpenStudio", "Could not open file at " << toString(fileName));

        versionUpdateMessageBox(versionTranslator, false, fileName, openstudio::path());
      }

    } else if (args.size() > 2) {
      LOG_FREE(Warn, "OpenStudio", "Incorrect number of arguments " << args.size());
    }

    if (!openedCommandLine) {
      newFromEmptyTemplateSlot();
    }
  }
}

bool OpenStudioApp::openFile(const QString& fileName, bool restoreTabs) {
  // Note: already checked for in open() before calling this
  if (fileName.length() > 0) {
    osversion::VersionTranslator versionTranslator;
    versionTranslator.setAllowNewerVersions(false);

    boost::optional<openstudio::model::Model> temp = versionTranslator.loadModel(toPath(fileName));
    // If VT worked
    if (temp) {
      model::Model model = temp.get();

      bool wasQuitOnLastWindowClosed = this->quitOnLastWindowClosed();
      this->setQuitOnLastWindowClosed(false);

      int startTabIndex = this->startTabIndex();
      int startSubTabIndex = 0;
      if (m_osDocument) {

        if (restoreTabs) {
          startTabIndex = m_osDocument->verticalTabIndex();
          startSubTabIndex = m_osDocument->subTabIndex();
        }

        if (!closeDocument()) {
          this->setQuitOnLastWindowClosed(wasQuitOnLastWindowClosed);
          return false;
        }
        processEvents();
      }

      // TODO: waitDialog isn't showed until VT has actually happened and worked?
      // I tried to show it visible in the begining of the method, but it isn't displayed correctly:
      // transparent + hidden by Filedialog which isn't closed yet.
      waitDialog()->setVisible(true);
      processEvents();

      m_osDocument =
        std::shared_ptr<OSDocument>(new OSDocument(componentLibrary(), resourcesPath(), model, fileName, false, startTabIndex, startSubTabIndex));

      connectOSDocumentSignals();

      waitDialog()->setVisible(false);

      versionUpdateMessageBox(versionTranslator, true, fileName, openstudio::toPath(m_osDocument->modelTempDir()));

      this->setQuitOnLastWindowClosed(wasQuitOnLastWindowClosed);

      return true;
    } else {
      LOG_FREE(Warn, "OpenStudio", "Could not open file at " << toString(fileName));

      versionUpdateMessageBox(versionTranslator, false, fileName, openstudio::path());
    }
  }
  return false;
}

std::vector<std::string> OpenStudioApp::buildCompLibraries() {
  std::vector<std::string> failed;

  // This is unused
  //QWidget * parent = nullptr;
  //if( this->currentDocument() ){
  //parent = this->currentDocument()->mainWindow();
  //}

  // Get the first Qlabel waitDialog (0 = stretch, 1 = "Loading model", 2 = "This may take a minute...", 3=hidden lable,   = stretch)
  waitDialog()->m_firstLine->setText(tr("Loading Library Files"));
  waitDialog()->m_secondLine->setText(tr("(Manage library files in Preferences->Change default libraries)"));

  // DLM: this was causing a crash because waitDialog is created on the main thread but this is called on the wait thread.
  // Because this is just the wait dialog let's just keep the line always visible.
  // Make it visible
  //waitDialog()->m_thirdLine->setVisible(true);
  //waitDialog()->m_fourthLine->setVisible(true);

  m_compLibrary = model::Model();

  std::string thisVersion = openStudioVersion();

  for (auto path : libraryPaths()) {
    try {
      if (exists(path)) {
        boost::optional<VersionString> version = openstudio::IdfFile::loadVersionOnly(path);
        if (version) {
          waitDialog()->m_thirdLine->setText(tr("Translation From version ") + QString::fromStdString(version->str()) + tr(" to ")
                                             + QString::fromStdString(thisVersion) + ": ");
        } else {
          waitDialog()->m_thirdLine->setText(tr("Unknown starting version"));
        }

        waitDialog()->m_fourthLine->setText(toQString(path));

        osversion::VersionTranslator versionTranslator;
        versionTranslator.setAllowNewerVersions(false);
        boost::optional<Model> temp = versionTranslator.loadModel(path);
        if (temp) {
          m_compLibrary.insertObjects(temp->objects());
        } else {
          LOG_FREE(Error, "OpenStudioApp", "Failed to load library");
          failed.push_back(path.string());
        }
      } else {
        failed.push_back(path.string());
      }
    } catch (...) {
      failed.push_back(path.string());
    }
  }

  // Reset all labels
  waitDialog()->resetLabels();

  return failed;
}

OpenStudioApp* OpenStudioApp::instance() {
  return qobject_cast<OpenStudioApp*>(QApplication::instance());
}

openstudio::model::Model OpenStudioApp::componentLibrary() const {
  return m_compLibrary;
}

openstudio::model::Model OpenStudioApp::hvacComponentLibrary() const {
  return m_hvacCompLibrary;
}

void OpenStudioApp::quit() {
  if (m_osDocument) {
    if (closeDocument()) {
      QApplication::quit();
    }
  } else {
    QApplication::quit();
  }
}

void OpenStudioApp::newFromEmptyTemplateSlot() {
  newFromTemplateSlot(NEWFROMTEMPLATE_EMPTY);
}

void OpenStudioApp::newFromTemplateSlot(NewFromTemplateEnum newFromTemplateEnum) {
  m_osDocument = std::shared_ptr<OSDocument>(new OSDocument(componentLibrary(), resourcesPath(), boost::none, QString(), false, startTabIndex()));

  connectOSDocumentSignals();

  waitDialog()->hide();
}

std::shared_ptr<OSDocument> OpenStudioApp::currentDocument() const {
  return m_osDocument;
}

void OpenStudioApp::importIdf() {
  QWidget* parent = nullptr;

  if (this->currentDocument()) {
    parent = this->currentDocument()->mainWindow();
  }

  QString fileName = QFileDialog::getOpenFileName(parent, tr("Import Idf"), lastPath(), tr("(*.idf)"));

  if (!(fileName == "")) {
    setLastPath(QFileInfo(fileName).path());

    boost::optional<IdfFile> idfFile;

    openstudio::path path = toPath(fileName);
    idfFile = openstudio::IdfFile::load(path, IddFileType::EnergyPlus);

    if (idfFile) {

      IdfFile _idfFile = idfFile.get();

      if (!_idfFile.isValid(StrictnessLevel::Draft)) {

        // Something is wrong, try to be informative

        // Retrieve IDF File Version
        boost::optional<VersionString> idfFileVersion = openstudio::IdfFile::loadVersionOnly(path);

        // Retrieve current version of E+ used by OS
        VersionString currentVersion(energyPlusVersion());

        QString informativeText;

        if (idfFileVersion.has_value()) {

          informativeText = QString("The IDF is at version '") + toQString(idfFileVersion->str());

          if (idfFileVersion.get() < currentVersion) {
            informativeText.append(tr("' while OpenStudio uses a <strong>newer</strong> EnergyPlus '") + toQString(currentVersion.str())
                                   + tr("'. Consider using the EnergyPlus Auxiliary program IDFVersionUpdater to update your IDF file."));
          } else if (idfFileVersion.get() > currentVersion) {
            informativeText.append(tr("' while OpenStudio uses an <strong>older</strong> EnergyPlus '") + toQString(currentVersion.str()) + tr("'."));
          } else {
            informativeText.append(tr("' which is the <strong>same</strong> version of EnergyPlus that OpenStudio uses (")
                                   + toQString(currentVersion.str()) + QString(")."));
          }
        } else {
          informativeText = tr("<strong>The IDF does not have a VersionObject</strong>. Check that it is of correct version (")
                            + toQString(currentVersion.str()) + tr(") and that all fields are valid against Energy+.idd. ");
        }

        informativeText.append(tr("<br/><br/>The ValidityReport follows."));

        QString text = tr("<strong>File is not valid to draft strictness</strong>. Check that all fields are valid against Energy+.idd.");

        //QMessageBox messageBox(parent);
        //messageBox.setText(text);

        // Customize with title and critical icon
        QMessageBox messageBox(QMessageBox::Critical, tr(" IDF Import Failed"), text, QMessageBox::NoButton, parent,
                               Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

        messageBox.setInformativeText(informativeText);

        std::stringstream ss;
        ss << _idfFile.validityReport(StrictnessLevel::Draft);
        messageBox.setDetailedText(toQString(ss.str()));
        messageBox.exec();

      } else {

        Workspace workspace(_idfFile);

        energyplus::ReverseTranslator trans;
        model::Model model = trans.translateWorkspace(workspace);

        bool wasQuitOnLastWindowClosed = this->quitOnLastWindowClosed();
        this->setQuitOnLastWindowClosed(false);

        if (m_osDocument) {
          if (!closeDocument()) {
            this->setQuitOnLastWindowClosed(wasQuitOnLastWindowClosed);
            return;
          }
          processEvents();
        }

        m_osDocument = std::shared_ptr<OSDocument>(new OSDocument(componentLibrary(), resourcesPath(), model, QString(), false, startTabIndex()));
        m_osDocument->markAsModified();
        // ETH: parent should change now ...
        //parent = m_osDocument->mainWindow();

        connectOSDocumentSignals();

        QString log;

        // DetailedText of QMessageBox is always interpreted a raw text, so not HTML formatting.
        /*
 *        if (!trans.errors().empty()) {
 *          log.append("<h3>Errors:</h3>");
 *          log.append("<ul>");
 *          for( const auto & message : trans.errors() ) {
 *            log.append("<li>" + QString::fromStdString(message.logMessage()) + "</li>");
 *          }
 *          log.append("</ul>");
 *        }
 *
 *        if (!trans.warnings().empty()) {
 *          log.append("<h3>Warnings:</h3>");
 *          log.append("<ul>");
 *          for( const auto & message : trans.warnings() ) {
 *            log.append("<li>" + QString::fromStdString(message.logMessage()) + "</li>");
 *          }
 *          log.append("</ul>");
 *        }
 *
 *        if (!trans.untranslatedIdfObjects().empty()) {
 *          log.append("<h3>The following idf objects were not imported:</h3>");
 *          log.append("<ul>");
 *
 *          for( const auto & idfObject : trans.untranslatedIdfObjects() ) {
 *            std::string message;
 *            if( auto name = idfObject.name() ) {
 *              message = idfObject.iddObject().name() + " named " + name.get();
 *            } else {
 *              message = "Unnamed " + idfObject.iddObject().name();
 *            }
 *            log.append("<li>" + QString::fromStdString(message) + "</li>");
 *          }
 *          log.append("</ul>");
 *        }
 */

        if (!trans.errors().empty()) {
          log.append(tr("=============== Errors ===============\n\n"));
          for (const auto& message : trans.errors()) {
            log.append(" * " + QString::fromStdString(message.logMessage()) + "\n");
          }
          log.append("\n\n");
        }

        if (!trans.warnings().empty()) {
          log.append(tr("============== Warnings ==============\n\n"));
          for (const auto& message : trans.warnings()) {
            log.append(" * " + QString::fromStdString(message.logMessage()) + "\n");
          }
          log.append("\n\n");
        }

        if (!trans.untranslatedIdfObjects().empty()) {
          log.append(tr("==== The following idf objects were not imported ====\n\n"));

          for (const auto& idfObject : trans.untranslatedIdfObjects()) {
            QString message;
            if (auto name = idfObject.name()) {
              message = toQString(idfObject.iddObject().name()) + tr(" named ") + toQString(name.get());
            } else {
              message = tr("Unnamed ") + toQString(idfObject.iddObject().name());
            }
            log.append(" * " + message + "\n");
          }
        }

        QString text = tr("<strong>Some portions of the IDF file were not imported.</strong>");

        // QMessageBox messageBox; // (parent); ETH: ... but is hidden, so don't actually use
        // messageBox.setText(text);

        // Passing parent = nullptr on purpose here
        QMessageBox messageBox(QMessageBox::Warning, tr("IDF Import"), text, QMessageBox::NoButton, nullptr,
                               Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        messageBox.setInformativeText(
          tr("Only geometry, constructions, loads, thermal zones, and schedules are supported by the OpenStudio IDF import feature."));
        messageBox.setDetailedText(log);

        messageBox.exec();

        this->setQuitOnLastWindowClosed(wasQuitOnLastWindowClosed);
      }
    }
  }
}

void OpenStudioApp::importgbXML() {
  import(GBXML);
}

void OpenStudioApp::importSDD() {
  import(SDD);
}

void OpenStudioApp::importIFC() {
  QWidget* parent = nullptr;
  if (this->currentDocument()) {
    parent = this->currentDocument()->mainWindow();
  }

  auto projectImportation = new bimserver::ProjectImporter(parent);
  boost::optional<model::Model> model = projectImportation->run();
  projectImportation->close();

  if (model) {
    bool wasQuitOnLastWindowClosed = this->quitOnLastWindowClosed();
    this->setQuitOnLastWindowClosed(false);

    if (m_osDocument) {
      if (!closeDocument()) {
        this->setQuitOnLastWindowClosed(wasQuitOnLastWindowClosed);
        return;
      }
      processEvents();
    }

    m_osDocument = std::shared_ptr<OSDocument>(new OSDocument(componentLibrary(), resourcesPath(), *model, QString(), false, startTabIndex()));

    m_osDocument->markAsModified();

    connectOSDocumentSignals();

    this->setQuitOnLastWindowClosed(wasQuitOnLastWindowClosed);
  }
}

void OpenStudioApp::import(OpenStudioApp::fileType type) {
  QWidget* parent = nullptr;

  std::vector<LogMessage> translatorErrors, translatorWarnings;

  QString fileExtension;
  if (type == SDD) {
    fileExtension = "SDD";
  } else if (type == GBXML) {
    fileExtension = "gbXML";
  } else {
    // should never get here
    OS_ASSERT(false);
  }
  QString text = tr("Import ");
  text.append(fileExtension);

  if (this->currentDocument()) {
    parent = this->currentDocument()->mainWindow();
  }

  QString fileName = QFileDialog::getOpenFileName(parent, tr(text.toStdString().c_str()), lastPath(), tr("(*.xml)"));

  if (!(fileName == "")) {
    setLastPath(QFileInfo(fileName).path());

    boost::optional<model::Model> model;

    if (type == SDD) {
      sdd::ReverseTranslator trans;
      model = trans.loadModel(toPath(fileName));
      translatorErrors = trans.errors();
      translatorWarnings = trans.warnings();
    } else if (type == GBXML) {
      gbxml::ReverseTranslator trans;
      model = trans.loadModel(toPath(fileName));
      translatorErrors = trans.errors();
      translatorWarnings = trans.warnings();
    }

    if (model) {
      bool wasQuitOnLastWindowClosed = this->quitOnLastWindowClosed();
      this->setQuitOnLastWindowClosed(false);

      if (m_osDocument) {
        if (!closeDocument()) {
          this->setQuitOnLastWindowClosed(wasQuitOnLastWindowClosed);
          return;
        }
        processEvents();
      }

      m_osDocument = std::shared_ptr<OSDocument>(new OSDocument(componentLibrary(), resourcesPath(), *model, QString(), false, startTabIndex()));
      m_osDocument->markAsModified();
      // ETH: parent should change now ...
      //parent = m_osDocument->mainWindow();

      connectOSDocumentSignals();

      bool errorsOrWarnings = false;

      QString log;

      for (const auto& error : translatorErrors) {
        errorsOrWarnings = true;

        log.append(QString::fromStdString(error.logMessage()));
        log.append("\n");
        log.append("\n");
      }

      for (const auto& warning : translatorWarnings) {
        errorsOrWarnings = true;

        log.append(QString::fromStdString(warning.logMessage()));
        log.append("\n");
        log.append("\n");
      }

      if (errorsOrWarnings) {
        QMessageBox messageBox;  // (parent); ETH: ... but is hidden, so don't actually use
        messageBox.setText(tr("Errors or warnings occurred on import of ") + fileExtension);
        messageBox.setDetailedText(log);
        messageBox.exec();
      }

      this->setQuitOnLastWindowClosed(wasQuitOnLastWindowClosed);

    } else {

      QMessageBox messageBox;  // (parent); ETH: ... but is hidden, so don't actually use
      messageBox.setText(tr("Could not import SDD file."));

      QString log(tr("Could not import ") + fileExtension + tr(" file at ") + fileName + ".\n\n");

      for (const auto& error : translatorErrors) {
        log.append(QString::fromStdString(error.logMessage()));
        log.append("\n");
        log.append("\n");
      }

      for (const auto& warning : translatorWarnings) {
        log.append(QString::fromStdString(warning.logMessage()));
        log.append("\n");
        log.append("\n");
      }

      messageBox.setDetailedText(log);
      messageBox.exec();
    }
  }
}

bool OpenStudioApp::openFromDrag(QString path) {
  return openFile(path);
}

bool OpenStudioApp::closeDocument() {
  if (m_osDocument->modified()) {
    QWidget* parent = m_osDocument->mainWindow();

    auto messageBox = new QMessageBox(parent);

    messageBox->setWindowTitle(tr("Save Changes?"));
    messageBox->setText(tr("The document has been modified."));
    messageBox->setInformativeText(tr("Do you want to save your changes?"));

    messageBox->setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    messageBox->setDefaultButton(QMessageBox::Save);

    messageBox->button(QMessageBox::Save)->setShortcut(QKeySequence(Qt::Key_S));
    messageBox->button(QMessageBox::Discard)->setShortcut(QKeySequence(Qt::Key_D));

    messageBox->setIcon(QMessageBox::Question);

    int ret = messageBox->exec();

    delete messageBox;

    switch (ret) {
      case QMessageBox::Save:

        // Save was clicked
        m_osDocument->save();
        m_osDocument->mainWindow()->hide();
        m_osDocument = std::shared_ptr<OSDocument>();
        return true;

      case QMessageBox::Discard:

        // Don't Save was clicked
        m_osDocument->mainWindow()->hide();
        m_osDocument = std::shared_ptr<OSDocument>();
        return true;

      case QMessageBox::Cancel:

        // Cancel was clicked
        m_osDocument->mainWindow()->activateWindow();
        return false;

      default:

        // should never be reached
        return false;
    }

  } else {
    m_osDocument->mainWindow()->hide();
    m_osDocument = std::shared_ptr<OSDocument>();

    return true;
  }
}

void OpenStudioApp::onCloseClicked() {
  if (closeDocument()) {
#ifndef Q_OS_DARWIN
    QApplication::quit();
#endif
  }
}

void OpenStudioApp::open() {

  QWidget* parent = nullptr;

  if (this->currentDocument()) {
    parent = this->currentDocument()->mainWindow();
  }

  QString fileName = QFileDialog::getOpenFileName(parent, tr("Open"), lastPath(), tr("(*.osm)"));

  if (!fileName.length()) return;

  setLastPath(QFileInfo(fileName).path());

  openFile(fileName);

  // Reset the labels
  waitDialog()->resetLabels();
}

//void OpenStudioApp::loadLibrary()
//{
//  if( this->currentDocument() )
//  {
//    QWidget * parent = this->currentDocument()->mainWindow();
//
//
//    QString fileName = QFileDialog::getOpenFileName( parent,
//                                                    tr("Select Library"),
//                                                    toQString(resourcesPath()),
//                                                    tr("(*.osm)") );
//
//    if( ! (fileName == "") )
//    {
//      osversion::VersionTranslator versionTranslator;
//      versionTranslator.setAllowNewerVersions(false);
//
//      boost::optional<openstudio::model::Model> model = versionTranslator.loadModel(toPath(fileName));
//      if( model ) {
//        this->currentDocument()->setComponentLibrary(*model);
//        versionUpdateMessageBox(versionTranslator, true, fileName, openstudio::path());
//      }else{
//        LOG_FREE(Warn, "OpenStudio", "Could not open file at " << toString(fileName));
//        versionUpdateMessageBox(versionTranslator, false, fileName, openstudio::path());
//      }
//    }
//  }
//}

void OpenStudioApp::newModel() {
  bool wasQuitOnLastWindowClosed = this->quitOnLastWindowClosed();
  this->setQuitOnLastWindowClosed(false);

  if (m_osDocument) {
    if (!closeDocument()) {
      this->setQuitOnLastWindowClosed(wasQuitOnLastWindowClosed);
      return;
    }
    processEvents();
  }

  //*************************************************************************************
  //
  ///! TODO StartView has been deprecated until the template wizard functions
  //
  //if (!openedCommandLine){
  //  m_startupView->show();
  //}
  //
  newFromTemplateSlot(NEWFROMTEMPLATE_EMPTY);  // remove when above code uncommented
  //
  //*************************************************************************************

  this->setQuitOnLastWindowClosed(wasQuitOnLastWindowClosed);
}

void OpenStudioApp::showHelp() {
  QDesktopServices::openUrl(QUrl("http:/openstudiocoalition.org/reference/openstudio_application_interface/"));
}

void OpenStudioApp::checkForUpdate() {
  QWidget* parent = nullptr;

  if (currentDocument()) {
    parent = currentDocument()->mainWindow();
  }

  modeleditor::GithubReleases releases("openstudiocoalition", "OpenStudioApplication");
  releases.waitForFinished();

  QString text;
  QMessageBox::StandardButtons buttons;
  bool openURL = false;
  if (releases.newReleaseAvailable()) {
    text = QString(tr("A new version is available at <a href=\"")) + toQString(releases.releasesUrl()) + QString("\">")
           + toQString(releases.releasesUrl()) + QString("</a>");
    openURL = true;
    buttons = QMessageBox::Open | QMessageBox::Ignore;
  } else {
    text = tr("Currently using the most recent version");
    buttons = QMessageBox::Ok;
  }

  QMessageBox about(parent);
  about.setStandardButtons(buttons);
  about.setTextFormat(Qt::RichText);
  about.setText(text);
  about.setWindowTitle(tr("Check for Updates"));
  about.exec();

  if (openURL && (about.result() == QMessageBox::Open)) {
    QDesktopServices::openUrl(QUrl(toQString(releases.releasesUrl())));
  }
}

void OpenStudioApp::showAbout() {
  QWidget* parent = nullptr;

  if (currentDocument()) {
    parent = currentDocument()->mainWindow();
  }
  QString details = tr("Measure Manager Server: ") + measureManager().url().toString() + "\n";
  details += tr("Chrome Debugger: http://localhost:") + qgetenv("QTWEBENGINE_REMOTE_DEBUGGING") + "\n";
  details += tr("Temp Directory: ") + currentDocument()->modelTempDir();
  QMessageBox about(parent);
  about.setText(OPENSTUDIOAPP_ABOUTBOX);
  about.setDetailedText(details);
  about.setStyleSheet("qproperty-alignment: AlignCenter;");
  about.setWindowTitle(tr("About ") + applicationName());
  about.exec();
}

void OpenStudioApp::reloadFile(const QString& osmPath, bool modified, bool saveCurrentTabs) {
  OS_ASSERT(m_osDocument);

  QFileInfo info(osmPath);  // handles windows links and "\"
  QString fileName = info.absoluteFilePath();
  osversion::VersionTranslator versionTranslator;
  openstudio::path path = toPath(fileName);
  boost::optional<openstudio::model::Model> model = versionTranslator.loadModel(path);
  if (model) {

    bool wasQuitOnLastWindowClosed = this->quitOnLastWindowClosed();
    this->setQuitOnLastWindowClosed(false);

    // DLM: load OSW from the existing temp dir
    openstudio::path workflowPath = openstudio::toPath(m_osDocument->modelTempDir()) / toPath("resources") / toPath("workflow.osw");
    boost::optional<WorkflowJSON> workflowJSON = WorkflowJSON::load(workflowPath);
    if (workflowJSON) {
      model->setWorkflowJSON(*workflowJSON);
    }

    m_osDocument->setModel(*model, modified, saveCurrentTabs);

    versionUpdateMessageBox(versionTranslator, true, fileName, openstudio::toPath(m_osDocument->modelTempDir()));

    this->setQuitOnLastWindowClosed(wasQuitOnLastWindowClosed);

  } else {
    QMessageBox::warning(m_osDocument->mainWindow(), tr("Failed to load model"), tr("Failed to load model"));
  }

  processEvents();
}

openstudio::path OpenStudioApp::resourcesPath() const {
  openstudio::path p;
  if (isOpenStudioApplicationRunningFromBuildDirectory()) {
    p = boost::filesystem::canonical(openstudio::toPath("src/openstudio_app/Resources"), getOpenStudioApplicationSourceDirectory());
  } else {
    p = boost::filesystem::canonical(openstudio::toPath("../Resources"), getOpenStudioApplicationDirectory());
  }

  return p;
}

openstudio::path OpenStudioApp::openstudioCLIPath() const {
  /*
   *auto dir = applicationDirPath();
   *QString ext;
   *#ifdef _WIN32
   *  ext = QFileInfo(applicationFilePath()).suffix();
   *#endif
   *if (ext.isEmpty())
   *{
   *  return openstudio::toPath(dir + "/openstudio");
   *}
   *return openstudio::toPath(dir + "/openstudio." + ext);
   */

  return openstudio::getOpenStudioCoreCLI();
}

bool OpenStudioApp::notify(QObject* receiver, QEvent* event) {
  if (event->type() == QEvent::FileOpen) {
    return openFile(static_cast<QFileOpenEvent*>(event)->file());
  }
  // Note: the original call below bypasses OSAppBase, OpenStudioApp's base class
  //return QApplication::notify(receiver, event);
  return OSAppBase::notify(receiver, event);
}

bool OpenStudioApp::event(QEvent* e) {
  return OSAppBase::event(e);
}

void OpenStudioApp::childEvent(QChildEvent* e) {
  OSAppBase::childEvent(e);
}

void OpenStudioApp::versionUpdateMessageBox(const osversion::VersionTranslator& translator, bool successful, const QString& fileName,
                                            const openstudio::path& tempModelDir) {
  QMessageBox messageBox;

  QString log;

  std::vector<LogMessage> messages = translator.errors();

  for (const auto& message : messages) {
    log.append(QString::fromStdString(message.logMessage()));
    log.append("\n");
    log.append("\n");
  }

  messages = translator.warnings();

  for (const auto& message : messages) {
    log.append(QString::fromStdString(message.logMessage()));
    log.append("\n");
    log.append("\n");
  }

  if (log.length() > 0) {
    messageBox.setDetailedText(log);
  }

  if (successful) {

    bool removedScriptDirs = false;
    if (!tempModelDir.empty()) {
      std::vector<openstudio::path> scriptfolders;

      scriptfolders.push_back(tempModelDir / openstudio::toPath("resources/scripts/post_energyplus"));
      scriptfolders.push_back(tempModelDir / openstudio::toPath("resources/scripts/idf_resources"));
      scriptfolders.push_back(tempModelDir / openstudio::toPath("resources/scripts/idf_scripts"));
      scriptfolders.push_back(tempModelDir / openstudio::toPath("resources/scripts/model_resources"));
      scriptfolders.push_back(tempModelDir / openstudio::toPath("resources/scripts/model_scripts"));

      for (std::vector<openstudio::path>::const_iterator itr = scriptfolders.begin(); itr != scriptfolders.end(); ++itr) {
        if (openstudio::filesystem::exists(*itr)) {
          removedScriptDirs = true;
          openstudio::filesystem::remove_all(*itr);
        }
      }
    }

    VersionString originalVersion(translator.originalVersion()), currentVersion(openStudioVersion());
    bool versionChanged = originalVersion != currentVersion;

    if (versionChanged || removedScriptDirs) {
      QTimer::singleShot(0, m_osDocument.get(), &OSDocument::markAsModified);

      QString message;
      if (versionChanged) {
        if (originalVersion > currentVersion) {
          message = tr("Opening future version ") + toQString(originalVersion.str()) + tr(" using ") + toQString(currentVersion.str()) + ".";
        } else {
          message = tr("Model updated from ") + toQString(originalVersion.str()) + tr(" to ") + toQString(currentVersion.str()) + ".";
        }
      }

      if (removedScriptDirs) {
        if (!message.isEmpty()) {
          message += "\n\n";
        }

        message += tr("Existing Ruby scripts have been removed.\nRuby scripts are no longer supported and have been replaced by measures.");
      }

      messageBox.setText(message);
      messageBox.exec();
    }
  } else {
    messageBox.setText(tr("Failed to open file at ") + fileName + QString("."));
    messageBox.exec();
  }
}

void OpenStudioApp::readSettings() {
  QString organizationName = QCoreApplication::organizationName();
  QString applicationName = QCoreApplication::applicationName();
  QSettings settings(organizationName, applicationName);
  setLastPath(settings.value("lastPath", QDir::homePath()).toString());
  setDviewPath(openstudio::toPath(settings.value("dviewPath", "").toString()));
  m_currLang = settings.value("language", "en").toString();
  LOG_FREE(Debug, "OpenStudioApp", "\n\n\nm_currLang=[" << m_currLang.toStdString() << "]\n\n\n");
  if (m_currLang.isEmpty()) {
    m_currLang = "en";
  }
}

void OpenStudioApp::writeSettings() {
  QString organizationName = QCoreApplication::organizationName();
  QString applicationName = QCoreApplication::applicationName();
  QSettings settings(organizationName, applicationName);
  if (!settings.isWritable()) {
    QMessageBox::warning(nullptr, tr("Settings file not writable"),
                         tr("Your settings file '") + settings.fileName() + tr("' is not writable. Adjust the file permissions"));
  }
  settings.setValue("lastPath", lastPath());
}

QString OpenStudioApp::lastPath() const {
  return QDir().exists(m_lastPath) ? m_lastPath : QDir::homePath();
}

void OpenStudioApp::setLastPath(const QString& t_lastPath) {
  m_lastPath = t_lastPath;
  writeSettings();
}

void OpenStudioApp::revertToSaved() {
  QString fileName = this->currentDocument()->mainWindow()->windowFilePath();

  QFile testFile(fileName);
  if (!testFile.exists()) {
    // Tell the user the file has never been saved, and ask them if they want to create a new file
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(mainWidget(), tr("Revert to Saved"), tr("This model has never been saved.\nDo you want to create a new model?"),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply == QMessageBox::Yes) {
      // JM: copied DLM's hack below so we do not trigger prompt to save in call to closeDocument during newModel()
      // this->currentDocument()->markAsUnmodified();

      newModel();
    }

  } else {
    // Ask for confirmation
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(mainWidget(), tr("Revert to Saved"), tr("Are you sure you want to revert to the last saved version?"),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply == QMessageBox::Yes) {
      // DLM: quick hack so we do not trigger prompt to save in call to closeDocument during openFile
      this->currentDocument()->markAsUnmodified();

      openFile(fileName, true);
    }
  }
}

void OpenStudioApp::connectOSDocumentSignals() {
  OS_ASSERT(m_osDocument);
  connect(m_osDocument.get(), &OSDocument::closeClicked, this, &OpenStudioApp::onCloseClicked, Qt::QueuedConnection);
  connect(m_osDocument.get(), &OSDocument::exitClicked, this, &OpenStudioApp::quit, Qt::QueuedConnection);
  connect(m_osDocument.get(), &OSDocument::importClicked, this, &OpenStudioApp::importIdf);
  connect(m_osDocument.get(), &OSDocument::importgbXMLClicked, this, &OpenStudioApp::importgbXML);
  connect(m_osDocument.get(), &OSDocument::importSDDClicked, this, &OpenStudioApp::importSDD);
  connect(m_osDocument.get(), &OSDocument::importIFCClicked, this, &OpenStudioApp::importIFC);
  connect(m_osDocument.get(), &OSDocument::loadFileClicked, this, &OpenStudioApp::open);
  connect(m_osDocument.get(), &OSDocument::osmDropped, this, &OpenStudioApp::openFromDrag);
  connect(m_osDocument.get(), &OSDocument::changeDefaultLibrariesClicked, this, &OpenStudioApp::changeDefaultLibraries);
  connect(m_osDocument.get(), &OSDocument::configureExternalToolsClicked, this, &OpenStudioApp::configureExternalTools);
  connect(m_osDocument.get(), &OSDocument::changeLanguageClicked, this, &OpenStudioApp::changeLanguage);
  connect(m_osDocument.get(), &OSDocument::loadLibraryClicked, this, &OpenStudioApp::loadLibrary);
  connect(m_osDocument.get(), &OSDocument::newClicked, this, &OpenStudioApp::newModel);
  connect(m_osDocument.get(), &OSDocument::helpClicked, this, &OpenStudioApp::showHelp);
  connect(m_osDocument.get(), &OSDocument::checkForUpdateClicked, this, &OpenStudioApp::checkForUpdate);
  connect(m_osDocument.get(), &OSDocument::aboutClicked, this, &OpenStudioApp::showAbout);
}

void OpenStudioApp::measureManagerProcessStateChanged(QProcess::ProcessState newState) {}

void OpenStudioApp::measureManagerProcessFinished() {
  // any exit of the cli is an error
  // DLM: I can't get this to fire when I terminate the process in taskmanager
  OS_ASSERT(m_measureManagerProcess);

  // the cli crashed
  QByteArray stdErr = m_measureManagerProcess->readAllStandardError();
  QByteArray stdOut = m_measureManagerProcess->readAllStandardOutput();

  QString message = tr("Measure Manager has crashed, attempting to restart\n\n");
  message += stdErr;
  message += stdOut;

  QMessageBox::warning(nullptr, tr("Measure Manager has crashed"), message);

  startMeasureManagerProcess();
}

void OpenStudioApp::startMeasureManagerProcess() {
  if (m_measureManagerProcess) {
    // will terminate the existing process, blocking call
    delete m_measureManagerProcess;
  }

  // find available port
  QTcpServer tcpServer;
  tcpServer.listen(QHostAddress::LocalHost);
  quint16 port = tcpServer.serverPort();
  tcpServer.close();

  QString portString = QString::number(port);
  QString urlString = "http://127.0.0.1:" + portString;
  QUrl url(urlString);
  measureManager().setUrl(url);

  m_measureManagerProcess = new QProcess(this);

  bool test;
  // finished is an overloaded signal so have to be clear about which version to use
  test = connect(m_measureManagerProcess, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
                 &OpenStudioApp::measureManagerProcessFinished);
  OS_ASSERT(test);
  test = connect(m_measureManagerProcess, &QProcess::errorOccurred, this, &OpenStudioApp::measureManagerProcessFinished);
  OS_ASSERT(test);
  test = connect(m_measureManagerProcess, &QProcess::stateChanged, this, &OpenStudioApp::measureManagerProcessStateChanged);
  OS_ASSERT(test);

  QString program = toQString(openstudioCLIPath());
  QStringList arguments;
  arguments << "measure";
  arguments << "-s";
  arguments << portString;

  LOG(Debug, "Starting measure manager server at " << url.toString().toStdString());
  LOG(Debug, "Command: " << toString(openstudioCLIPath()) << " measure -s " << toString(portString));

  m_measureManagerProcess->start(program, arguments);
}

void OpenStudioApp::writeLibraryPaths(std::vector<openstudio::path> paths) {

  auto defaultPaths = defaultLibraryPaths();

  QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

  if (!settings.isWritable()) {
    QMessageBox::warning(nullptr, tr("Settings file not writable"),
                         tr("Your settings file '") + settings.fileName() + tr("' is not writable. Adjust the file permissions"));
  }

  if (paths == defaultPaths) {
    settings.remove("library");
  } else {
    // Write the paths
    settings.remove("library");
    settings.beginWriteArray("library");
    int i = 0;
    auto resPath = resourcesPath();
    std::string s_resPath = toString(resPath);

    for (const auto path : paths) {
      settings.setArrayIndex(i);

      // If this is one of the defaultPaths
      // if (std::find(defaultPaths.begin(),defaultPaths.end(),path) == defaultPaths.end())
      // If path is located in the 'Resources' folder

      openstudio::path::const_iterator begin1 = resPath.begin();
      openstudio::path::const_iterator end1 = resPath.end();

      openstudio::path::const_iterator begin2 = path.begin();
      openstudio::path::const_iterator end2 = path.end();

      bool is_resource = false;

      if (std::distance(begin1, end1) > std::distance(begin2, end2)) {
        is_resource = false;  // the run dir has fewer elements than the build dir - cannot be running from builddir
      } else {
        // if the rundir begins with the builddir, we know it's running from the builddir
        is_resource = std::equal(begin1, end1, begin2);
      }

      if (is_resource) {
        // Only in boost < 1.6... : boost::filesystem::relative
        std::string s_path = toString(path);

        // Every path here is an absolute canonical path, so we can just slice the path up to the
        // 'Resources' part, +1 to strip also the delimiter
        // TODO: there is probably a more reliable way to do this, it works on linux but unsure on Windows
        s_path = s_path.substr(s_resPath.length() + 1);
        openstudio::path rel_path = openstudio::path(s_path);
        // std::cout << "For '" << path << "', computed relative: " << rel_path << "\n";
        settings.setValue("path", QString::fromStdString(rel_path.string()));

      } else {
        settings.setValue("path", QString::fromStdString(path.string()));
      }

      settings.setValue("is_resource", is_resource);

      ++i;
    }
    settings.endArray();
  }
}

int OpenStudioApp::startTabIndex() const {
  int result = OSDocument::VerticalTabID::SITE;
  if (qEnvironmentVariableIsSet("OPENSTUDIO_APPLICATION_START_TAB_INDEX")) {
    LOG(Debug, "OPENSTUDIO_APPLICATION_START_TAB_INDEX is set");
    bool ok;
    int test = qEnvironmentVariableIntValue("OPENSTUDIO_APPLICATION_START_TAB_INDEX", &ok);
    if (ok) {
      if ((test >= OSDocument::VerticalTabID::SITE) && (test <= OSDocument::VerticalTabID::RESULTS_SUMMARY)) {
        result = test;
        LOG(Debug, "OPENSTUDIO_APPLICATION_START_TAB_INDEX is " << result);
      }
    }
  }

  return result;
}

void OpenStudioApp::changeLanguage(const QString& rLanguage) {
  qDebug() << "Trying to change language from '" << m_currLang << "' to '" << rLanguage << "'.";
  if (m_currLang != rLanguage) {
    switchLanguage(rLanguage);

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
      mainWidget(), tr("Restart required"),
      tr("A restart of the OpenStudio Application is required for language changes to be fully functionnal.\nWould you like to restart now?"),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply == QMessageBox::Yes) {
      revertToSaved();
    }
  }
}

bool OpenStudioApp::switchLanguage(const QString& rLanguage) {

  qDebug() << "Trying to change language from '" << m_currLang << "' to '" << rLanguage << "'.";
  m_currLang = rLanguage;
  QLocale loc = QLocale(m_currLang);
  QLocale::setDefault(loc);
  // QString languageName = QLocale::languageToString(loc.language());

  this->removeTranslator(&m_qtTranslator);
  if (m_qtTranslator.load(loc, QLatin1String("qt"), QLatin1String("_"), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
    qDebug() << "m_qtTranslator ok";
    this->installTranslator(&m_qtTranslator);
  } else {
    qDebug() << "m_qtTranslator not ok for m_currLang=" << m_currLang;
  }

  this->removeTranslator(&m_qtBaseTranslator);
  if (m_qtBaseTranslator.load(loc, QLatin1String("qtbase"), QLatin1String("_"), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
    qDebug() << "m_qtBaseTranslator ok";
    this->installTranslator(&m_qtBaseTranslator);
  } else if (m_currLang == "zh_CN") {
    // For some reason, zh_CN doesn't exist for qt_base but zh_TW does. Using that...
    QLocale loc2("zh_TW");
    if (m_qtBaseTranslator.load(loc2, QLatin1String("qtbase"), QLatin1String("_"), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
      qDebug() << "m_qtBaseTranslator ok, with alternative zh_TW";
    } else {
      qDebug() << "m_qtBaseTranslator not ok for m_currLang=" << m_currLang << ", tried zh_TW too but it failed";
    }

  } else {
    qDebug() << "m_qtBaseTranslator not ok for m_currLang=" << m_currLang;
  }

  // remove the old translator
  this->removeTranslator(&m_translator);

  if (m_translator.load(loc, QLatin1String("OpenStudioApp"), QLatin1String("_"), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
    qDebug() << "\n\n\nINSTALLING lang = " << QLocale::languageToString(loc.language()) << "\n\n\n";

    this->installTranslator(&m_translator);
  } else {
    if (m_currLang != "en") {
      qDebug() << "\n\n\nFAILED TO INSTALL TRANSLATOR for lang = " << QLocale::languageToString(loc.language()) << "\n\n\n";
      return false;
    } else {
      qDebug() << "\n\n\nNO TRANSLATOR NEEDED for lang = " << QLocale::languageToString(loc.language()) << "\n\n\n";
    }
  }

  if (m_currLang == QString("fa")) {
    // Force Right to Left display. This is not done automatically like in Arabic because qt itself isn't translated (no qt_fa.qm / qt_base_fa.qm)
    qDebug() << "Forcing RightToLeft";
    this->setLayoutDirection(Qt::RightToLeft);
  }

  return true;
}

void OpenStudioApp::loadLibrary() {
  if (this->currentDocument()) {
    QWidget* parent = this->currentDocument()->mainWindow();

    QString fileName = QFileDialog::getOpenFileName(parent, tr("Select Library"), toQString(resourcesPath()), tr("(*.osm)"));

    if (!(fileName == "")) {
      QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

      auto paths = libraryPaths();

      auto path = toPath(fileName.toStdString());

      if (std::find(paths.begin(), paths.end(), path) == paths.end()) {
        // If the loaded library isn't already in the list of paths, we add it and rewrite all
        paths.push_back(path);
        writeLibraryPaths(paths);

        auto future = QtConcurrent::run(this, &OpenStudioApp::buildCompLibraries);
        m_changeLibrariesWatcher.setFuture(future);
        connect(&m_changeLibrariesWatcher, &QFutureWatcher<std::vector<std::string>>::finished, this, &OpenStudioApp::onChangeDefaultLibrariesDone);
      }
    }
  }
}

void OpenStudioApp::changeDefaultLibraries() {
  auto defaultPaths = defaultLibraryPaths();
  auto paths = libraryPaths();
  auto resources = resourcesPath();
  // Starts the library dialog
  LibraryDialog dialog(paths, defaultPaths, resources);
  auto code = dialog.exec();
  auto newPaths = dialog.paths();

  // If user accepts its changes, and there are actually changes to the list of paths
  if ((code == QDialog::Accepted) && (paths != newPaths)) {

    // Write the library settings
    writeLibraryPaths(newPaths);

    // Trigger actual loading of the libraries
    auto future = QtConcurrent::run(this, &OpenStudioApp::buildCompLibraries);
    m_changeLibrariesWatcher.setFuture(future);
    connect(&m_changeLibrariesWatcher, &QFutureWatcher<std::vector<std::string>>::finished, this, &OpenStudioApp::onChangeDefaultLibrariesDone);
  }
}

void OpenStudioApp::removeLibraryFromsSettings(const openstudio::path& path) {
  // erase the given path from the current list of paths
  auto paths = libraryPaths();
  paths.erase(std::remove(paths.begin(), paths.end(), path), paths.end());
  // Rewrite all
  writeLibraryPaths(paths);
}

void OpenStudioApp::showFailedLibraryDialog(const std::vector<std::string>& failedPaths) {
  if (!failedPaths.empty()) {
    QString text = tr("Failed to load the following libraries...\n\n");
    for (const auto& path : failedPaths) {
      text.append(QString::fromStdString(path));
      text.append("\n");
    }
    // text.append("\n\nYou will now be able to modify the library paths or restore to the default paths");
    // QMessageBox::critical(nullptr, QString("Failed to load library"), text);

    text.append(tr("\n\nWould you like to Restore library paths to default values or Open the library settings to change them manually?"));

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(mainWidget(), QString("Failed to load library"), text, QMessageBox::RestoreDefaults | QMessageBox::Open,
                                  QMessageBox::Open);
    if (reply == QMessageBox::RestoreDefaults) {
      QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
      settings.remove("library");
    } else {
      // Open the library dialog
      changeDefaultLibraries();
    }
  }
}

void OpenStudioApp::onChangeDefaultLibrariesDone() {
  auto failed = m_changeLibrariesWatcher.result();

  showFailedLibraryDialog(failed);

  auto doc = currentDocument();
  if (doc) {
    doc->setComponentLibrary(m_compLibrary);
  }
}

std::vector<openstudio::path> OpenStudioApp::defaultLibraryPaths() const {
  std::vector<openstudio::path> paths;

  paths.push_back(resourcesPath() / toPath("default/hvac_library.osm"));
  paths.push_back(resourcesPath() / toPath("default/office_default.osm"));

  return paths;
}

std::vector<openstudio::path> OpenStudioApp::libraryPaths() const {
  std::vector<openstudio::path> paths;

  QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

  openstudio::path resPath = resourcesPath();

  int size = settings.beginReadArray("library");
  for (int i = 0; i < size; ++i) {
    settings.setArrayIndex(i);
    auto path = toPath(settings.value("path").toString());

    // Read whether this path is in the resource folder, if not present, assume its absolute
    auto is_resource = settings.value("is_resource", false).toBool();
    // We stored resources path as relative, so recompute an absolute canonical path
    if (is_resource) {
      // std::cout << "i=" << i << "; Path rel=" << path << "\n";
      // std::cout << "i=" << i << "; Path abs=" << resPath / path << "\n";

      paths.push_back(resPath / path);

    } else {
      // std::cout << "i=" << i << "; Path is already abs=" << resPath / path << "\n";
      paths.push_back(path);
    }
  }
  settings.endArray();

  if (paths.empty()) {
    return defaultLibraryPaths();
  } else {
    return paths;
  }
}

openstudio::path OpenStudioApp::inferredDViewPath() const {

  openstudio::path result;

#if defined _WIN32
  openstudio::path dview_executable("DView.exe");
  // todo: add mac
#else
  openstudio::path dview_executable("DView");
#endif

  openstudio::path dviewPath = openstudio::findInSystemPath(dview_executable);
  LOG_FREE(Debug, "OpenStudioApp", "inferredDViewPath, findInSystemPath returned dviewPath = '" << dviewPath << "'.");

  // findInSystemPath returns whatever was passed if not found...
  if (!dviewPath.empty() && (dviewPath != dview_executable)) {
    result = openstudio::completeAndNormalize(dviewPath);
  }

  LOG_FREE(Debug, "OpenStudioApp", "inferredDViewPath = '" << result << "'.");

  return result;
}

void OpenStudioApp::setDviewPath(const openstudio::path& t_dviewPath) {

  LOG_FREE(Debug, "OpenStudioApp", "setDViewPath called with t_dviewPath = '" << t_dviewPath << "'.");

  m_dviewPath.clear();

  if (!t_dviewPath.empty()) {
    LOG_FREE(Debug, "OpenStudioApp", "setDViewPath t_dviewPath is not empty.");

    // check if exists?
    if (openstudio::filesystem::exists(t_dviewPath) && !openstudio::filesystem::is_directory(t_dviewPath)) {
      m_dviewPath = t_dviewPath;
    } else {
      LOG_FREE(Error, "OpenStudioApp", "setDViewPath: t_dviewPath doesn't not appear to be valid: '" << t_dviewPath << "'.");
    }
  }

  LOG_FREE(Debug, "OpenStudioApp", "setDViewPath: m_dviewPath = '" << m_dviewPath << "'.");
}

openstudio::path OpenStudioApp::dviewPath() const {
  if (m_dviewPath.empty()) {
    return inferredDViewPath();
  } else {
    return m_dviewPath;
  }
}

void OpenStudioApp::configureExternalTools() {

  // Get the actual one, or the inferred one
  openstudio::path t_dviewPath = dviewPath();
  LOG_FREE(Debug, "OpenStudioApp", "configureExternalTools: dviewPath() = '" << t_dviewPath << "'.");

  // Starts the External Tools dialog
  ExternalToolsDialog dialog(t_dviewPath);  // TODO: currently only sets DView

  auto code = dialog.exec();

  // If user accepts its changes, and there are actually changes to the list of paths
  if (code == QDialog::Accepted) {

    auto newDviewPath = dialog.dviewPath();
    if (newDviewPath != m_dviewPath) {

      // Write the library settings
      setDviewPath(newDviewPath);  // This will call inferredDviewPath again if the field was cleared.
      QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

      if (m_dviewPath.empty()) {
        settings.remove("dviewPath");
        // TODO: log that stuff didn't pan out
      } else {
        settings.setValue("dviewPath", toQString(m_dviewPath));
      }
    }

    // example if adding another one
    //auto newOtherToolPath = dialog.otherToolPath();
    //if (newOtherToolPath != m_otherToolPath) {
    //// Write the library settings
    //QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    //// Set OtherToolPath here
    //if ( m_otherToolPath.empty() ) {
    //settings.remove("otherToolPath");
    //// TODO: log that stuff didn't pan out
    //} else {
    //settings.setValue("otherToolPath", toQString(m_otherToolPath));
    //}
    //}
  }
}

}  // namespace openstudio
