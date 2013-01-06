/***********************************************************************
**
**   numberEditor.cpp
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c): 2012-2013 Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#include <climits>

#include <QtGui>

#include "mainwindow.h"
#include "numberInputPad.h"
#include "numberEditor.h"

/**
 * Constructor
 */
NumberEditor::NumberEditor( QWidget *parent,
                            QString number,
                            QString prefix,
                            QString suffix ) :
  QLabel( parent ),
  m_nip(0),
  m_prefix(prefix),
  m_number(number),
  m_suffix(suffix),
  m_title(tr("edit number")),
  m_decimalFlag(true),
  m_pmFlag(true),
  m_validator(0),
  m_inputMask(""),
  m_maxLength(32767),
  m_intMax(false, INT_MAX),
  m_intMin(false, INT_MIN),
  m_doubleMax(false,INT_MAX),
  m_doubleMin(false, INT_MIN)
{
  setObjectName("NumberEditor");

//  QPalette p = palette();
//  p.setColor( QPalette::Window, Qt::white );
//  setPalette(p);
//  setAutoFillBackground( true );

  setBackgroundRole( QPalette::Light );
  setAutoFillBackground( true );
  setAlignment(Qt::AlignCenter);
  setMargin(2);
  setFrameStyle(QFrame::StyledPanel | QFrame::Plain);

  QSizePolicy sp = sizePolicy();
  sp.setVerticalPolicy( QSizePolicy::Fixed );
  setSizePolicy( sp );

  setText();
}

NumberEditor::~NumberEditor()
{
}

void NumberEditor::showEvent( QShowEvent* event )
{
  QSize size = minimumSizeHint();
  setMinimumHeight( size.height() );
  setMaximumHeight( size.height() );

  QLabel::showEvent(event);
}

void NumberEditor::mousePressEvent( QMouseEvent* event )
{
  // Opens the number input pad after a mouse press.
  if( ! m_nip )
    {
      m_nip = new NumberInputPad( m_number, this );
      m_nip->setWindowTitle( m_title );
      m_nip->setDecimalVisible( m_decimalFlag );
      m_nip->setPmVisible( m_pmFlag );
      m_nip->setValidator( m_validator );
      m_nip->setMaxLength( m_maxLength );
      m_nip->setInputMask( m_inputMask );

      if( m_intMax.first ) m_nip->setIntMaximum( m_intMax.second );
      if( m_intMin.first ) m_nip->setIntMinimum( m_intMin.second );
      if( m_doubleMax.first ) m_nip->setDoubleMaximum( m_doubleMax.second );
      if( m_doubleMin.first ) m_nip->setDoubleMinimum( m_doubleMin.second );

      connect( m_nip, SIGNAL(numberEdited(const QString&) ),
               SLOT(slot_NumberEdited(const QString&)) );
      connect( m_nip, SIGNAL(valueChanged(int) ),
               this, SIGNAL(valueChanged(int)) );
      connect( m_nip, SIGNAL(valueChanged(double) ),
               this, SIGNAL(valueChanged(double)) );

      m_nip->show();

#ifdef ANDROID

      // Sets the window's background to another color.
      m_nip->setAutoFillBackground( true );
      m_nip->setBackgroundRole( QPalette::Window );
      m_nip->setPalette( QPalette( QColor( Qt::lightGray ) ) );

      QSize ms = m_nip->minimumSizeHint();
      ms += QSize(10, 10);

      // A dialog is not centered over the parent and not limited in
      // its size under Android. Therefore this must be done by our self.
      m_nip->setGeometry( (MainWindow::mainWindow()->width() - ms.width()) / 2,
                          (MainWindow::mainWindow()->height() - ms.height()) / 2,
                           ms.width(), ms.height() );

      // That do not work under Android.
      m_nip->getEditor()->setFocus();

#endif

    }

  event->accept();
}

void NumberEditor::slot_NumberEdited( const QString& number )
{
  m_nip = 0;
  m_number = number;
  setText();
  emit numberEdited( number );
}
