/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef koStoreDevice_h
#define koStoreDevice_h

#include <KoStore.h>

/**
 * This class implements a QIODevice around KoStore, so that
 * it can be used to create a QDomDocument from it, to be written or read
 * using QDataStream or to be written using QTextStream
 */
class KoStoreDevice : public QIODevice
{
public:
  /// Note: KoStore::open() should be called before calling this.
  explicit KoStoreDevice( KoStore * store ) : m_store(store) {
      // koffice-1.x behavior compat: a KoStoreDevice is automatically open
      setOpenMode( m_store->mode() == KoStore::Read ? QIODevice::ReadOnly : QIODevice::WriteOnly );
  }
  ~KoStoreDevice() override {}

  bool isSequential() const override { return true; }

  bool open( OpenMode m ) override {
    setOpenMode(m);
    if ( m & QIODevice::ReadOnly )
      return ( m_store->mode() == KoStore::Read );
    if ( m & QIODevice::WriteOnly )
      return ( m_store->mode() == KoStore::Write );
    return false;
  }
  void close() override {}

  qint64 size() const override {
    if ( m_store->mode() == KoStore::Read )
      return m_store->size();
    else
      return 0xffffffff;
  }

  qint64 readData( char *data, qint64 maxlen ) override { return m_store->read(data, maxlen); }
  qint64 writeData( const char *data, qint64 len ) override { return m_store->write( data, len ); }

#if 0
  int getch() {
    char c[2];
    if ( m_store->read(c, 1) == -1)
      return -1;
    else
      return c[0];
  }
  int putch( int _c ) {
    char c[2];
    c[0] = _c;
    c[1] = 0;
    if (m_store->write( c, 1 ) == 1)
      return _c;
    else
      return -1;
  }
  int ungetch( int ) { return -1; } // unsupported
#endif

  // See QIODevice
  qint64 pos() const override { return m_store->pos(); }
  bool seek( qint64 pos ) override { return m_store->seek(pos); }
  bool atEnd() const override { return m_store->atEnd(); }

protected:
  KoStore * m_store;
};

#endif
