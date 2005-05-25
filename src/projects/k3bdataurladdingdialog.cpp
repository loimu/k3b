/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdataurladdingdialog.h"

#include <qtimer.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qdir.h>
#include <qfileinfo.h>

#include <k3bbusywidget.h>
#include <k3bdatadoc.h>
#include <k3bdiritem.h>
#include <k3bcore.h>
#include <k3bfileitem.h>
#include <k3bmultichoicedialog.h>
#include <k3bvalidators.h>

#include <klocale.h>
#include <kurl.h>
#include <kconfig.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kglobal.h>


K3bDataUrlAddingDialog::K3bDataUrlAddingDialog( QWidget* parent, const char* name )
  : KDialogBase( Plain,
		 0,
		 parent,
		 name,
		 true,
		 i18n("Please be patient..."),
		 Cancel,
		 Cancel,
		 true ),
    m_bExistingItemsReplaceAll(false),
    m_bExistingItemsIgnoreAll(false)
{
  QWidget* page = plainPage();
  QGridLayout* grid = new QGridLayout( page );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );

//   QLabel* pixLabel = new QLabel( page );
//   pixLabel->setPixmap( KGlobal::iconLoader()->loadIcon( "editcopy", KIcon::NoGroup, 32 ) );
//   pixLabel->setScaledContents( false );

  m_infoLabel = new QLabel( page );
  m_busyWidget = new K3bBusyWidget( page );

  //  grid->addMultiCellWidget( pixLabel, 0, 1, 0, 0 );
  grid->addWidget( m_infoLabel, 0, 0 );
  grid->addWidget( m_busyWidget, 1, 0 );
}


K3bDataUrlAddingDialog::~K3bDataUrlAddingDialog()
{
}


int K3bDataUrlAddingDialog::addUrls( const KURL::List& urls, 
				     K3bDirItem* dir,
				     QWidget* parent )
{
  K3bDataUrlAddingDialog dlg( parent );
  dlg.m_infoLabel->setText( i18n("Adding files to project \"%1\"...").arg(dir->doc()->URL().fileName()) );
  for( KURL::List::ConstIterator it = urls.begin(); it != urls.end(); ++it )
    dlg.m_urlQueue.append( qMakePair( *it, dir ) );

  dlg.slotAddUrls();
  int ret = QDialog::Accepted;
  if( !dlg.m_urlQueue.isEmpty() ) {
    dlg.m_busyWidget->showBusy(true);
    ret = dlg.exec();
  }

  QString message;
  if( !dlg.m_unreadableFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("Insufficient permissions to read the following files") )
      .arg( dlg.m_unreadableFiles.join( "<br>" ) );
  if( !dlg.m_notFoundFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("Unable to find the following files") )
      .arg( dlg.m_notFoundFiles.join( "<br>" ) );
  if( !dlg.m_nonLocalFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("No non-local files supported") )
      .arg( dlg.m_unreadableFiles.join( "<br>" ) );

  if( !message.isEmpty() )
    KMessageBox::detailedSorry( parent, i18n("Could not add all files."), message );

  return ret;
}


void K3bDataUrlAddingDialog::slotAddUrls()
{
  // add next url
  KURL url = m_urlQueue.first().first;
  K3bDirItem* dir = m_urlQueue.first().second;
  m_urlQueue.remove( m_urlQueue.begin() );

  bool valid = true;
  QFileInfo f( url.path() );
  if( !url.isLocalFile() ) {
    valid = false;
    m_nonLocalFiles.append( url.path() );
  }

  // QFileInfo::exists and QFileInfo::isReadable return false for broken symlinks :(
  // and symlinks are always readable and can always be added to a project
  else if( !f.isSymLink() ) {
    if( !f.isReadable() ) {
      valid = false;
      m_unreadableFiles.append( url.path() );
    }
    if( !f.exists() ) {
      valid = false;
      m_notFoundFiles.append( url.path() );
    }
  }

  QString newName = url.fileName();


  K3bDirItem* newDirItem = 0;
  if( valid ) {
    if( K3bDataItem* oldItem = dir->find( newName ) ) {
      if( oldItem->isDir() && f.isDir() )
	newDirItem = dynamic_cast<K3bDirItem*>(oldItem);
      else if( oldItem->isFromOldSession() ) {
	// we cannot replace files in the old session with dirs and vice versa (I think)
	if( f.isDir() != oldItem->isDir() ) {
	  if( !getNewName( newName, dir, newName ) )
	    valid = false;
	}
	// else: Ok, files handled in K3bFileItem constructor and dirs handled above
      }
      else if( m_bExistingItemsIgnoreAll )
	valid = false;
      else if( m_bExistingItemsReplaceAll )
	delete oldItem;
      else {
	// TODO: use KGuiItems to add ToolTips
	switch( K3bMultiChoiceDialog::choose( i18n("File already exists"),
					      i18n("%1 already exists.").arg(newName),
					      this,
					      0,
					      5,
					      i18n("Replace"),
					      i18n("Replace All"),
					      i18n("Ignore"),
					      i18n("Ignore All"),
					      i18n("Rename") ) ) {
	case 1: // replace
	  delete oldItem;
	  break;
	case 2: // replace all
	  delete oldItem;
	  m_bExistingItemsReplaceAll = true;
	  break;
	case 3: // ignore
	  valid = false;
	  break;
	case 4: // ignore all
	  m_bExistingItemsIgnoreAll = true;
	  valid = false;
	  break;
	case 5: // rename
	  if( !getNewName( newName, dir, newName ) )
	    valid = false;
	  break;
	}
      }
    }
  }

  if( valid ) {
    if( f.isDir() && !f.isSymLink() ) {
      if( !newDirItem )
	newDirItem = new K3bDirItem( newName , dir->doc(), dir );

      // TODO: ask the user if he wants the hidden files to be added and remove the option from the config dialog
      KConfig* c = k3bcore->config();
      c->setGroup( "Data project settings" );
      
      int dirFilter = QDir::All;
      if( c->readBoolEntry( "Add hidden files", true ) )
	dirFilter |= QDir::Hidden;
      if( c->readBoolEntry( "Add system files", false ) )
	dirFilter |= QDir::System;
	
      QStringList dlist = QDir( f.absFilePath() ).entryList( dirFilter );
      dlist.remove(".");
      dlist.remove("..");

      for( QStringList::Iterator it = dlist.begin(); it != dlist.end(); ++it ) {
	m_urlQueue.append( qMakePair( KURL::fromPathOrURL(f.absFilePath() + "/" + *it), newDirItem ) );
      }
    }
    else {
      (void)new K3bFileItem( url.path(), dir->doc(), dir, newName );
    }
  }

  if( m_urlQueue.isEmpty() )
    accept();
  else
    QTimer::singleShot( 0, this, SLOT(slotAddUrls()) );
}


bool K3bDataUrlAddingDialog::getNewName( const QString& oldName, K3bDirItem* dir, QString& newName )
{
  bool ok = true;
  newName = oldName;
  QValidator* validator = K3bValidators::iso9660Validator( false, this );
  do {
    newName = KInputDialog::getText( i18n("Enter New Filename"),
				     i18n("A file with that name already exists. Please enter a new name:"),
				     newName, &ok, this, "renamedialog", validator );
    
  } while( ok && dir->find( newName ) );
  
  delete validator;

  return ok;
}

#include "k3bdataurladdingdialog.moc"
