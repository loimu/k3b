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

#include "k3bcuefileparser.h"

#include <k3bmsf.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qregexp.h>

#include <kdebug.h>


// TODO: add method: usableByCdrecordDirectly()
// TODO: add Toc with sector sizes

class K3bCueFileParser::Private
{
public:
  bool inFile;
  bool inTrack;
  bool isAudioTrack;
  bool haveIndex1;
};



K3bCueFileParser::K3bCueFileParser( const QString& filename )
  : K3bImageFileReader()
{
  d = new Private;
  openFile( filename );
}


K3bCueFileParser::~K3bCueFileParser()
{
  delete d;
}


void K3bCueFileParser::readFile()
{
  setValid(true);

  d->inFile = d->inTrack = d->haveIndex1 = d->isAudioTrack = false;

  QFile f( filename() );
  if( f.open( IO_ReadOnly ) ) {
    QTextStream s( &f );
    QString line = s.readLine();
    while( !line.isNull() ) {
      
      if( !parseLine(line) ) {
	setValid(false);
	break;
      }

      line = s.readLine();
    }
  }
  else {
    kdDebug() << "(K3bCueFileParser) could not open file " << filename() << endl;
    setValid(false);
  }
}


bool K3bCueFileParser::parseLine( QString& line )
{
  // use cap(1) for the filename
  static QRegExp fileRx( "FILE\\s\"?([^\"]*)\"?\\s[^\"\\s]*" );

  // use cap(1) for the flags
  static QRegExp flagsRx( "FLAGS(\\s(DCP|4CH|PRE|SCMS)){1,4}" );

  // use cap(1) for the tracknumber and cap(2) for the datatype
  static QRegExp trackRx( "TRACK\\s(\\d{1,2})\\s(AUDIO|CDG|MODE1/2048|MODE1/2352|MODE2/2336|MODE2/2352|CDI/2336|CDI/2352)" );

  // use cap(1) for the index number, cap(3) for the minutes, cap(4) for the seconds, cap(5) for the frames,
  // and cap(2) for the MSF value string
  static QRegExp indexRx( "INDEX\\s(\\d{1,2})\\s((\\d\\d):([0-5]\\d):((?:[0-6]\\d)|(?:7[0-4])))" );

  // use cap(1) for the MCN
  static QRegExp catalogRx( "CATALOG\\s(\\w{13,13})" );

  // use cap(1) for the ISRC
  static QRegExp isrcRx( "ISRC\\s(\\w{5,5}\\d{7,7})" );

  static QString cdTextRxStr = "\"?([^\"]{1,80})\"?";

  // use cap(1) for the string
  static QRegExp titleRx( "TITLE\\s" + cdTextRxStr );
  static QRegExp performerRx( "PERFORMER\\s" + cdTextRxStr );
  static QRegExp songwriterRx( "SONGWRITER\\s" + cdTextRxStr );


  // simplify all white spaces except those in filenames and CD-TEXT
  simplifyWhiteSpace( line );

  // skip comments and empty lines
  if( line.startsWith("REM") || line.startsWith("#") || line.isEmpty() )
    return true;


  //
  // FILE
  //
  if( fileRx.exactMatch( line ) ) {

    QString dataFile = fileRx.cap(1);

    // find data file
    if( dataFile[0] == '/' ) {
      setImageFilename( dataFile );
    }
    else {
      // relative path
      setImageFilename( filename().mid( 0, filename().findRev('/') + 1 ) + dataFile );
    }

    //
    // CDRDAO does not use this image filename but replaces the extension from the cue file
    // with "bin" to get the image filename, we should take this into account
    //
    kdDebug() << "(K3bCueFileParser) trying bin file: " << dataFile << endl;
    if( QFileInfo( imageFilename() ).isFile() ) {
      setValid( true );
      m_imageFilenameInCue = true;
    }
    else {
      setImageFilename( filename().mid( filename().length() - 3 ) + "bin" );
      setValid( QFileInfo( imageFilename() ).isFile() );
      m_imageFilenameInCue = false;
    }
    
    if( d->inFile ) {
      kdDebug() << "(K3bCueFileParser) only one FILE statement allowed." << endl;
      return false;
    }
    d->inFile = true;
    d->inTrack = false;
    d->haveIndex1 = false;
    return true;
  }


  //
  // TRACK
  //
  else if( trackRx.exactMatch( line ) ) {
    if( !d->inFile ) {
      kdDebug() << "(K3bCueFileParser) TRACK statement before FILE." << endl;
      return false;
    }

    // check if we had index1 for the last track
    if( d->inTrack && !d->haveIndex1 ) {
      kdDebug() << "(K3bCueFileParser) TRACK without INDEX 1." << endl;
      return false;
    }

    d->isAudioTrack = ( trackRx.cap(2) == "AUDIO" );
    d->haveIndex1 = false;
    d->inTrack = true;
    return true;
  }


  //
  // FLAGS
  //
  else if( flagsRx.exactMatch( line ) ) {
    if( !d->inTrack ) {
      kdDebug() << "(K3bCueFileParser) FLAGS statement without TRACK." << endl;
      return false;
    }

    // TODO: save the flags
    return true;
  }


  //
  // INDEX
  //
  else if( indexRx.exactMatch( line ) ) {
    if( !d->inTrack ) {
      kdDebug() << "(K3bCueFileParser) INDEX statement without TRACK." << endl;
      return false;
    }

    unsigned int indexNumber = indexRx.cap(1).toInt();

    K3b::Msf indexStart = K3b::Msf::fromString( indexRx.cap(2) );

    if( indexNumber == 0 ) {
      // TODO: save pregap start and set last track's length
    }
    else if( indexNumber == 1 ) {
      d->haveIndex1 = true;
      // TODO: set length of last track or pregap length of this track
    }
    else {
      // TODO: add index
    }

    return true;
  }


  //
  // CATALOG
  //
  if( catalogRx.exactMatch( line ) ) {
    // TODO: set the toc's mcn
    return true;
  }


  //
  // ISRC
  //
  if( isrcRx.exactMatch( line ) ) {
    if( d->inTrack ) {
      // TODO: set the track's ISRC
      return true;
    }
    else {
      kdDebug() << "(K3bCueFileParser) ISRC without TRACK." << endl;
      return false;
    }
  }


  //
  // CD-TEXT
  // TODO: create K3bCdDevice::TrackCdText entries
  //
  else if( titleRx.exactMatch( line ) ) {
    if( d->inTrack )
      kdDebug() << "TRACK-TITLE: '" << titleRx.cap(1) << "'" << endl;
    else
      kdDebug() << "CD-TITLE: '" << titleRx.cap(1) << "'" << endl;
    return true;
  }

  else if( performerRx.exactMatch( line ) ) {
    if( d->inTrack )
      kdDebug() << "TRACK-PERFORMER: '" << performerRx.cap(1) << "'" << endl;
    else
      kdDebug() << "CD-PERFORMER: '" << performerRx.cap(1) << "'" << endl;
    return true;
  }

  else if( songwriterRx.exactMatch( line ) ) {
    if( d->inTrack )
      kdDebug() << "TRACK-SONGWRITER: '" << songwriterRx.cap(1) << "'" << endl;
    else
      kdDebug() << "CD-SONGWRITER: '" << songwriterRx.cap(1) << "'" << endl;
    return true;
  }

  else {
    kdDebug() << "(K3bCueFileParser) unknown Cue line: '" << line << "'" << endl;
    return false;
  }
}


void K3bCueFileParser::simplifyWhiteSpace( QString& s )
{
  s = s.stripWhiteSpace();

  unsigned int i = 0;
  bool insideQuote = false;
  while( i < s.length() ) {
    if( !insideQuote ) {
      if( s[i].isSpace() && s[i+1].isSpace() )
	s.remove( i, 1 );
    }

    if( s[i] == '"' )
      insideQuote = !insideQuote;

    ++i;
  }
}
