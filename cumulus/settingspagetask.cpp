/***********************************************************************
**
**   settingspagetask.cpp
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2007-2010 Axel Pauli, axel@kflog.org
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#include <QtGui>

#include "settingspagetask.h"
#include "generalconfig.h"
#include "flighttask.h"
#include "mapcontents.h"

extern MapContents  *_globalMapContents;

/**
 * @short Configuration settings for start, turn and end points of a
 * task
 *
 * @author Axel Pauli
 */

// Constructor of class
SettingsPageTask::SettingsPageTask( QWidget *parent) :
  QWidget( parent ),
  loadedCylinderRadius(0),
  loadedInnerSectorRadius(0),
  loadedOuterSectorRadius(0)
{
  setObjectName("SettingsPageTask");

  GeneralConfig *conf = GeneralConfig::instance();

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  int row = 0;

  QGridLayout *topLayout = new QGridLayout( this );
  topLayout->setMargin(3);
  topLayout->setSpacing(3);

  QGroupBox *tsBox = new QGroupBox( tr("TP Scheme"), this );
  topLayout->addWidget( tsBox, row, 0 );

  QRadioButton* cylinder = new QRadioButton( tr("Cylinder"), this );
  QRadioButton* sector   = new QRadioButton( tr("Sector"), this );

  csScheme = new QButtonGroup(this);
  csScheme->addButton( cylinder, 0 );
  csScheme->addButton( sector, 1 );

  QVBoxLayout *vbox = new QVBoxLayout;
  vbox->addWidget( cylinder );
  vbox->addWidget( sector );
  vbox->addStretch(1);
  tsBox->setLayout(vbox);

  cylinder->setEnabled(true);
  cylinder->setChecked(false);
  sector->setEnabled(true);
  sector->setChecked(false);

  // set active button as selected
  selectedCSScheme = (int) conf->getActiveCSTaskScheme();

  if( csScheme->button(selectedCSScheme) )
    {
      csScheme->button(selectedCSScheme)->setChecked(true);
    }

  //---------------------------------------------------------------

  QGroupBox *ssBox = new QGroupBox( tr("Switch Scheme"), this );
  topLayout->addWidget( ssBox, row, 1 );

  ntScheme = new QButtonGroup(this);

  QRadioButton* nearest  = new QRadioButton( tr("Minimum"), this );
  QRadioButton* touched  = new QRadioButton( tr("Touched"), this );

  ntScheme->addButton( nearest, 0 );
  ntScheme->addButton( touched, 1 );

  vbox = new QVBoxLayout;
  vbox->addWidget( nearest );
  vbox->addWidget( touched );
  vbox->addStretch(1);
  ssBox->setLayout(vbox);

  nearest->setEnabled(true);
  touched->setEnabled(true);
  touched->setChecked(true);

  // set active button as selected
  selectedNTScheme = (int) conf->getActiveNTTaskScheme();

  if( ntScheme->button(selectedNTScheme) )
    {
      ntScheme->button(selectedNTScheme)->setEnabled(true);
    }

  //--------------------------------------------------------------
  // as next shape group is added
  shapeGroup = new QGroupBox( tr("Shape"), this );
  topLayout->addWidget( shapeGroup, row, 2 );

  vbox = new QVBoxLayout;
  drawShape = new QCheckBox( tr("Draw Shape"), this );
  fillShape = new QCheckBox( tr("Fill Shape"), this );

  vbox->addWidget( drawShape );
  vbox->addWidget( fillShape );
  vbox->addStretch(1);

  shapeGroup->setLayout(vbox);

  drawShape->setChecked( conf->getTaskDrawShape() );
  fillShape->setChecked( conf->getTaskFillShape() );

  row++;

  //--------------------------------------------------------------
  // as next cylinder group is added
  cylinderGroup = new QGroupBox( tr("Cylinder"), this );
  topLayout->addWidget( cylinderGroup, row++, 0, 1, 3 );

  QHBoxLayout *hbox = new QHBoxLayout;

  QLabel *lbl = new QLabel( tr("Radius:"), this );
  hbox->addWidget( lbl );

  // get current distance unit. This unit must be considered during
  // storage. The internal storage is always in meters.
  distUnit = Distance::getUnit();

  const char *unit = "";

  // Input accepts different units
  if( distUnit == Distance::kilometers )
    {
      unit = " km";
    }
  else if( distUnit == Distance::miles )
    {
      unit = " ml";
    }
  else // if( distUnit == Distance::nautmiles )
    {
      unit = " nm";
    }

  cylinderRadius = new QDoubleSpinBox( this );
  cylinderRadius->setRange(0.1, 1000.0);
  cylinderRadius->setSingleStep(0.1);
  cylinderRadius->setButtonSymbols(QSpinBox::PlusMinus);
  cylinderRadius->setSuffix( unit );
  hbox->addWidget( cylinderRadius );
  hbox->addStretch(10);

  cylinderGroup->setLayout(hbox);

  //--------------------------------------------------------------
  // as next sector group is added
  sectorGroup = new QGroupBox( tr("Sector"), this );
  topLayout->addWidget( sectorGroup, row++, 0, 1, 3 );

  QGridLayout *sectorLayout = new QGridLayout( sectorGroup );
  sectorLayout->setMargin(10);
  sectorLayout->setSpacing(3);

  int row1 = 0;

  lbl = new QLabel( tr("Inner Radius:"), sectorGroup );
  sectorLayout->addWidget( lbl, row1, 0 );

  innerSectorRadius = new QDoubleSpinBox( sectorGroup );
  innerSectorRadius->setRange(0.0, 10.0);
  innerSectorRadius->setSingleStep(0.1);
  innerSectorRadius->setButtonSymbols(QSpinBox::PlusMinus);
  innerSectorRadius->setSuffix( unit );
  sectorLayout->addWidget( innerSectorRadius, row1, 1 );

  row1++;
  lbl = new QLabel( tr("Outer Radius:"), sectorGroup );
  sectorLayout->addWidget( lbl, row1, 0 );
  outerSectorRadius = new QDoubleSpinBox( sectorGroup );
  outerSectorRadius->setRange(0.1, 1000.0);
  outerSectorRadius->setSingleStep(0.1);
  outerSectorRadius->setButtonSymbols(QSpinBox::PlusMinus);
  outerSectorRadius->setSuffix( unit );
  sectorLayout->addWidget( outerSectorRadius, row1, 1 );

  row1++;
  lbl = new QLabel( tr("Angle:"), sectorGroup );
  sectorLayout->addWidget( lbl, row1, 0 );
  sectorAngle = new QSpinBox( sectorGroup );
  sectorAngle->setRange( 90, 180 );
  sectorAngle->setSingleStep( 5 );
  sectorAngle->setButtonSymbols(QSpinBox::PlusMinus);
  sectorAngle->setSuffix( QString(Qt::Key_degree) );
  sectorAngle->setWrapping( true );
  sectorAngle->setValue( conf->getTaskSectorAngle() );
  sectorLayout->addWidget( sectorAngle, row1, 1 );

  sectorLayout->setColumnStretch(2, 10);

  //--------------------------------------------------------------
  // as next course line group is added
  clGroup = new QGroupBox( tr("Course Line"), this );

  QGridLayout *clLayout = new QGridLayout( clGroup );
  clLayout->setMargin(10);
  clLayout->setSpacing(3);
  row1 = 0;

  lbl = new QLabel( tr("Width:"), clGroup );
  clLayout->addWidget( lbl, row1, 0 );

  clWidth = new QSpinBox( clGroup );
  clWidth->setRange( 3, 10 );
  clWidth->setSingleStep( 1 );
  clWidth->setButtonSymbols( QSpinBox::PlusMinus );
  clLayout->addWidget( clWidth, row1, 1 );

  row1++;
  lbl = new QLabel( tr("Color:"), clGroup );
  clLayout->addWidget( lbl, row1, 0 );

  clColorButton = new QPushButton( clGroup );
  clLayout->addWidget( clColorButton, row1, 1 );

  // on click the color chooser dialog will be opened
  connect( clColorButton, SIGNAL(clicked()), this, SLOT(slot_editClColor()) );

  //--------------------------------------------------------------
  hbox = new QHBoxLayout;
  hbox->addWidget( cylinderGroup );
  hbox->addWidget( sectorGroup );
  hbox->addWidget( clGroup );
  hbox->addStretch( 10 );

  topLayout->addLayout( hbox, row++, 0, 1, 3 );
  topLayout->setRowStretch( row, 10 );
  //--------------------------------------------------------------

  connect( csScheme, SIGNAL(buttonClicked(int)), this, SLOT(slot_buttonPressedCS(int)) );
  connect( ntScheme, SIGNAL(buttonClicked(int)), this, SLOT(slot_buttonPressedNT(int)) );
  connect( outerSectorRadius, SIGNAL(valueChanged(double)), this, SLOT(slot_outerSBChanged(double)) );
}

