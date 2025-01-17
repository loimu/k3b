// -*- c-basic-offset: 2 -*-
/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
   SPDX-FileCopyrightText: 2000-2002 David Faure <faure@kde.org>, Werner Trobin <trobin@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KoStore.h"
//#include "KoTarStore.h"
#include "KoZipStore.h"
//#include "KoDirectoryStore.h"
#ifdef QCA2
#include "KoEncryptedStore.h"
#endif

#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QIODevice>
#include <QTemporaryFile>
#include <QUrl>

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KJobWidgets>
#include <KMessageBox>

#define DefaultFormat KoStore::Zip

Q_LOGGING_CATEGORY(KOSTORE, "KoStore")

KoStore::Backend KoStore::determineBackend( QIODevice* dev )
{
    unsigned char buf[5];
    if ( dev->read( (char *)buf, 4 ) < 4 )
      return DefaultFormat; // will create a "bad" store (bad()==true)
    //if ( buf[0] == 0037 && buf[1] == 0213 ) // gzip -> tar.gz
      //return Tar;
    if ( buf[0] == 'P' && buf[1] == 'K' && buf[2] == 3 && buf[3] == 4 )
      return Zip;
    return DefaultFormat; // fallback
}

KoStore* KoStore::createStore( const QString& fileName, Mode mode, const QByteArray & appIdentification, Backend backend )
{
  bool automatic = false;
  if ( backend == Auto ) {
    automatic = true;
    if ( mode == KoStore::Write )
      backend = DefaultFormat;
    else
    {
      QFileInfo inf( fileName );
      if ( inf.isDir() )
        backend = Directory;
      else
      {
        QFile file( fileName );
        if ( file.open( QIODevice::ReadOnly ) )
          backend = determineBackend( &file );
        else
          backend = DefaultFormat; // will create a "bad" store (bad()==true)
      }
    }
  }
  switch ( backend )
  {
//  case Tar:
  //  return new KoTarStore( fileName, mode, appIdentification );
  case Zip:
#ifdef QCA2
    if( automatic && mode == Read ) {
        // When automatically detecting, this might as well be an encrypted file. We'll need to check anyway, so we'll just use the encrypted store.
        return new KoEncryptedStore( fileName, Read, appIdentification );
    }
#else
    Q_UNUSED( automatic )
#endif
    return new KoZipStore( fileName, mode, appIdentification );
//  case Directory:
//    return new KoDirectoryStore( fileName /* should be a dir name.... */, mode );
#ifdef QCA2
  case Encrypted:
    return new KoEncryptedStore( fileName, mode, appIdentification );
#endif
  default:
    qCWarning(KOSTORE) << "Unsupported backend requested for KoStore : " << backend;
    return 0L;
  }
}

KoStore* KoStore::createStore( QIODevice *device, Mode mode, const QByteArray & appIdentification, Backend backend )
{
  bool automatic = false;
  if ( backend == Auto )
  {
    automatic = true;
    if ( mode == KoStore::Write )
      backend = DefaultFormat;
    else {
      if ( device->open( QIODevice::ReadOnly ) ) {
        backend = determineBackend( device );
        device->close();
      }
    }
  }
  switch ( backend )
  {
  //case Tar:
    //return new KoTarStore( device, mode, appIdentification );
  case Directory:
    qCCritical(KOSTORE) << "Can't create a Directory store for a memory buffer!" << Qt::endl;
    // fallback
    Q_FALLTHROUGH();
  case Zip:
#ifdef QCA2
    if( automatic && mode == Read ) {
        // When automatically detecting, this might as well be an encrypted file. We'll need to check anyway, so we'll just use the encrypted store.
        return new KoEncryptedStore( device, Read, appIdentification );
    }
#else
    Q_UNUSED( automatic )
#endif
    return new KoZipStore( device, mode, appIdentification );
#ifdef QCA2
  case Encrypted:
    return new KoEncryptedStore( device, mode, appIdentification );
#endif
  default:
    qCWarning(KOSTORE) << "Unsupported backend requested for KoStore : " << backend;
    return 0L;
  }
}

