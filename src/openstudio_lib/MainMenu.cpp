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

#include "MainMenu.hpp"

#include "OSAppBase.hpp"
#include "OSDocument.hpp"

#include <openstudio/osversion/VersionTranslator.hpp>

#include <openstudio/utilities/core/Assert.hpp>
#include <openstudio/utilities/core/Path.hpp>

#include <QMenu>
#include <QMessageBox>

namespace openstudio {

MainMenu::MainMenu(bool isIP, bool isPlugin, const QString& currLang, QWidget* parent)
  : QMenuBar(parent), m_isPlugin(isPlugin), m_isIP(isIP), m_currLang(currLang) {

  QAction* action = nullptr;

  // File menu
  m_fileMenu = new QMenu(tr("&File"), this);
  addMenu(m_fileMenu);

  // DLM: actions which result in this menu being deleted should be queued
  action = new QAction(tr("&New"), this);
  action->setShortcut(QKeySequence(QKeySequence::New));
  m_fileMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::newClicked, Qt::QueuedConnection);

  // DLM: actions which result in this menu being deleted should be queued
  action = new QAction(tr("&Open"), this);
  action->setShortcut(QKeySequence(QKeySequence::Open));
  m_fileMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::loadFileClicked, Qt::QueuedConnection);

  m_fileMenu->addSeparator();

  m_revertToSavedAction = new QAction(tr("&Revert to Saved"), this);
  m_revertToSavedAction->setShortcut(QKeySequence(QKeySequence(tr("Ctrl+R"))));  // QKeySequence(Qt::CTRL + Qt::Key_R)
  // m_revertToSavedAction->setDisabled(true);
  m_revertToSavedAction->setDisabled(false);
  m_fileMenu->addAction(m_revertToSavedAction);
  connect(m_revertToSavedAction, &QAction::triggered, this, &MainMenu::revertFileClicked, Qt::QueuedConnection);

  action = new QAction(tr("&Save"), this);
  action->setShortcut(QKeySequence(QKeySequence::Save));
  m_fileMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::saveFileClicked);

  action = new QAction(tr("Save &As"), this);
  action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));
  m_fileMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::saveAsFileClicked);

  m_fileMenu->addSeparator();

  //formatMenu = editMenu->addMenu(tr("&Format"))
  QMenu* importMenu = m_fileMenu->addMenu(tr("&Import"));

  action = new QAction(tr("&IDF"), this);
  m_fileImportActions.push_back(action);
  importMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::importClicked, Qt::QueuedConnection);

  action = new QAction(tr("&gbXML"), this);
  m_fileImportActions.push_back(action);
  importMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::importgbXMLClicked, Qt::QueuedConnection);

  action = new QAction(tr("&SDD"), this);
  m_fileImportActions.push_back(action);
  importMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::importSDDClicked, Qt::QueuedConnection);

  action = new QAction(tr("I&FC"), this);
  m_fileImportActions.push_back(action);
  importMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::importIFCClicked, Qt::QueuedConnection);

  QMenu* exportMenu = m_fileMenu->addMenu(tr("&Export"));

  action = new QAction(tr("&IDF"), this);
  exportMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::exportClicked);

  action = new QAction(tr("&gbXML"), this);
  exportMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::exportgbXMLClicked);

  action = new QAction(tr("&SDD"), this);
  exportMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::exportSDDClicked);

  action = new QAction(tr("&Load Library"), this);
  m_fileMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::loadLibraryClicked);
  m_fileImportActions.push_back(action);
  //m_preferencesActions.push_back(action); // DLM: I'm unclear if this should be enabled/disabled with preferences or file imports, right now does not matter

  if (!m_isPlugin) {

    m_fileMenu->addSeparator();

    //m_fileMenu->addAction(action);
    //connect(action, &QAction::triggered, this, &MainMenu::showRubyConsoleClicked);

    m_fileMenu->addSeparator();

    action = new QAction(tr("E&xit"), this);
    action->setShortcuts(QKeySequence::Quit);
    m_fileMenu->addAction(action);
    connect(action, &QAction::triggered, this, &MainMenu::exitClicked, Qt::QueuedConnection);
  }

  // Preferences menu
  m_preferencesMenu = new QMenu(tr("&Preferences"), this);
  addMenu(m_preferencesMenu);

  QMenu* unitsMenu = m_preferencesMenu->addMenu(tr("&Units"));

  m_displaySIUnitsAction = new QAction(tr("Metric (&SI)"), this);
  m_preferencesActions.push_back(m_displaySIUnitsAction);
  m_displaySIUnitsAction->setCheckable(true);
  unitsMenu->addAction(m_displaySIUnitsAction);
  connect(m_displaySIUnitsAction, &QAction::triggered, this, &MainMenu::displaySIUnitsClicked);

  m_displayIPUnitsAction = new QAction(tr("English (&I-P)"), this);
  m_preferencesActions.push_back(m_displayIPUnitsAction);
  m_displayIPUnitsAction->setCheckable(true);
  unitsMenu->addAction(m_displayIPUnitsAction);
  connect(m_displayIPUnitsAction, &QAction::triggered, this, &MainMenu::displayIPUnitsClicked);

  action = new QAction(tr("&Change My Measures Directory"), this);
  m_preferencesActions.push_back(action);
  m_preferencesMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::changeMyMeasuresDir);

  action = new QAction(tr("&Change Default Libraries"), this);
  m_preferencesActions.push_back(action);
  m_preferencesMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::changeDefaultLibrariesClicked);

  action = new QAction(tr("&Configure External Tools"), this);
  m_preferencesActions.push_back(action);
  m_preferencesMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::configureExternalToolsClicked);

  QMenu* langMenu = m_preferencesMenu->addMenu(tr("&Language"));

  m_langEnglishAction = new QAction(tr("English"), this);
  m_preferencesActions.push_back(m_langEnglishAction);
  m_langEnglishAction->setCheckable(true);
  langMenu->addAction(m_langEnglishAction);
  connect(m_langEnglishAction, &QAction::triggered, this, &MainMenu::langEnglishClicked);

  m_langFrenchAction = new QAction(tr("French"), this);
  m_preferencesActions.push_back(m_langFrenchAction);
  m_langFrenchAction->setCheckable(true);
  langMenu->addAction(m_langFrenchAction);
  connect(m_langFrenchAction, &QAction::triggered, this, &MainMenu::langFrenchClicked);

  m_langArabicAction = new QAction(tr("Arabic"), this);
  m_preferencesActions.push_back(m_langArabicAction);
  m_langArabicAction->setCheckable(true);
  langMenu->addAction(m_langArabicAction);
  connect(m_langArabicAction, &QAction::triggered, this, &MainMenu::langArabicClicked);

  m_langSpanishAction = new QAction(tr("Spanish"), this);
  m_preferencesActions.push_back(m_langSpanishAction);
  m_langSpanishAction->setCheckable(true);
  langMenu->addAction(m_langSpanishAction);
  connect(m_langSpanishAction, &QAction::triggered, this, &MainMenu::langSpanishClicked);

  m_langFarsiAction = new QAction(tr("Farsi"), this);
  m_preferencesActions.push_back(m_langFarsiAction);
  m_langFarsiAction->setCheckable(true);
  langMenu->addAction(m_langFarsiAction);
  connect(m_langFarsiAction, &QAction::triggered, this, &MainMenu::langFarsiClicked);

  m_langHebrewAction = new QAction(tr("Hebrew"), this);
  m_preferencesActions.push_back(m_langHebrewAction);
  m_langHebrewAction->setCheckable(true);
  langMenu->addAction(m_langHebrewAction);
  connect(m_langHebrewAction, &QAction::triggered, this, &MainMenu::langHebrewClicked);

  m_langItalianAction = new QAction(tr("Italian"), this);
  m_preferencesActions.push_back(m_langItalianAction);
  m_langItalianAction->setCheckable(true);
  langMenu->addAction(m_langItalianAction);
  connect(m_langItalianAction, &QAction::triggered, this, &MainMenu::langItalianClicked);

  m_langChineseAction = new QAction(tr("Chinese"), this);
  m_preferencesActions.push_back(m_langChineseAction);
  m_langChineseAction->setCheckable(true);
  langMenu->addAction(m_langChineseAction);
  connect(m_langChineseAction, &QAction::triggered, this, &MainMenu::langChineseClicked);

  m_langGreekAction = new QAction(tr("Greek"), this);
  m_preferencesActions.push_back(m_langGreekAction);
  m_langGreekAction->setCheckable(true);
  langMenu->addAction(m_langGreekAction);
  connect(m_langGreekAction, &QAction::triggered, this, &MainMenu::langGreekClicked);

  m_langPolishAction = new QAction(tr("Polish"), this);
  m_preferencesActions.push_back(m_langPolishAction);
  m_langPolishAction->setCheckable(true);
  langMenu->addAction(m_langPolishAction);
  connect(m_langPolishAction, &QAction::triggered, this, &MainMenu::langPolishClicked);

  m_langCatalanAction = new QAction(tr("Catalan"), this);
  m_preferencesActions.push_back(m_langCatalanAction);
  m_langCatalanAction->setCheckable(true);
  langMenu->addAction(m_langCatalanAction);
  connect(m_langCatalanAction, &QAction::triggered, this, &MainMenu::langCatalanClicked);

  m_langHindiAction = new QAction(tr("Hindi"), this);
  m_preferencesActions.push_back(m_langHindiAction);
  m_langHindiAction->setCheckable(true);
  langMenu->addAction(m_langHindiAction);
  connect(m_langHindiAction, &QAction::triggered, this, &MainMenu::langHindiClicked);

  m_langVietnameseAction = new QAction(tr("Vietnamese"), this);
  m_preferencesActions.push_back(m_langVietnameseAction);
  m_langVietnameseAction->setCheckable(true);
  langMenu->addAction(m_langVietnameseAction);
  connect(m_langVietnameseAction, &QAction::triggered, this, &MainMenu::langVietnameseClicked);

  m_langJapaneseAction = new QAction(tr("Japanese"), this);
  m_preferencesActions.push_back(m_langJapaneseAction);
  m_langJapaneseAction->setCheckable(true);
  langMenu->addAction(m_langJapaneseAction);
  connect(m_langJapaneseAction, &QAction::triggered, this, &MainMenu::langJapaneseClicked);

  m_langGermanAction = new QAction(tr("German"), this);
  m_preferencesActions.push_back(m_langGermanAction);
  m_langGermanAction->setCheckable(true);
  langMenu->addAction(m_langGermanAction);
  connect(m_langGermanAction, &QAction::triggered, this, &MainMenu::langGermanClicked);

  action = new QAction(tr("Add a new language"), this);
  m_preferencesActions.push_back(action);
  langMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::addingNewLanguageClicked);

  //action = new QAction(tr("&Scan for Tools"),this);
  //m_preferencesMenu->addAction(action);
  //connect(action, &QAction::triggered, this, &MainMenu::scanForToolsClicked);

  //action = new QAction(tr("Show &Tools"),this);
  //m_preferencesMenu->addAction(action);
  //connect(action, &QAction::triggered, this, &MainMenu::showRunManagerPreferencesClicked);

  //action = new QAction(tr("Change BCL Login Information"),this);
  //m_preferencesMenu->addAction(action);
  //connect(action, &QAction::triggered, this, &MainMenu::changeBclLogin);

  action = new QAction(tr("&Configure Internet Proxy"), this);
  m_preferencesActions.push_back(action);
  m_preferencesMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::configureProxyClicked);

  if (m_isIP) {
    m_displayIPUnitsAction->trigger();
  } else {
    m_displaySIUnitsAction->trigger();
  }

  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  if (m_currLang == "fr") {
    m_langFrenchAction->setChecked(true);
  } else if (m_currLang == "ar") {
    m_langArabicAction->setChecked(true);
  } else if (m_currLang == "es") {
    m_langSpanishAction->setChecked(true);
  } else if (m_currLang == "fa") {
    m_langFarsiAction->setChecked(true);
  } else if (m_currLang == "he") {
    m_langHebrewAction->setChecked(true);
  } else if (m_currLang == "it") {
    m_langItalianAction->setChecked(true);
  } else if (m_currLang == "zh_CN") {
    m_langChineseAction->setChecked(true);
  } else if (m_currLang == "el") {
    m_langGreekAction->setChecked(true);
  } else if (m_currLang == "pl") {
    m_langPolishAction->setChecked(true);
  } else if (m_currLang == "ca") {
    m_langCatalanAction->setChecked(true);
  } else if (m_currLang == "hi") {
    m_langHindiAction->setChecked(true);
  } else if (m_currLang == "vi") {
    m_langVietnameseAction->setChecked(true);
  } else if (m_currLang == "ja") {
    m_langJapaneseAction->setChecked(true);
  } else if (m_currLang == "de") {
    m_langGermanAction->setChecked(true);
  } else {
    // default to english
    // m_langEnglishAction->trigger();
    m_langEnglishAction->setChecked(true);
  }

  // Measure menu
  m_measureMenu = new QMenu(tr("&Components && Measures"), this);
  addMenu(m_measureMenu);

  action = new QAction(tr("&Apply Measure Now"), this);
  m_componentsMeasuresActions.push_back(action);
  action->setShortcut(QKeySequence(QKeySequence(tr("Ctrl+M"))));
  m_measureMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::applyMeasureClicked);

  action = new QAction(tr("Find &Measures"), this);
  m_componentsMeasuresActions.push_back(action);
  m_measureMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::downloadMeasuresClicked);

  action = new QAction(tr("Find &Components"), this);
  m_componentsMeasuresActions.push_back(action);
  m_measureMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::downloadComponentsClicked);

  // Help menu
  m_helpMenu = new QMenu(tr("&Help"), this);
  addMenu(m_helpMenu);

  action = new QAction(tr("OpenStudio &Help"), this);
  m_helpMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::helpClicked);

  action = new QAction(tr("Check For &Update"), this);
  m_helpMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::checkForUpdateClicked);

  action = new QAction(tr("&About"), this);
  m_helpMenu->addAction(action);
  connect(action, &QAction::triggered, this, &MainMenu::aboutClicked);
}

