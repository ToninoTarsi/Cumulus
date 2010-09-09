/***********************************************************************
**
**   gpsstatusdialog.cpp
**
**   This file is part of Cumulus
**
************************************************************************
**
**   Copyright (c): 2003      by André Somers
**                  2008-2010 by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#include <cmath>

#include <QtGui>

#include "gpsstatusdialog.h"
#include "gpsnmea.h"

GpsStatusDialog::GpsStatusDialog(QWidget * parent) :
  QWidget( parent, Qt::Window ),
  showNmeaData( true )
{
  setWindowTitle(tr("GPS Status"));
  setWindowModality( Qt::WindowModal );
  setAttribute(Qt::WA_DeleteOnClose);

  if( parent )
    {
      resize( parent->size() );
      move( parent->pos() );
    }

  elevAziDisplay = new GpsElevationAzimuthDisplay(this);
  snrDisplay     = new GpsSnrDisplay(this);

  nmeaBox = new QTextEdit(this);
  nmeaBox->setObjectName("NmeaBox");
  nmeaBox->setReadOnly(true);
  nmeaBox->document()->setMaximumBlockCount(250);
  nmeaBox->setLineWrapMode(QTextEdit::NoWrap);
  QFont f = font();
  f.setPixelSize(14);
  nmeaBox->setFont(f);

  startStop = new QPushButton( tr("Stop"), this );
  save      = new QPushButton( tr("Save"), this );

  QPushButton* close = new QPushButton( tr("Close"), this );

  QVBoxLayout* buttonBox = new QVBoxLayout;
  buttonBox->addStretch( 5 );
  buttonBox->addWidget( startStop );
  buttonBox->addSpacing( 10 );
  buttonBox->addWidget( save );
  buttonBox->addSpacing( 10 );
  buttonBox->addWidget( close );
  buttonBox->addStretch( 5 );

  QHBoxLayout* hBox = new QHBoxLayout;
  hBox->addWidget(elevAziDisplay, 1);
  hBox->addWidget(snrDisplay, 2);
  hBox->addLayout( buttonBox );

  QVBoxLayout* topLayout = new QVBoxLayout( this );
  topLayout->addLayout( hBox );
  topLayout->addWidget( nmeaBox );

  connect( GpsNmea::gps, SIGNAL(newSentence(const QString&)),
           this, SLOT(slot_Sentence(const QString&)) );
  connect( GpsNmea::gps, SIGNAL(newSatInViewInfo(QList<SIVInfo>&)),
           this, SLOT(slot_SIV(QList<SIVInfo>&)) );

  connect( startStop, SIGNAL(clicked()), this, SLOT(slot_toggleStartStop()) );

  connect( save, SIGNAL(clicked()), this, SLOT(slot_SaveNmeaData()) );

  connect( close, SIGNAL(clicked()), this, SLOT(close()) );
}

GpsStatusDialog::~GpsStatusDialog()
{
}

void GpsStatusDialog::slot_SIV( QList<SIVInfo>& siv )
{
  //qDebug("received new sivi info signal");
  elevAziDisplay->setSatInfo( siv );
  snrDisplay->setSatInfo( siv );
}


void GpsStatusDialog::slot_Sentence(const QString& sentence)
{
  if( showNmeaData )
    {
      nmeaBox->insertPlainText(sentence.trimmed().append("\n"));
    }
}

/**
 * Called if the start/stop button is pressed to start or stop NMEA display.
 */
void GpsStatusDialog::slot_toggleStartStop()
{
  showNmeaData = ! showNmeaData;

  if( showNmeaData )
    {
      startStop->setText( tr("Stop") );
    }
  else
    {
      startStop->setText( tr("Start") );
    }
}

/**
 * Called if the save button is pressed to save NMEA display content into a file.
 */
