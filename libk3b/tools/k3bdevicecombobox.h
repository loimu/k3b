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


#ifndef _K3B_DEVICE_COMBO_BOX_H_
#define _K3B_DEVICE_COMBO_BOX_H_

#include <kcombobox.h>
#include "k3b_export.h"

namespace K3bDevice {
  class Device;
}


class LIBK3B_EXPORT K3bDeviceComboBox : public KComboBox
{
  Q_OBJECT

 public:
  K3bDeviceComboBox( QWidget* parent = 0, const char* name = 0 );
  ~K3bDeviceComboBox();

  K3bDevice::Device* selectedDevice() const;

 signals:
  void selectionChanged( K3bDevice::Device* );

 public slots:
  void addDevice( K3bDevice::Device* );
  void addDevices( const QPtrList<K3bDevice::Device>& );
  void setSelectedDevice( K3bDevice::Device* );
  void clear();

 private slots:
  void slotActivated( int );

 private:
  class Private;
  Private* d;
};

#endif
