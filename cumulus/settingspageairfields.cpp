/***********************************************************************
 **
 **   settingspageairfields.cpp
 **
 **   This file is part of Cumulus.
 **
 ************************************************************************
 **
 **   Copyright (c): 2008-2013 Axel Pauli
 **
 **   This file is distributed under the terms of the General Public
 **   License. See the file COPYING for more information.
 **
 **   $Id$
 **
 ***********************************************************************/

/**
 * \author Axel Pauli
 *
 * \brief Contains the airfield data settings.
 *
 * \date 2008-2013
 *
 * \version $Id$
 */

#ifndef QT_5
#include <QtGui>
#else
#include <QtWidgets>
#endif

#ifdef QTSCROLLER
#include <QtScroller>
#endif

#include "generalconfig.h"
#include "helpbrowser.h"
#include "layout.h"
#include "mapcontents.h"
#include "numberEditor.h"
#include "settingspageairfields.h"
#include "settingspageairfieldloading.h"

#ifdef INTERNET
#include "httpclient.h"
#endif

extern MapContents *_globalMapContents;

SettingsPageAirfields::SettingsPageAirfields(QWidget *parent) :
  QWidget(parent)
{
  setObjectName("SettingsPageAirfields");
  setWindowFlags( Qt::Tool );
  setWindowModality( Qt::WindowModal );
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle( tr("Settings - Airfields") );

  if( parent )
    {
      resize( parent->size() );
    }

  // Layout used by scroll area
  QHBoxLayout *sal = new QHBoxLayout;

  // new widget used as container for the dialog layout.
  QWidget* sw = new QWidget;

  // Scroll area
  QScrollArea* sa = new QScrollArea;
  sa->setWidgetResizable( true );
  sa->setFrameStyle( QFrame::NoFrame );
  sa->setWidget( sw );

#ifdef QSCROLLER
  QScroller::grabGesture( sa->viewport(), QScroller::LeftMouseButtonGesture );
#endif

#ifdef QTSCROLLER
  QtScroller::grabGesture( sa->viewport(), QtScroller::LeftMouseButtonGesture );
#endif

  // Add scroll area to its own layout
  sal->addWidget( sa );

  QHBoxLayout *contentLayout = new QHBoxLayout;
  setLayout(contentLayout);

  // Pass scroll area layout to the content layout.
  contentLayout->addLayout( sal, 10 );

  // The parent of the layout is the scroll widget
  QVBoxLayout *topLayout = new QVBoxLayout(sw);

  QGridLayout *sourceLayout = new QGridLayout;
  QLabel* lbl = new QLabel( tr("Source:") );
  sourceLayout->addWidget( lbl, 0, 0 );
  m_sourceBox = new QComboBox;
  m_sourceBox->addItem("OpenAIP");
  m_sourceBox->addItem("Welt2000");
  sourceLayout->addWidget( m_sourceBox, 0, 1 );

  QPushButton *cmdHelp = new QPushButton( tr("Help") );
  sourceLayout->addWidget( cmdHelp, 0, 3 );
  sourceLayout->setColumnStretch( 2, 5 );
  topLayout->addLayout(sourceLayout);

  connect( m_sourceBox, SIGNAL(currentIndexChanged(int)),
           this, SLOT(slot_sourceChanged(int)));

  connect( cmdHelp, SIGNAL(clicked()), this, SLOT(slot_openHelp()) );

  m_oaipGroup = new QGroupBox( "www.openaip.net", this );
  topLayout->addWidget(m_oaipGroup);

  int grow = 0;
  QGridLayout *oaipLayout = new QGridLayout(m_oaipGroup);
  lbl = new QLabel( tr("Home Radius:") );
  oaipLayout->addWidget(lbl, grow, 0);

  m_homeRadiusOaip = new NumberEditor( this );
  m_homeRadiusOaip->setDecimalVisible( false );
  m_homeRadiusOaip->setPmVisible( false );
  m_homeRadiusOaip->setMaxLength(4);
  m_homeRadiusOaip->setSuffix( " " + Distance::getUnitText() );
  m_homeRadiusOaip->setSpecialValueText(tr("Off"));
  m_homeRadiusOaip->setRange( 0, 9999 );
  oaipLayout->addWidget(m_homeRadiusOaip, grow, 1);
  grow++;

  QPushButton *cmdLoadFiles = new QPushButton( tr("Load") );
  oaipLayout->addWidget( cmdLoadFiles, grow, 0 );
  oaipLayout->setColumnStretch( 2, 5 );

  connect( cmdLoadFiles, SIGNAL(clicked()), this, SLOT(slot_openLoadDialog()) );

  //----------------------------------------------------------------------------
  m_weltGroup = new QGroupBox("www.segelflug.de/vereine/welt2000", this);
  topLayout->addWidget(m_weltGroup);

  QGridLayout* weltLayout = new QGridLayout(m_weltGroup);

  grow = 0;
  lbl = new QLabel(tr("Country Filter:"), (m_weltGroup));
  weltLayout->addWidget(lbl, grow, 0);

  Qt::InputMethodHints imh;

  m_countryFilter = new QLineEdit(m_weltGroup);
  imh = (m_countryFilter->inputMethodHints() | Qt::ImhNoPredictiveText);
  m_countryFilter->setInputMethodHints(imh);

  weltLayout->addWidget(m_countryFilter, grow, 1, 1, 3);
  grow++;

  lbl = new QLabel(tr("Home Radius:"), m_weltGroup);
  weltLayout->addWidget(lbl, grow, 0);

  m_homeRadiusW2000 = new NumberEditor( this );
  m_homeRadiusW2000->setDecimalVisible( false );
  m_homeRadiusW2000->setPmVisible( false );
  m_homeRadiusW2000->setMaxLength(4);
  m_homeRadiusW2000->setSuffix( " " + Distance::getUnitText() );
  m_homeRadiusW2000->setRange( 0, 9999 );
  QRegExpValidator* eValidator = new QRegExpValidator( QRegExp( "([1-9][0-9]{0,3})" ), this );
  m_homeRadiusW2000->setValidator( eValidator );
  weltLayout->addWidget(m_homeRadiusW2000, grow, 1 );

  m_loadOutlandings = new QCheckBox( tr("Load Outlandings"), m_weltGroup );
  weltLayout->addWidget(m_loadOutlandings, grow, 3, Qt::AlignRight );
  grow++;

#ifdef INTERNET

  weltLayout->setRowMinimumHeight(grow++, 10);

  m_installWelt2000 = new QPushButton( tr("Install"), m_weltGroup );
  m_installWelt2000->setToolTip(tr("Install Welt2000 data"));
  weltLayout->addWidget(m_installWelt2000, grow, 0 );

  connect( m_installWelt2000, SIGNAL( clicked()), this, SLOT(slot_installWelt2000()) );

  m_welt2000FileName = new QLineEdit(m_weltGroup);
  m_welt2000FileName->setInputMethodHints(imh);
  m_welt2000FileName->setToolTip(tr("Enter Welt2000 filename as to see on the web page"));
  weltLayout->addWidget(m_welt2000FileName, grow, 1, 1, 3);

#endif

  weltLayout->setColumnStretch(2, 10);

  //----------------------------------------------------------------------------
  // JD: adding group box for diverse list display settings
  grow = 0;
  QGroupBox* listGroup = new QGroupBox(tr("List Display"), this);
  topLayout->addWidget(listGroup);
  topLayout->addStretch(10);

  QGridLayout* listLayout = new QGridLayout(listGroup);

  lbl = new QLabel(tr( "More space in AF/WP/OL lists:"), listGroup);
  listLayout->addWidget(lbl, grow, 0);

  m_afMargin = new NumberEditor( this );
  m_afMargin->setDecimalVisible( false );
  m_afMargin->setPmVisible( false );
  m_afMargin->setMaxLength(2);
  m_afMargin->setSuffix( tr(" Pixels") );
  m_afMargin->setMaximum( 30 );
  m_afMargin->setTitle("0...30");
  eValidator = new QRegExpValidator( QRegExp( "([0-9]|[1-2][0-9]|30)" ), this );
  m_afMargin->setValidator( eValidator );
  listLayout->addWidget(m_afMargin, grow, 1 );
  grow++;
  lbl = new QLabel(tr( "More space in Emergency list:"), listGroup);
  listLayout->addWidget(lbl, grow, 0);

  m_rpMargin = new NumberEditor( this );
  m_rpMargin->setDecimalVisible( false );
  m_rpMargin->setPmVisible( false );
  m_rpMargin->setMaxLength(2);
  m_rpMargin->setSuffix( tr(" Pixels") );
  m_rpMargin->setMaximum( 30 );
  m_rpMargin->setTitle("0...30");
  eValidator = new QRegExpValidator( QRegExp( "([0-9]|[1-2][0-9]|30)" ), this );
  m_rpMargin->setValidator( eValidator );
  listLayout->addWidget(m_rpMargin, grow, 1 );
  grow++;
  listLayout->setRowStretch(grow, 10);
  listLayout->setColumnStretch(2, 10);

#ifndef ANDROID
  // Android makes trouble, if word detection is enabled and the input is
  // changed by us.
  connect( m_countryFilter, SIGNAL(textChanged(const QString&)),
           this, SLOT(slot_filterChanged(const QString&)) );
#endif

  QPushButton *cancel = new QPushButton(this);
  cancel->setIcon(QIcon(GeneralConfig::instance()->loadPixmap("cancel.png")));
  cancel->setIconSize(QSize(IconSize, IconSize));
  cancel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::QSizePolicy::Preferred);

  QPushButton *ok = new QPushButton(this);
  ok->setIcon(QIcon(GeneralConfig::instance()->loadPixmap("ok.png")));
  ok->setIconSize(QSize(IconSize, IconSize));
  ok->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::QSizePolicy::Preferred);

  QLabel *titlePix = new QLabel(this);
  titlePix->setPixmap(GeneralConfig::instance()->loadPixmap("setup.png"));

  connect(ok, SIGNAL(pressed()), this, SLOT(slotAccept()));
  connect(cancel, SIGNAL(pressed()), this, SLOT(slotReject()));

  QVBoxLayout *buttonBox = new QVBoxLayout;
  buttonBox->setSpacing(0);
  buttonBox->addStretch(2);
  buttonBox->addWidget(cancel, 1);
  buttonBox->addSpacing(30);
  buttonBox->addWidget(ok, 1);
  buttonBox->addStretch(2);
  buttonBox->addWidget(titlePix);
  contentLayout->addLayout(buttonBox);

  load();
}

