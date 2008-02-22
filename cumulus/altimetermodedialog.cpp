/***********************************************************************
**
**   altimetermodedialog.cpp
**
**   This file is part of Cumulus
**
************************************************************************
**
**   Copyright (c):  2004 by Eckhard Voellm
**                   2008 Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#include <QLabel>
#include <QFont>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QBoxLayout>
#include <QSignalMapper>

#include "generalconfig.h"
#include "altimetermodedialog.h"
#include "cucalc.h"
#include "glider.h"
#include "mapconfig.h"

extern MapConfig * _globalMapConfig;

int AltimeterModeDialog::_mode = 0; // MSL mode

AltimeterModeDialog::AltimeterModeDialog (QWidget *parent)
    : QDialog(parent, "altimetermodedialog", true, Qt::WStyle_StaysOnTop)
{
  setWindowTitle(tr("Altimeter"));

  setFont(QFont ( "Helvetica", 16, QFont::Bold ));

  QGroupBox* altMode = new QGroupBox(tr("Altimeter Mode"), this);
  _msl=new QRadioButton(tr("MSL"),altMode);
  _gnd=new QRadioButton(tr("AGL"),altMode);
  _std=new QRadioButton(tr("STD"),altMode);
  _msl->setChecked(true);

  _msl->setEnabled(true);
  _gnd->setEnabled(true);
  _std->setEnabled(true);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                 | QDialogButtonBox::Cancel);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setObjectName("mainlayout");
  QHBoxLayout* modeLayout = new QHBoxLayout(this);
  modeLayout->setObjectName("modelayout");
  modeLayout->addWidget(altMode);

  QHBoxLayout* radioLayout = new QHBoxLayout(altMode);
  radioLayout->setObjectName("radiolayout");
  radioLayout->addWidget(_msl);
  radioLayout->addWidget(_gnd);
  radioLayout->addWidget(_std);

  QVBoxLayout* buttonLayout = new QVBoxLayout(this);
  buttonLayout->setObjectName("buttonlayout");
  buttonLayout->addWidget(buttonBox);

  mainLayout->addLayout(modeLayout);
  mainLayout->addLayout(buttonLayout);
  setLayout (mainLayout);

  timeout = new QTimer(this);
  QSignalMapper* signalMapper = new QSignalMapper();
  connect(_msl, SIGNAL(clicked()), signalMapper, SLOT(map()));
  signalMapper->setMapping(_msl, 0);
  connect(_gnd, SIGNAL(clicked()), signalMapper, SLOT(map()));
  signalMapper->setMapping(_gnd, 1);
  connect(_std, SIGNAL(clicked()), signalMapper, SLOT(map()));
  signalMapper->setMapping(_std, 2);
  connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(change_mode(int)));

  connect (timeout, SIGNAL(timeout()), this, SLOT(reject()));
  connect (buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect (buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  load();
}

QString AltimeterModeDialog::Pretext()
{
  switch( _mode )
    {
    case 0:
      return QString("Msl");
    case 1:
      return QString("Agl");
    case 2:
    default:
      return QString("Std");
    }
}

AltimeterModeDialog::~AltimeterModeDialog()
{}

void AltimeterModeDialog::load()
{
  GeneralConfig *conf = GeneralConfig::instance();

  _mode = conf->getAltimeterMode();
  _toggling_mode = conf->getAltimeterToggleMode();

  switch (_mode) {
    case 0:
      _msl->setChecked(true);
      break;
    case 1:
      _gnd->setChecked(true);
      break;
    case 2:
      _std->setChecked(true);
      break;
    default:
      qFatal("AltimeterModeDialog::load(): invalid mode: %d", _mode);
  }

  setTimer();
}

void AltimeterModeDialog::work()
{
  // qDebug("_toggling_mode %d", _toggling_mode );

  GeneralConfig *conf = GeneralConfig::instance();

  _mode = conf->getAltimeterMode();
  _toggling_mode = conf->getAltimeterToggleMode();

  if( _toggling_mode )
    {
      // qDebug("_mode %d", _mode );

      switch( _mode )
        {
        case 0:
          save( 1 );
          break;
        case 1:
          save( 2 );
          break;
        case 2:
        default:
          save( 0 );
          break;
        }
    }
  else
    {
      load();
    }
}

void AltimeterModeDialog::save(int mode)
{
  _mode = mode;
  GeneralConfig *conf = GeneralConfig::instance();
  conf->setAltimeterMode(_mode);
  conf->save();
  // qDebug("Save new val %d", _mode );
  emit newAltimeterMode();
  emit settingsChanged();
}

void AltimeterModeDialog::change_mode (int mode) {
  _mode = mode;
}

void AltimeterModeDialog::accept()
{

  save(_mode);

  QDialog::accept();
}

void AltimeterModeDialog::setTimer()
{
  // @AP: let us take the user's defined info display time
  GeneralConfig *conf = GeneralConfig::instance();

  _time = conf->getInfoDisplayTime();

  timeout->start (_time * 1000);
}