void GpsStatusDialog::slot_SaveNmeaData()
{
  QString text = nmeaBox->toPlainText();

  if( text.isEmpty() )
    {
      // nothing to save
      return;
    }

  bool ok;

  QString fileName = QInputDialog::getText( this, tr("Append to?"),
                                            tr("File name:"), QLineEdit::Normal,
                                            "nmea-stream.log", &ok);
  if( ok && ! fileName.isEmpty() )
    {
      if( ! fileName.startsWith( "/") )
        {
          // We will store the file in the user's home location, when it not
          // starts with a slash. Other cases as . or .. are ignored by us.
          QFileInfo fi( fileName );
          fileName = QDir::homePath() + "/" + fi.fileName();
        }

      QFile file( fileName );

      if( ! file.open( QIODevice::Append | QIODevice::Text ) )
        {
          QMessageBox::warning( this, tr("Save failed"), tr("Cannot open file!") );
          return;
        }

      file.write( text.toAscii() );
      file.close();
    }
}

void GpsStatusDialog::keyPressEvent(QKeyEvent *e)
{
  // close the dialog on key press
  // qDebug("GpsStatusDialog::keyPressEvent");
  switch(e->key())
    {
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Escape:
      close();
      break;
    }
}

/*************************************************************************************/

#define MARGIN 10

GpsElevationAzimuthDisplay::GpsElevationAzimuthDisplay(QWidget *parent) :
  QFrame(parent)
{
  setFrameStyle(StyledPanel | QFrame::Plain);
  setLineWidth(2);
  setMidLineWidth(2);

  background = new QPixmap();
}

GpsElevationAzimuthDisplay::~GpsElevationAzimuthDisplay()
{
  delete background;
}

void GpsElevationAzimuthDisplay::resizeEvent( QResizeEvent *event )
{
  QFrame::resizeEvent( event );

  width = contentsRect().width();
  height = contentsRect().height();

  if (width > height)
    {
      width = height;
    }

  width  -= ( MARGIN * 2 ); //keep a 10 pixel margin
  height -= ( MARGIN * 2 );
  center  = QPoint(contentsRect().width()/2, contentsRect().height()/2);

  delete background;
  background = new QPixmap( contentsRect().width(), contentsRect().height() );

  background->fill( palette().color(QPalette::Window) );

  QPainter p(background);
  p.setPen( QPen( Qt::black, 2, Qt::SolidLine ) );
  //outer circle
  p.drawEllipse(center.x() - ( width / 2 ), center.y() - ( width / 2 ), width, width);
  //inner circle. This is the 45 degrees. The diameter is cos(angle)
  p.drawEllipse(center.x() - ( width / 4 ), center.y() - ( width / 4 ),
                width / 2, width / 2 );

  p.drawLine(center.x(), center.y() - ( width / 2 ) - 5, center.x(), center.y() + ( width / 2 ) + 5);
  p.drawLine(center.x() - ( width / 2 ) -5 , center.y(), center.x() + ( width / 2 ) + 5, center.y());
}

void GpsElevationAzimuthDisplay::paintEvent( QPaintEvent *event )
{
  // Call paint method from QFrame otherwise the frame is not drawn.
  QFrame::paintEvent( event );

  QPainter p(this);
  QFont f = font();
  f.setPixelSize(12);
  p.setFont(f);

  // copy background to widget
  p.drawPixmap ( contentsRect().left(), contentsRect().top(), *background,
                 0, 0, background->width(), background->height() );

  // draw satellites
  if (sats.count())
    {
      for (int i=0; i < sats.count(); i++)
        {
          drawSat(&p, sats.at(i));
        }
    }
  else
    {
      p.fillRect( center.x()-23, center.y()-7, 46, 14, palette().color(QPalette::Window) );
      p.drawText(center.x()-23, center.y()-7, 46, 14, Qt::AlignCenter, tr("No Data"));
    }
}

void GpsElevationAzimuthDisplay::setSatInfo( QList<SIVInfo>& list )
{
  sats = list;
  // update();
  repaint();
}

void GpsElevationAzimuthDisplay::drawSat( QPainter *p, const SIVInfo& sivi )
{
  if (sivi.db < 0)
    {
      return;
    }

  double a = (double(( sivi.azimuth - 180 )/ -180.0) * M_PI);
  double r = (90 - sivi.elevation) / 90.0;
  int x = int(center.x() + (width / 2) * r * sin (a));
  int y = int(center.y() + (width / 2) * r * cos (a));
  //qDebug("%f", a);

  int R, G;
  int db = qMin(sivi.db * 2, 99);

  if (db < 50)
    {
      R=255;
      G=(255/50 * db);
    }
  else
    {
      R=255 - (255/50 * (db - 50));
      G=255;
    }

  QFont f = font();
  f.setPixelSize(12);
  p->setFont(f);

  p->setBrush(QColor(R,G,0));
  p->fillRect(x - 9, y - 7, 18 , 14 , p->brush());
  p->drawRect(x - 9, y - 7, 18 , 14 );
  p->drawText(x - 9, y - 5, 18 , 14 , Qt::AlignCenter, QString::number(sivi.id) );
}