KoStore* KoStore::createStore( QWidget* window, const QUrl& url, Mode mode, const QByteArray & appIdentification, Backend backend )
{
  const bool automatic = ( backend == Auto );
  if ( url.isLocalFile() )
    return createStore(url.toLocalFile(), mode,  appIdentification, backend );

  QTemporaryFile tmpFile;
  if ( mode == KoStore::Write )
  {
    if ( automatic )
      backend = DefaultFormat;
  }
  else
  {
    KIO::StoredTransferJob* transferJob = KIO::storedGet( url );
    KJobWidgets::setWindow( transferJob, window );
    bool downloaded = true;
    QObject::connect( transferJob, &KJob::result, [&](KJob*) {
      if( transferJob->error() != KJob::NoError ) {
        tmpFile.open();
        tmpFile.write( transferJob->data() );
        tmpFile.close();
      } else {
        downloaded = false;
      }
    } );

    if (!downloaded)
    {
      qCCritical(KOSTORE) << "Could not download file!" << Qt::endl;
      backend = DefaultFormat; // will create a "bad" store (bad()==true)
    }
    else if ( automatic )
    {
      if ( tmpFile.open() )
      {
        backend = determineBackend( &tmpFile );
        tmpFile.close();
      }
    }
  }
  switch ( backend )
  {
  //case Tar:
    //return new KoTarStore( &tmpFile, mode, appIdentification );
  case Zip:
#ifdef QCA2
    if( automatic && mode == Read ) {
        // When automatically detecting, this might as well be an encrypted file. We'll need to check anyway, so we'll just use the encrypted store.
        return new KoEncryptedStore( &tmpFile, Read, appIdentification );
    }
#endif
    return new KoZipStore( &tmpFile, mode, appIdentification );
#ifdef QCA2
  case Encrypted:
    return new KoEncryptedStore( &tmpFile, mode, appIdentification );
#endif
  default:
    qCWarning(KOSTORE) << "Unsupported backend requested for KoStore (QUrl) : " << backend;
    KMessageBox::error( window,
        i18n("The directory mode is not supported for remote locations."),
        i18n("KOffice Storage"));
    return 0L;
  }
}

namespace {
  const char* const ROOTPART = "root";
  const char* const MAINNAME = "maindoc.xml";
}

KoStore::KoStore()
{
}

bool KoStore::init( Mode _mode )
{
  m_bIsOpen = false;
  m_mode = _mode;
  m_stream = 0;
  m_bFinalized = false;

  // Assume new style names.
  m_namingVersion = NAMING_VERSION_2_2;
  return true;
}

KoStore::~KoStore()
{
  delete m_stream;
}

bool KoStore::open( const QString & _name )
{
  // This also converts from relative to absolute, i.e. merges the currentPath()
  m_sName = toExternalNaming( _name );

  if ( m_bIsOpen )
  {
    qCWarning(KOSTORE) << "KoStore: File is already opened";
    //return KIO::ERR_INTERNAL;
    return false;
  }

  if ( m_sName.length() > 512 )
  {
      qCCritical(KOSTORE) << "KoStore: Filename " << m_sName << " is too long" << Qt::endl;
      //return KIO::ERR_MALFORMED_URL;
      return false;
  }

  if ( m_mode == Write )
  {
    qCDebug(KOSTORE) <<"KoStore: opening for writing '" << m_sName <<"'";
    if ( m_strFiles.contains( m_sName ) )
    {
      qCWarning(KOSTORE) << "KoStore: Duplicate filename " << m_sName;
      //return KIO::ERR_FILE_ALREADY_EXIST;
      return false;
    }

    m_strFiles.append( m_sName );

    m_iSize = 0;
    if ( !openWrite( m_sName ) )
      return false;
  }
  else if ( m_mode == Read )
  {
    qCDebug(KOSTORE) <<"Opening for reading '" << m_sName <<"'";
    if ( !openRead( m_sName ) )
      return false;
  }
  else
    //return KIO::ERR_UNSUPPORTED_ACTION;
    return false;

  m_bIsOpen = true;
  return true;
}

bool KoStore::isOpen() const
{
  return m_bIsOpen;
}

bool KoStore::close()
{
  qCDebug(KOSTORE) <<"KoStore: Closing";

  if ( !m_bIsOpen )
  {
    qCWarning(KOSTORE) << "KoStore: You must open before closing";
    //return KIO::ERR_INTERNAL;
    return false;
  }

  bool ret = m_mode == Write ? closeWrite() : closeRead();

  delete m_stream;
  m_stream = 0L;
  m_bIsOpen = false;
  return ret;
}

QIODevice* KoStore::device() const
{
  if ( !m_bIsOpen )
    qCWarning(KOSTORE) << "KoStore: You must open before asking for a device";
  if ( m_mode != Read )
    qCWarning(KOSTORE) << "KoStore: Can not get device from store that is opened for writing";
  return m_stream;
}

QByteArray KoStore::read( qint64 max )
{
  QByteArray data;

  if ( !m_bIsOpen )
  {
    qCWarning(KOSTORE) << "KoStore: You must open before reading";
    return data;
  }
  if ( m_mode != Read )
  {
    qCCritical(KOSTORE) << "KoStore: Can not read from store that is opened for writing" << Qt::endl;
    return data;
  }

  return m_stream->read( max );
}

qint64 KoStore::write( const QByteArray& data )
{
  return write( data.data(), data.size() ); // see below
}

