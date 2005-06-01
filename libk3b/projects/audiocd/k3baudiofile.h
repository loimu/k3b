/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_AUDIO_FILE_H_
#define _K3B_AUDIO_FILE_H_

#include "k3baudiodatasource.h"

#include <k3bmsf.h>
#include <kurl.h>
#include "k3b_export.h"

class K3bAudioDecoder;
class K3bAudioTrack;


class LIBK3B_EXPORT K3bAudioFile : public K3bAudioDataSource
{
 public:
  K3bAudioFile( K3bAudioDecoder*, K3bAudioDoc* );
  K3bAudioFile( const K3bAudioFile& );
  ~K3bAudioFile();

  const QString& filename() const;

  /**
   * The complete length of the file used by this source.
   */
  K3b::Msf originalLength() const;

  QString type() const;
  QString sourceComment() const;

  bool isValid() const;

  K3bAudioDecoder* decoder() const { return m_decoder; }

  bool seek( const K3b::Msf& );

  int read( char* data, unsigned int max );

  K3bAudioDataSource* copy() const;

 private:
  K3bAudioDoc* m_doc;
  K3bAudioDecoder* m_decoder;

  unsigned long long m_decodedData;
};

#endif