// Destructor of class
SettingsPageTask::~SettingsPageTask()
{
}

// value of outer spin box changed
void SettingsPageTask::slot_outerSBChanged( double /* value */ )
{
  // set max range of inner radius spin box to current value of this box
  innerSectorRadius->setMaximum( outerSectorRadius->value() );

  if( innerSectorRadius->value() > outerSectorRadius->value() )
    {
      // inner spin box value must be less equal outer spin box value
      innerSectorRadius->setValue( outerSectorRadius->value() );
    }
}

// Set the passed scheme widget as active and the other one to
// inactive
void SettingsPageTask::slot_buttonPressedCS( int newScheme )
{
  selectedCSScheme = newScheme;

  if( newScheme == GeneralConfig::Sector )
    {
      sectorGroup->setVisible(true);
      cylinderGroup->setVisible(false);
    }
  else
    {
      sectorGroup->setVisible(false);
      cylinderGroup->setVisible(true);
    }
}

void SettingsPageTask::slot_buttonPressedNT( int newScheme )
{
  selectedNTScheme = newScheme;
}

/** Opens the color chooser dialog for the course line */
void SettingsPageTask::slot_editClColor()
{
  // Open color chooser dialog to edit course line color
  QString title = tr("Course Line Color");
  QColor newColor = QColorDialog::getColor( clColor, this, title );

  if( newColor.isValid() && clColor != newColor )
    {
      clColor = newColor;
      // determine pixmap size to be used for icons in dependency of the used font
      int size = font().pointSize() - 2;
      QSize pixmapSize = QSize( size, size );
      QPixmap pixmap(pixmapSize);
      pixmap.fill( newColor );
      clColorButton->setIcon( QIcon(pixmap) );
    }
}