SettingsPageAirfields::~SettingsPageAirfields()
{
}

void SettingsPageAirfields::slot_sourceChanged( int index )
{
  // Toggle source visibility
  if( index == 0 )
    {
      m_oaipGroup->setVisible( true );
      m_weltGroup->setVisible( false );
    }
  else
    {
      m_oaipGroup->setVisible( false );
      m_weltGroup->setVisible( true );
    }
}

void SettingsPageAirfields::slotAccept()
{
  if( checkChanges() )
    {
      if( save() == false )
        {
          return;
        }

      emit settingsChanged();
    }

  QWidget::close();
}

void SettingsPageAirfields::slotReject()
{
  QWidget::close();
}

/**
 * Called to initiate loading of the configuration file
 */
void SettingsPageAirfields::load()
{
  GeneralConfig *conf = GeneralConfig::instance();

  m_sourceBox->setCurrentIndex( conf->getAirfieldSource() );
  slot_sourceChanged( conf->getAirfieldSource() );

  m_homeRadiusOaip->setValue( conf->getAirfieldHomeRadius() );
  m_countryFilter->setText( conf->getWelt2000CountryFilter() );

  // @AP: radius value is stored without considering unit.
  m_homeRadiusW2000->setValue( conf->getAirfieldHomeRadius() );

  if( conf->getWelt2000LoadOutlandings() )
    {
      m_loadOutlandings->setCheckState( Qt::Checked );
    }
  else
    {
      m_loadOutlandings->setCheckState( Qt::Unchecked );
    }

#ifdef INTERNET

  m_welt2000FileName->setText( conf->getWelt2000FileName() );

#endif

  m_afMargin->setValue(conf->getListDisplayAFMargin());
  m_rpMargin->setValue(conf->getListDisplayRPMargin());

  // sets home radius enabled/disabled in dependency to filter string
  slot_filterChanged(m_countryFilter->text());
}