MainMenu::~MainMenu() {}

void MainMenu::displaySIUnitsClicked() {
  m_displaySIUnitsAction->setChecked(true);
  m_displayIPUnitsAction->setChecked(false);
  emit toggleUnitsClicked(false);
}

void MainMenu::displayIPUnitsClicked() {
  m_displayIPUnitsAction->setChecked(true);
  m_displaySIUnitsAction->setChecked(false);
  emit toggleUnitsClicked(true);
}

void MainMenu::langEnglishClicked() {
  m_langEnglishAction->setChecked(true);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("en");
}

void MainMenu::langFrenchClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(true);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("fr");
}

void MainMenu::langArabicClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(true);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("ar");
}

void MainMenu::langSpanishClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(true);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("es");
}

void MainMenu::langFarsiClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(true);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("fa");
}

void MainMenu::langHebrewClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(true);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("he");
}

void MainMenu::langItalianClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(true);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("it");
}

void MainMenu::langChineseClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(true);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("zh_CN");
}

void MainMenu::langGreekClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(true);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("el");
}

void MainMenu::langPolishClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(true);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("pl");
}

void MainMenu::langCatalanClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(true);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("ca");
}

void MainMenu::langHindiClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(true);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("hi");
}

void MainMenu::langVietnameseClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(true);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("vi");
}