qint64 KoStore::read( char *_buffer, qint64 _len )
{
  if ( !m_bIsOpen )
  {
    qCCritical(KOSTORE) << "KoStore: You must open before reading" << Qt::endl;
    return -1;
  }
  if ( m_mode != Read )
  {
    qCCritical(KOSTORE) << "KoStore: Can not read from store that is opened for writing" << Qt::endl;
    return -1;
  }

  return m_stream->read( _buffer, _len );
}

qint64 KoStore::write( const char* _data, qint64 _len )
{
  if ( _len == 0L ) return 0;

  if ( !m_bIsOpen )
  {
    qCCritical(KOSTORE) << "KoStore: You must open before writing" << Qt::endl;
    return 0L;
  }
  if ( m_mode != Write  )
  {
    qCCritical(KOSTORE) << "KoStore: Can not write to store that is opened for reading" << Qt::endl;
    return 0L;
  }

  int nwritten = m_stream->write( _data, _len );
  Q_ASSERT( nwritten == (int)_len );
  m_iSize += nwritten;

  return nwritten;
}

qint64 KoStore::size() const
{
  if ( !m_bIsOpen )
  {
    qCWarning(KOSTORE) << "KoStore: You must open before asking for a size";
    return static_cast<qint64>(-1);
  }
  if ( m_mode != Read )
  {
    qCWarning(KOSTORE) << "KoStore: Can not get size from store that is opened for writing";
    return static_cast<qint64>(-1);
  }
  return m_iSize;
}

bool KoStore::enterDirectory( const QString& directory )
{
  //qCDebug(KOSTORE) <<"KoStore::enterDirectory" << directory;
  int pos;
  bool success = true;
  QString tmp( directory );

  while ( ( pos = tmp.indexOf( '/' ) ) != -1 &&
          ( success = enterDirectoryInternal( tmp.left( pos ) ) ) )
          tmp = tmp.mid( pos + 1 );

  if ( success && !tmp.isEmpty() )
    return enterDirectoryInternal( tmp );
  return success;
}

bool KoStore::leaveDirectory()
{
  if ( m_currentPath.isEmpty() )
    return false;

  m_currentPath.pop_back();

  return enterAbsoluteDirectory( expandEncodedDirectory( currentPath() ) );
}

QString KoStore::currentDirectory() const
{
  return expandEncodedDirectory( currentPath() );
}

QString KoStore::currentPath() const
{
  QString path;
  QStringList::ConstIterator it = m_currentPath.begin();
  QStringList::ConstIterator end = m_currentPath.end();
  for ( ; it != end; ++it ) {
    path += *it;
    path += '/';
  }
  return path;
}

void KoStore::pushDirectory()
{
  m_directoryStack.push( currentPath() );
}

void KoStore::popDirectory()
{
  m_currentPath.clear();
  enterAbsoluteDirectory( QString() );
  enterDirectory( m_directoryStack.pop() );
}

bool KoStore::addLocalFile( const QString &fileName, const QString &destName )
{
  QFileInfo fi( fileName );
  uint size = fi.size();
  QFile file( fileName );
  if ( !file.open( QIODevice::ReadOnly ))
  {
    return false;
  }

  if ( !open ( destName ) )
  {
    return false;
  }

  QByteArray data;
  data.resize( 8 * 1024 );

  uint total = 0;
  for ( int block = 0; ( block = file.read( data.data(), data.size() ) ) > 0; total += block )
  {
    data.resize(block);
    if ( write( data ) != block )
      return false;
    data.resize(8*1024);
  }
  Q_ASSERT( total == size );

  close();
  file.close();

  return true;
}

bool KoStore::addDataToFile( QByteArray &buffer, const QString &destName )
{
  QBuffer file( &buffer );
  if ( !file.open( QIODevice::ReadOnly ))
  {
    return false;
  }

  if ( !open ( destName ) )
  {
    return false;
  }

  QByteArray data;
  data.resize( 8 * 1024 );

  uint total = 0;
  for ( int block = 0; ( block = file.read( data.data(), data.size() ) ) > 0; total += block )
  {
    data.resize(block);
    if ( write( data ) != block )
      return false;
    data.resize(8*1024);
  }

  close();
  file.close();

  return true;
}

bool KoStore::extractFile ( const QString &srcName, const QString &fileName )
{
  QFile file( fileName );
  return extractFile( srcName, file );
}


bool KoStore::extractFile( const QString &srcName, QByteArray &data )
{
  QBuffer buffer( &data );
  return extractFile( srcName, buffer );
}