void SettingsPageTask::slot_load()
{
  GeneralConfig *conf = GeneralConfig::instance();

  slot_buttonPressedCS( (int) conf->getActiveCSTaskScheme() );

  // @AP: radius is always fetched in meters.
  Distance cRadius = conf->getTaskCylinderRadius();
  Distance iRadius = conf->getTaskSectorInnerRadius();
  Distance oRadius = conf->getTaskSectorOuterRadius();

  if( distUnit == Distance::kilometers ) // user gets meters
    {
      cylinderRadius->setValue( cRadius.getKilometers() );
      innerSectorRadius->setValue( iRadius.getKilometers() );
      outerSectorRadius->setValue( oRadius.getKilometers() );
    }
  else if( distUnit == Distance::miles ) // user gets miles
    {
      cylinderRadius->setValue( cRadius.getMiles() );
      innerSectorRadius->setValue( iRadius.getMiles() );
      outerSectorRadius->setValue( oRadius.getMiles() );
    }
  else // ( distUnit == Distance::nautmiles )
    {
      cylinderRadius->setValue( cRadius.getNautMiles() );
      innerSectorRadius->setValue( iRadius.getNautMiles() );
      outerSectorRadius->setValue( oRadius.getNautMiles() );
    }

  // Save loaded integer values of spin boxes
  loadedCylinderRadius = cylinderRadius->value();
  loadedInnerSectorRadius = innerSectorRadius->value();
  loadedOuterSectorRadius = outerSectorRadius->value();

  drawShape->setChecked( conf->getTaskDrawShape() );
  fillShape->setChecked( conf->getTaskFillShape() );

  // load task course line color
  clColor = conf->getTaskCourseLineColor();
  selectedClColor = clColor;

  // determine pixmap size to be used for icons in dependency of the used font
  int size = font().pointSize() - 2;
  QSize pixmapSize = QSize( size, size );
  QPixmap pixmap(pixmapSize);
  pixmap.fill( clColor );
  clColorButton->setIcon( QIcon(pixmap) );

  seletedClWidth = conf->getTaskCourseLineWidth();
  clWidth->setValue( seletedClWidth );
}

