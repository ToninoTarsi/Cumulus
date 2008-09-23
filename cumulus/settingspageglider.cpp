/***********************************************************************
**
**   settingspageglider.cpp
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2002 by Andr� Somers, 2008 Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

/**
 * This widget provides an interface to add, edit and delete gliders
 * from the glider list.
 *
 * @author Andr� Somers
 */

#include <QPushButton>
#include <QMessageBox>
#include <QSettings>
#include <QPixmap>

#include "generalconfig.h"
#include "settingspageglider.h"
#include "settingspagegliderdata.h"

SettingsPageGlider::SettingsPageGlider(QWidget *parent) : QWidget(parent)
{
  setObjectName("SettingsPageGlider");

  resize(parent->size());
  QBoxLayout *topLayout = new QVBoxLayout(this);
  topLayout->setSpacing(5);
  QBoxLayout *editrow=new QHBoxLayout;
  topLayout->addLayout(editrow);
  editrow->setSpacing(0);
  editrow->addStretch(10);

  QPushButton * cmdNew = new QPushButton(this);
  cmdNew->setIcon( QIcon(GeneralConfig::instance()->loadPixmap("add.png")) );
  cmdNew->setIconSize( QSize(26,26) );
  editrow->addWidget(cmdNew,1);

  editrow->addSpacing(10);
  QPushButton * cmdEdit = new QPushButton(this);
  cmdEdit->setIcon( QIcon(GeneralConfig::instance()->loadPixmap("edit_new.png")) );
  cmdEdit->setIconSize( QSize(26,26) );
  editrow->addWidget(cmdEdit,1);

  editrow->addSpacing(10);
  QPushButton * cmdDel = new QPushButton(this);
  cmdDel->setIcon( QIcon(GeneralConfig::instance()->loadPixmap( "delete.png" )) );
  cmdDel->setIconSize( QSize(26,26) );
  editrow->addWidget(cmdDel,1);

  list = new GliderListWidget(this);
  topLayout->addWidget(list, 10);

  connect(cmdNew,  SIGNAL(clicked()), this, SLOT(slot_new()));
  connect(cmdEdit, SIGNAL(clicked()), this, SLOT(slot_edit()));
  connect(cmdDel,  SIGNAL(clicked()), this, SLOT(slot_delete()));
}


SettingsPageGlider::~SettingsPageGlider()
{
  // qDebug("SettingsPageGlider::~SettingsPageGlider() is called");
}


void SettingsPageGlider::showEvent(QShowEvent *)
{
  list->setFocus();
}


/** Called when a new glider needs to be made. */
void SettingsPageGlider::slot_new()
{
  SettingsPageGliderData *dlg = new SettingsPageGliderData(this, 0);
  connect(dlg, SIGNAL(newGlider(Glider*)), list, SLOT(slot_Added(Glider *)));

  dlg->show();
}


/** Called when the selected glider needs must be opened in the editor */
void SettingsPageGlider::slot_edit()
{
  Glider *selectedGlider = list->getSelectedGlider();

  if( selectedGlider == 0 )
    {
      // @AP: no glider is selected, ignore the request
      return;
    }

  SettingsPageGliderData *dlg = new SettingsPageGliderData(this, selectedGlider );
  connect(dlg, SIGNAL(editedGlider(Glider *)), list, SLOT(slot_Edited(Glider *)));

  dlg->show();
}


/** Called when the selected glider should be deleted from the catalog */
void SettingsPageGlider::slot_delete()
{
  Glider *glider = list->getSelectedGlider();

  if (! glider) {
    return;
  }

  int answer= QMessageBox::warning(this,tr("Delete?"),tr("Delete selected glider?"),
                                   QMessageBox::Ok,
                                   QMessageBox::Cancel | QMessageBox::Escape | QMessageBox::Default);

  if( answer == QMessageBox::Ok ) {
    list->slot_Deleted(glider);
  }
}

/** Called to fill the tree list */
void SettingsPageGlider::slot_load()
{
  list->fillList();
}


void SettingsPageGlider::slot_save()
{
  list->save();
}


/* Called to ask is confirmation on the close is needed. */
void SettingsPageGlider::slot_query_close(bool& warn, QStringList& warnings)
{
  /* set warn to 'true' if the data has changed. Note that we can NOT just set warn equal to
    _changed, because that way we might erase a warning flag set by another page! */
  if (list->has_changed()) {
    warn = true;
    warnings.append(tr("the glider list"));
  }
}