bool KoStore::extractFile( const QString &srcName, QIODevice &buffer )
{
  if ( !open ( srcName ) )
     return false;

  if( !buffer.open ( QIODevice::WriteOnly ) )
  {
    close();
    return false;
  }
  // ### This could use KArchive::copy or something, no?

  QByteArray data;
  data.resize( 8 * 1024 );
  uint total = 0;
  for( int block = 0; ( block = read( data.data(), data.size() ) ) > 0; total += block )
  {
    buffer.write( data.data(), block );
  }

  if( size() != static_cast<qint64>(-1) )
    Q_ASSERT( total == size() );

  buffer.close();
  close();

  return true;
}


QStringList KoStore::addLocalDirectory( const QString &dirPath, const QString &destName )
{
  QString dot(".");
  QString dotdot("..");
  QStringList content;

  QDir dir(dirPath);
  if ( !dir.exists() )
    return QStringList();

  QStringList files = dir.entryList();
  for ( QStringList::Iterator it = files.begin(); it != files.end(); ++it )
  {
     if ( *it != dot && *it != dotdot )
     {
        QString currentFile = dirPath + '/' + *it;
        QString dest = destName.isEmpty() ? *it : (destName + '/' + *it);

        QFileInfo fi ( currentFile );
        if ( fi.isFile() )
        {
          addLocalFile ( currentFile, dest );
          content.append(dest);
        }
        else if ( fi.isDir() )
        {
          content += addLocalDirectory ( currentFile, dest );
        }
     }
  }

  return content;
}


bool KoStore::seek( qint64 pos )
{
  return m_stream->seek( pos );
}

qint64 KoStore::pos() const
{
  return m_stream->pos();
}

bool KoStore::atEnd() const
{
  return m_stream->atEnd();
}

// See the specification for details of what this function does.
QString KoStore::toExternalNaming( const QString & _internalNaming ) const
{
  if ( _internalNaming == ROOTPART )
    return expandEncodedDirectory( currentPath() ) + MAINNAME;

  QString intern;
  if ( _internalNaming.startsWith( "tar:/" ) ) // absolute reference
    intern = _internalNaming.mid( 5 ); // remove protocol
  else
    intern = currentPath() + _internalNaming;

  return expandEncodedPath( intern );
}

QString KoStore::expandEncodedPath( const QString& _intern ) const
{
  QString intern = _intern;

  if ( m_namingVersion == NAMING_VERSION_RAW )
    return intern;

  QString result;
  int pos;

  if ( ( pos = intern.lastIndexOf( '/', -1 ) ) != -1 ) {
    result = expandEncodedDirectory( intern.left( pos ) ) + '/';
    intern = intern.mid( pos + 1 );
  }

  // Now process the filename. If the first character is numeric, we have
  // a main document.
  if ( QChar(intern.at(0)).isDigit() )
  {
    // If this is the first part name, check if we have a store with
    // old-style names.
    if ( ( m_namingVersion == NAMING_VERSION_2_2 ) &&
         ( m_mode == Read ) &&
         ( fileExists( result + "part" + intern + ".xml" ) ) )
      m_namingVersion = NAMING_VERSION_2_1;

    if ( m_namingVersion == NAMING_VERSION_2_1 )
      result = result + "part" + intern + ".xml";
    else
      result = result + "part" + intern + '/' + MAINNAME;
  }
  else
    result += intern;
  return result;
}

QString KoStore::expandEncodedDirectory( const QString& _intern ) const
{
  QString intern = _intern;

  if ( m_namingVersion == NAMING_VERSION_RAW )
    return intern;

  QString result;
  int pos;
  while ( ( pos = intern.indexOf( '/' ) ) != -1 ) {
    if ( QChar(intern.at(0)).isDigit() )
      result += "part";
    result += intern.left( pos + 1 ); // copy numbers (or "pictures") + "/"
    intern = intern.mid( pos + 1 ); // remove the dir we just processed
  }

  if ( !intern.isEmpty() && QChar(intern.at(0)).isDigit() )
    result += "part";
  result += intern;
  return result;
}

bool KoStore::enterDirectoryInternal( const QString& directory )
{
    if ( enterRelativeDirectory( expandEncodedDirectory( directory ) ) )
    {
      m_currentPath.append( directory );
      return true;
    }
    return false;
}

void KoStore::disallowNameExpansion( void )
{
    m_namingVersion = NAMING_VERSION_RAW;
}

bool KoStore::hasFile( const QString& fileName ) const
{
  return fileExists( toExternalNaming( currentPath() + fileName ) );
}

bool KoStore::finalize()
{
  Q_ASSERT( !m_bFinalized ); // call this only once!
  m_bFinalized = true;
  return doFinalize();
}

bool KoStore::isEncrypted()
{
    return false;
}

bool KoStore::setPassword( const QString& /*password*/ )
{
    return false;
}

QString KoStore::password( )
{
    return QString();
}