void MainMenu::langJapaneseClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(true);
  m_langGermanAction->setChecked(false);

  emit changeLanguageClicked("ja");
}

void MainMenu::langGermanClicked() {
  m_langEnglishAction->setChecked(false);
  m_langFrenchAction->setChecked(false);
  m_langArabicAction->setChecked(false);
  m_langSpanishAction->setChecked(false);
  m_langFarsiAction->setChecked(false);
  m_langHebrewAction->setChecked(false);
  m_langItalianAction->setChecked(false);
  m_langChineseAction->setChecked(false);
  m_langGreekAction->setChecked(false);
  m_langPolishAction->setChecked(false);
  m_langCatalanAction->setChecked(false);
  m_langHindiAction->setChecked(false);
  m_langVietnameseAction->setChecked(false);
  m_langJapaneseAction->setChecked(false);
  m_langGermanAction->setChecked(true);

  emit changeLanguageClicked("de");
}

void MainMenu::addingNewLanguageClicked() {

  QMessageBox::information(
    nullptr, tr("Adding a new language"),
    tr("Adding a new language requires almost no coding skill, but it does require language skills: the only thing to do is to translate each "
       "sentence/word with the help of a dedicated software.\nIf you would like to see the OpenStudioApplication translated in your language of "
       "choice, we would welcome your help. Send an email to osc@openstudiocoalition.org specifying which language you want to add, and we will be "
       "in touch to help you get started."));
}

void MainMenu::enableRevertToSavedAction(bool enable) {
  // We no longer switch the action ON/OFF
  // m_revertToSavedAction->setEnabled(enable);

  // Instead, we display an asterisk to indicate whether we think the model is dirty or not
  // Note: (The MainWindow also displays an asterisk, so perhaps it's not needed)
  //if (enable) {
  //  //m_revertToSavedAction->setText(tr("&Revert to Saved *"));
  m_revertToSavedAction->setText(tr("&Revert to Saved"));
  //} else {
  //  m_revertToSavedAction->setText(tr("&Revert to Saved"));
  //}
}

void MainMenu::enableFileImportActions(bool enable) {
  for (const auto& action : m_fileImportActions) {
    action->setEnabled(enable);
  }
}

void MainMenu::enablePreferencesActions(bool enable) {
  for (const auto& action : m_preferencesActions) {
    action->setEnabled(enable);
  }
}

void MainMenu::enableComponentsMeasuresActions(bool enable) {
  for (const auto& action : m_componentsMeasuresActions) {
    action->setEnabled(enable);
  }
}

}  // namespace openstudio
