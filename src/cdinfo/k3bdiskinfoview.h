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



#ifndef K3BDISKINFOVIEW_H
#define K3BDISKINFOVIEW_H

#include "../k3bcdcontentsview.h"

#include "../k3bmedium.h"

class QLabel;
class KListView;
class K3bIso9660;

namespace K3bDevice {
  class DiskInfoDetector;
  class DiskInfo;
  class DiskInfo;
}

class K3bDiskInfoView : public K3bCdContentsView
{
  Q_OBJECT

 public:
  K3bDiskInfoView( QWidget* parent = 0, const char* name = 0 );
  ~K3bDiskInfoView();

  void reload();

 public slots:
  void displayInfo( const K3bMedium& );

 private:
  void createMediaInfoItems( const K3bDevice::DiskInfo& info );
  void createIso9660InfoItems( const K3bIso9660SimplePrimaryDescriptor* );

  KListView* m_infoView;

  class HeaderViewItem;
  class TwoColumnViewItem;
};


#endif
