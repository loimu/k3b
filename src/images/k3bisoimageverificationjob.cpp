/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bisoimageverificationjob.h"

#include <k3bdevice.h>
#include <k3bdevicehandler.h>
#include <k3bmd5job.h>

#include <kdebug.h>
#include <klocale.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>

#include <qcstring.h>
#include <qapplication.h>


class K3bIsoImageVerificationJob::Private
{
public:
  Private()
    : md5Job(0),
      device(0) {
  }

  bool canceled;
  bool imageMd5SumCalculated;
  K3bMd5Job* md5Job;
  K3bCdDevice::CdDevice* device;
  QString imageFileName;

  KIO::filesize_t imageSize;

  QCString imageMd5Sum;
};


K3bIsoImageVerificationJob::K3bIsoImageVerificationJob( QObject* parent, const char* name )
  : K3bJob( parent, name )
{
  d = new Private();

  d->md5Job = new K3bMd5Job( this );
  connect( d->md5Job, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( d->md5Job, SIGNAL(percent(int)), this, SLOT(slotMd5JobProgress(int)) );
  connect( d->md5Job, SIGNAL(finished(bool)), this, SLOT(slotMd5JobFinished(bool)) );
}


K3bIsoImageVerificationJob::~K3bIsoImageVerificationJob()
{
  delete d;
}


void K3bIsoImageVerificationJob::cancel()
{
  d->canceled = true;
  if( d->md5Job )
    d->md5Job->cancel();
}


void K3bIsoImageVerificationJob::setDevice( K3bCdDevice::CdDevice* dev )
{
  d->device = dev;
}


void K3bIsoImageVerificationJob::setImageFileName( const QString& f )
{
  d->imageFileName = f;
}


void K3bIsoImageVerificationJob::start()
{
  emit started();

  d->canceled = false;
  d->imageMd5SumCalculated = false;

  // first we need to reload and mount the device
  emit newTask( i18n("Reloading the media") );

  connect( K3bCdDevice::reload( d->device ), SIGNAL(finished(bool)),
	     this, SLOT(slotMediaReloaded(bool)) );
}


void K3bIsoImageVerificationJob::slotMediaReloaded( bool success )
{
  if( !success )
    KMessageBox::information( qApp->activeWindow(), i18n("Please reload the medium and press 'ok'"),
			      i18n("Unable to close the tray") );

  emit newTask( i18n("Calculating the image's md5sum") );
  
  // start it
  d->md5Job->setFile( d->imageFileName );
  d->md5Job->start();
}


void K3bIsoImageVerificationJob::slotMd5JobFinished( bool success )
{
  if( d->canceled ) {
    finishVerification(false);
  }

  if( success ) {

    if( d->imageMd5SumCalculated ) {
      // compare the two sums
      if( d->imageMd5Sum != d->md5Job->hexDigest() ) {
	emit infoMessage( i18n("The written data differs."), ERROR );
	finishVerification(false);
      }
      else {
	emit infoMessage( i18n("The written image seems binary equal."), SUCCESS );
	finishVerification(true);
      }
    }
    else {
      
      d->imageMd5Sum = d->md5Job->hexDigest();
      d->imageMd5SumCalculated = true;

      //
      // now we need to calculate the md5sum of the written image
      // since it is possible that the image has been padded while writing we need
      // the image filesize to make sure we do not compare too much data and thus get
      // a wrong result
      //
      KIO::UDSEntry entry;
      if( !KIO::NetAccess::stat( KURL::fromPathOrURL(d->imageFileName), entry ) ) {
	emit infoMessage( i18n("Unable to read file %1.").arg(d->imageFileName), ERROR );
	finishVerification(false);
      }
      else {
	d->imageSize = (KIO::filesize_t)-1;
	for( KIO::UDSEntry::ConstIterator it = entry.begin(); it != entry.end(); it++ )
	  if( (*it).m_uds == KIO::UDS_SIZE )
	    d->imageSize = (*it).m_long;

	if( d->imageSize == (KIO::filesize_t)-1 ) {
	  emit infoMessage( i18n("Unable to determine size of file %1.").arg(d->imageFileName), ERROR );
	  finishVerification(false);
	}
	else if( d->device->open() < 0 ) {
	  emit infoMessage( i18n("Unable to open device %1.").arg(d->device->blockDeviceName()), ERROR );
	  finishVerification(false);
	}
	else {
	  // start the written data check
	  emit newTask( i18n("Calculating the written data's md5sum") );
	  d->md5Job->setFd( d->device->open() );
	  d->md5Job->setMaxReadSize( d->imageSize );
	  d->md5Job->start();
	}
      }
    }
  }
  else {
    // The md5job emitted an error message. So there is no need to do this again
    finishVerification(false);
  }
}


void K3bIsoImageVerificationJob::slotMd5JobProgress( int p )
{
  if( d->imageMd5SumCalculated )
    emit percent( 50 + p/2 );
  else
    emit percent( p/2 );
  emit subPercent( p );
}


void K3bIsoImageVerificationJob::finishVerification( bool success )
{
  // close the device
  if( d->device )
    d->device->close();

  if( d->canceled )
    emit canceled();

  emit finished(success);
}

#include "k3bisoimageverificationjob.moc"