/*************************************************************************************/

GpsSnrDisplay::GpsSnrDisplay(QWidget *parent) : QFrame(parent)
{
  setFrameStyle(StyledPanel | QFrame::Plain);
  setLineWidth(2);
  setMidLineWidth(2);

  background = new QPixmap();
  canvas = new QPixmap();
  mask = new QBitmap();
}

GpsSnrDisplay::~GpsSnrDisplay()
{
  delete background;
  delete canvas;
  delete mask;
}

void GpsSnrDisplay::resizeEvent( QResizeEvent *event )
{
  QFrame::resizeEvent( event );

  width  = contentsRect().width();
  height = contentsRect().height();
  center = QPoint(contentsRect().width()/2, contentsRect().height()/2);

  delete background;
  background = new QPixmap( width, height );
  background->fill();
  delete canvas;
  canvas = new QPixmap( width, height );

  delete mask;
  mask = new QBitmap( width, height );
  mask->fill();

  QPainter p(background);

  int R, G;
  double dbfactor = 100.0 / height;
  double db;

  for (int i=0;i<=height;i++)
    {
      db = i * dbfactor;
      if (db < 50)
        {
          R = 255;
          G = int( 255/50 * db );
        }
      else
        {
          R = int( 255 - (255/50 * (db - 50)) );
          G = 255;
        }

      p.setPen( QPen( QColor(R, G, 0), 1, Qt::SolidLine ) );
      p.drawLine(0, height-i, width, height-i);
    }
}

void GpsSnrDisplay::paintEvent( QPaintEvent *event )
{
  // Call paint method from QFrame otherwise the frame is not drawn.
  QFrame::paintEvent( event );

  QPainter p;
  p.begin(canvas);
  p.drawPixmap( 0, 0, *background, 0, 0, background->width(), background->height() );

  // draw satellites
  if( sats.count() )
    {
      QPainter pm(mask);
      pm.fillRect(0, 0, width, height, Qt::color0);

      for (int i=0; i < sats.count(); i++)
        {
          drawSat(&p, &pm, i, sats.count(), sats.at(i));
        }

      p.end();

      //copy canvas to widget, masked by the mask
      canvas->setMask(*mask);

      QPainter pw(this);
      pw.drawPixmap( 0, 0, *canvas, 0, 0, canvas->width(), canvas->height() );
    }
  else
    {
      QPainter pd(this);
      QFont f = font();
      f.setPixelSize(12);
      pd.setFont(f);
      pd.fillRect( center.x()-23, center.y()-7, 46, 14, palette().color(QPalette::Window) );
      pd.drawText(center.x()-23, center.y()-7, 46, 14, Qt::AlignCenter, tr("No Data"));
    }
}

void GpsSnrDisplay::setSatInfo(QList<SIVInfo>& list)
{
  sats = list;
  // update();
  repaint();
}

void GpsSnrDisplay::drawSat(QPainter * p, QPainter * pm, int i, int cnt, const SIVInfo& sivi)
{
  int bwidth = width / cnt;
  int left = bwidth * i + 2;
  int db = sivi.db * 2;
  db = qMin(db, 100);
  int bheight = qMax(int((double(height) / 99.0) * db), 14);
  //qDebug("id: %d, db: %d, bheight: %d (height: %d)", sivi->id, sivi->db, bheight, height);
  pm->fillRect(left, height, bwidth - 2, -bheight, Qt::color1);

  if (sivi.db < 0)
    {
      p->fillRect(left, height, bwidth - 2, -bheight, Qt::white);
    }

  QFont f = font();
  f.setPixelSize(12);
  p->setFont(f);
  p->setPen(Qt::black);
  p->drawRect(left, height, bwidth - 2, -bheight);
  p->drawText(left+1, height-13, bwidth-4, 12, Qt::AlignCenter, QString::number(sivi.id));
}