/**
 * Called to initiate saving to the configuration file.
 */
bool SettingsPageAirfields::save()
{
  GeneralConfig *conf = GeneralConfig::instance();

  // Save change states.
  bool openAipChanged  = checkIsOpenAipChanged();
  bool welt2000Changed = checkIsWelt2000Changed();

  conf->setAirfieldSource( m_sourceBox->currentIndex() );

  // Look which source widget is active to decide what have to be saved.
  if( m_sourceBox->currentIndex() == 0 )
    {
      conf->setAirfieldHomeRadius(m_homeRadiusOaip->value());
    }
  else
    {
      conf->setAirfieldHomeRadius(m_homeRadiusW2000->value());

      // We will check, if the country entries of Welt2000 are
      // correct. If not a warning message is displayed and the
      // modifications are discarded.
      QStringList clist = m_countryFilter->text().split(QRegExp("[, ]"),
                          QString::SkipEmptyParts);

      for( int i = 0; i < clist.size(); i++ )
        {
          const QString& s = clist.at(i);

          if( ! (s.length() == 2 && s.contains(QRegExp("[A-Za-z][A-Za-z]")) == true) )
            {
              QMessageBox mb( QMessageBox::Warning,
                              tr( "Please check entries" ),
                              tr("Every Welt2000 county sign must consist of two letters!<br>Allowed separators are space and comma!"),
                              QMessageBox::Ok,
                              this );

#ifdef ANDROID

              mb.show();
              QPoint pos = mapToGlobal(QPoint( width()/2  - mb.width()/2,
                                               height()/2 - mb.height()/2 ));
              mb.move( pos );

#endif
              mb.exec();
              return false;
            }
        }

      conf->setWelt2000CountryFilter(m_countryFilter->text().trimmed().toUpper());

      if( m_loadOutlandings->checkState() == Qt::Checked )
        {
          conf->setWelt2000LoadOutlandings( true );
        }
      else
        {
          conf->setWelt2000LoadOutlandings( false );
        }
    }

  conf->setListDisplayAFMargin(m_afMargin->value());
  conf->setListDisplayRPMargin(m_rpMargin->value());
  conf->save();

  if( openAipChanged == true )
    {
      emit reloadOpenAip();
    }

  if( welt2000Changed == true )
    {
      emit reloadWelt2000();
    }

  return true;
}

