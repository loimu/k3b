/* 
 *
 * $Id$
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


#ifndef _K3B_VIDEO_DVDVIEW_H_
#define _K3B_VIDEO_DVDVIEW_H_

#include <k3bdvdview.h>

class K3bVideoDvdDoc;


class K3bVideoDvdView : public K3bDvdView
{
  Q_OBJECT

 public:
  K3bVideoDvdView( K3bVideoDvdDoc* doc, QWidget *parent = 0, const char *name = 0 );
  ~K3bVideoDvdView();

 protected:
  virtual K3bProjectBurnDialog* newBurnDialog( QWidget* parent = 0, const char* name = 0 );

  void init();

 private:
  K3bVideoDvdDoc* m_doc;
};

#endif
