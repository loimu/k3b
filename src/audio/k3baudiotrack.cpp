/***************************************************************************
                          k3baudiotrack.cpp  -  description
                             -------------------
    begin                : Wed Mar 28 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3baudiotrack.h"
#include "../tools/k3bglobals.h"

// ID3lib-includes
#include <id3/tag.h>
#include <id3/misc_support.h>

#include "input/k3baudiomodule.h"

#include <kapp.h>
#include <kconfig.h>

#include <qstring.h>
#include <qfileinfo.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>



K3bAudioTrack::K3bAudioTrack( QPtrList<K3bAudioTrack>* parent, const QString& filename )
: m_file(filename)
{
  m_parent = parent;
  m_copy = false;
  m_preEmp = false;
  m_length = 0;
  
  kapp->config()->setGroup( "Audio project settings" );
  m_pregap = kapp->config()->readNumEntry( "default pregap", 150 );
  

  m_module = 0;
}


K3bAudioTrack::~K3bAudioTrack()
{
  delete m_module;
}


unsigned long K3bAudioTrack::size() const
{
  return length() * 2352;
}


int K3bAudioTrack::index() const
{
  int i = m_parent->find( this );
  if( i < 0 )
    qDebug( "(K3bAudioTrack) I'm not part of my parent!");
  return i;
}

void K3bAudioTrack::setBufferFile( const QString& path )
{
  m_bufferFile = path;
}