/**
 * Called if the text of the filter has been changed
 */
void SettingsPageAirfields::slot_filterChanged(const QString& text)
{
  Q_UNUSED( text )
}

void SettingsPageAirfields::slot_openLoadDialog()
{
  SettingsPageAirfieldLoading* dlg = new SettingsPageAirfieldLoading(this);

  connect( dlg, SIGNAL(fileListChanged()),
           _globalMapContents, SLOT(slotReloadOpenAipAirfields()) );

  dlg->setVisible( true );
}

void SettingsPageAirfields::slot_openHelp()
{
  // Check, which help is requested.
  QString file = (m_sourceBox->currentIndex() == 0 ? "cumulus-maps-openAIP.html" : "cumulus-maps-welt2000.html");

  HelpBrowser *hb = new HelpBrowser( this, file );
  hb->resize( this->size() );
  hb->setWindowState( windowState() );
  hb->setVisible( true );
}

bool SettingsPageAirfields::checkChanges()
{
  bool changed =
      (GeneralConfig::instance()->getAirfieldSource() != m_sourceBox->currentIndex());

  changed |= checkIsOpenAipChanged();
  changed |= checkIsWelt2000Changed();
  changed |= checkIsListDisplayChanged();

  return changed;
}

#ifdef INTERNET

/**
 * Called, if install button of Welt2000 is clicked.
 */