void SettingsPageTask::slot_save()
{
  GeneralConfig *conf = GeneralConfig::instance();

  conf->setActiveCSTaskScheme( (GeneralConfig::ActiveCSTaskScheme) selectedCSScheme );
  conf->setActiveNTTaskScheme( (GeneralConfig::ActiveNTTaskScheme) selectedNTScheme );

  // @AP: radius is always saved in meters. Save is done only after a
  // real change to avoid rounding errors.
  Distance cRadius;
  Distance iRadius;
  Distance oRadius;

  if( loadedCylinderRadius != cylinderRadius->value() )
    {
      if( distUnit == Distance::kilometers ) // user gets kilometers
        {
          cRadius.setKilometers( cylinderRadius->value() );
        }
      else if( distUnit == Distance::miles ) // user gets miles
        {
          cRadius.setMiles( cylinderRadius->value() );
        }
      else // ( distUnit == Distance::nautmiles )
        {
          cRadius.setNautMiles( cylinderRadius->value() );
        }

      conf->setTaskCylinderRadius( cRadius );
    }

  if( loadedInnerSectorRadius != innerSectorRadius->value() )
    {
      if( distUnit == Distance::kilometers ) // user gets kilometers
        {
          iRadius.setKilometers( innerSectorRadius->value() );
        }
      else if( distUnit == Distance::miles ) // user gets miles
        {
          iRadius.setMiles( innerSectorRadius->value() );
        }
      else // ( distUnit == Distance::nautmiles )
        {
          iRadius.setNautMiles( innerSectorRadius->value() );
        }

      conf->setTaskSectorInnerRadius( iRadius );
    }

  if( loadedOuterSectorRadius != outerSectorRadius->value() )
    {
      if( distUnit == Distance::kilometers ) // user gets kilometers
        {
          oRadius.setKilometers( outerSectorRadius->value() );
        }
      else if( distUnit == Distance::miles ) // user gets miles
        {
          oRadius.setMiles( outerSectorRadius->value() );
        }
      else // ( distUnit == Distance::nautmiles )
        {
          oRadius.setNautMiles( outerSectorRadius->value() );
        }

      conf->setTaskSectorOuterRadius( oRadius );
    }

  conf->setTaskSectorAngle( sectorAngle->value() );

  conf->setTaskDrawShape( drawShape->isChecked() );
  conf->setTaskFillShape( fillShape->isChecked() );

  // If a flight task is set, we must update the sector angles in it
   FlightTask *task = _globalMapContents->getCurrentTask();

   if( task != 0 )
     {
       task->updateTask();
     }

   // save task course line items
   if( selectedClColor != clColor )
     {
       conf->setTaskCourseLineColor( clColor );
     }

   if( seletedClWidth != clWidth->value() )
     {
       conf->setTaskCourseLineWidth( clWidth->value() );
     }
}
