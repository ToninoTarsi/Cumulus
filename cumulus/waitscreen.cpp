/***********************************************************************
**
**   waitscreen.cpp
**
**   This file is part of Cumulus
**
************************************************************************
**
**   Copyright (c):  2002 by Andre Somers, 2008 Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#include <QFrame>
#include <QPainter>

#include <libgen.h>
#include "cumulusapp.h"
#include "waitscreen.h"
#include "whatsthat.h"
#include "mapview.h"
#include "generalconfig.h"

extern MapView *_globalMapView;

WaitScreen::WaitScreen(QWidget *parent ) :
  QDialog(parent, Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::WStyle_Title)
{
  setObjectName("WaitScreen");
  setModal(true);
  _screenUsage = true;

  setWindowTitle(tr("Please wait..."));

  QGridLayout * backLayout = new QGridLayout(this, 3,3,0);
  QGridLayout * topLayout = new QGridLayout(3,2,5);
  backLayout->addLayout(topLayout,1,1);
  backLayout->addRowSpacing(0,3);
  backLayout->addRowSpacing(2,3);
  backLayout->addColSpacing(0,3);
  backLayout->addColSpacing(2,3);
  topLayout->addColSpacing(0,45);

  QFrame * frm = new QFrame(this);
  frm->setFrameStyle(QFrame::WinPanel | QFrame::Raised);
  backLayout->addMultiCellWidget(frm,0,2,0,2);

  Glider=new QLabel(this);
  topLayout->addMultiCellWidget(Glider,0,2,0,0);

  QLabel * txt=new QLabel(tr("Cumulus is working. Please wait..."),this);
  topLayout->addWidget(txt,0,1);

  Text1=new QLabel(this);
  topLayout->addWidget(Text1,1,1);

  Text2=new QLabel(this);
  topLayout->addWidget(Text2,2,1);

  //  Prog=new QLabel(this);
  //  topLayout->addWidget(Prog,3,1);

  progress=0;
  lastRot=0;

  _gliders = GeneralConfig::instance()->loadPixmap("gliders.png");
  _glider.resize (40,40);
  _glider.fill(Glider->backgroundColor());

  QPainter p(&_glider);
  p.drawPixmap( 0, 0, _gliders, 0, 0, 40, 40);

  Glider->setPixmap(_glider);
}


WaitScreen::~WaitScreen()
{
}


/** This slot is used to set the main text, such as "Loading maps..." */
void WaitScreen::slot_SetText1(const QString& text)
{

  Text1->setText(text);
  Text2->setText("");

  // qDebug("slot_SetText1(): text=%s\n", text.latin1());

  if( WhatsThat::getInstance() ) {
    // @AP: Return, if popup is active to avoid a blocking of wm
    return;
  }

  if( screenUsage() ) {
    show();
    qDebug("========= WaitScreen::slot_SetText1() calls processEvents =========");
    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);

  } else {
    _globalMapView->message(text);
  }
}


/** This slot is used to set the secondairy text, such as the name of the airspacefile that is being loaded. It is also reset to an empty string if SetText1 is called. */
void WaitScreen::slot_SetText2(const QString& text)
{

  QString shortText = text;
  if(text.length()>32) {
    shortText="..." + text.right(32);

    int pos = shortText.findRev( QChar('/'), -1 );

    if( pos != -1 ) {
      // cut directory pathes
      shortText = shortText.right(shortText.length() - pos - 1 );
    }
  }

  Text2->setText(shortText);

  if( WhatsThat::getInstance() ) {
    // @AP: Return, if popup is active to avoid a blocking of wm
    return;
  }

  if( screenUsage() ) {
    show();
    qDebug("========= WaitScreen::slot_SetText2() calls processEvents =========");
    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
  } else {
    _globalMapView->message(shortText);
  }
}


/** This slot is called to indicate progress. It is used to rotate the glider-icon to indicate to the user that something is in fact happening... */
void WaitScreen::slot_Progress(int stepsize)
{
  if( WhatsThat::getInstance() ) {
    // @AP: Return, if popup is active to avoid a blocking of wm
    return;
  }

  if( screenUsage() ) {
    progress+=stepsize;
    //  Prog->setText(QString("progress: %1").arg(progress));
    int rot=(progress) % 24;  //we are rotating in steps of 15 degrees.

    if (lastRot!=rot) {
      _glider.fill(Glider->backgroundColor());
      bitBlt(&_glider, 0, 0, &_gliders, rot*40, 0, 40, 40);

      Glider->setPixmap(_glider);

      lastRot=rot;
      show();
      qDebug("========= WaitScreen::slot_Progress() calls processEvents =========");
      QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
    }
  }
}