void SettingsPageAirfields::slot_installWelt2000()
{
  QString wfn = m_welt2000FileName->text().trimmed();

  if( wfn.isEmpty() )
    {
      QMessageBox mb( QMessageBox::Information,
                      tr( "Welt2000 settings invalid!" ),
                      tr( "Please add a valid Welt2000 filename!"),
                      QMessageBox::Ok,
                      this );

#ifdef ANDROID

      mb.show();
      QPoint pos = mapToGlobal(QPoint( width()/2  - mb.width()/2,
                                       height()/2 - mb.height()/2 ));
      mb.move( pos );

#endif

      mb.exec();
      return;
    }

  QMessageBox mb( QMessageBox::Question,
                  tr( "Download Welt2000?"),
                  tr( "Active Internet connection is needed!") +
                  QString("<p>") + tr("Start download now?"),
                  QMessageBox::Yes | QMessageBox::No,
                  this );

  mb.setDefaultButton( QMessageBox::No );

#ifdef ANDROID

  mb.show();
  QPoint pos = mapToGlobal(QPoint( width()/2  - mb.width()/2,
                                   height()/2 - mb.height()/2 ));
  mb.move( pos );

#endif


  if( mb.exec() == QMessageBox::No )
    {
      return;
    }

  emit downloadWelt2000( wfn );
}

#endif

bool SettingsPageAirfields::checkIsOpenAipChanged()
{
  bool changed = false;
  GeneralConfig *conf = GeneralConfig::instance();

  changed |= (conf->getAirfieldSource() == 1 && m_sourceBox->currentIndex() == 0);
  changed |= (conf->getAirfieldHomeRadius() != m_homeRadiusOaip->value());

  return changed;
}

/**
 * Checks, if the configuration of the Welt2000 has been changed
 */
bool SettingsPageAirfields::checkIsWelt2000Changed()
{
  bool changed = false;
  GeneralConfig *conf = GeneralConfig::instance();

  changed |= (conf->getAirfieldSource() == 0 && m_sourceBox->currentIndex() == 1);
  changed |= (conf->getWelt2000CountryFilter() != m_countryFilter->text());
  changed |= (conf->getAirfieldHomeRadius() != m_homeRadiusW2000->value());

  bool currentState = false;

  if( m_loadOutlandings->checkState() == Qt::Checked )
    {
      currentState = true;
    }

  changed |= (conf->getWelt2000LoadOutlandings() != currentState);

  // qDebug( "SettingsPageAirfields::checkIsWelt2000Changed(): %d", changed );
  return changed;
}

/**
 * Checks if the configuration of list display has been changed
 */
bool SettingsPageAirfields::checkIsListDisplayChanged()
{
  bool changed = false;
  GeneralConfig *conf = GeneralConfig::instance();

  changed |= (conf->getListDisplayAFMargin() != m_afMargin->value());
  changed |= (conf->getListDisplayRPMargin() != m_rpMargin->value());

  // qDebug( "SettingsPageAirfields::checkIsListDisplayChanged(): %d", changed );
  return changed;
}
