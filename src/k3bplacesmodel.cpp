/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bplacesmodel.h"
#include "k3bdevicemodel.h"

#include "k3bdevice.h"
#include "k3bdevicemanager.h"
#include "k3bcore.h"

#include <KDirModel>
#include <KDirLister>
#include <KIcon>


class K3b::PlacesModel::Private
{
public:
    K3b::DeviceModel* deviceModel;
    QList<KDirModel*> dirModels;
};



K3b::PlacesModel::PlacesModel( QObject* parent )
    : K3b::MetaItemModel( parent ),
      d( new Private() )
{
    d->deviceModel = new K3b::DeviceModel( this );
    addSubModel( "Devices", KIcon( "media-optical" ), d->deviceModel, true );

    connect( d->deviceModel, SIGNAL( modelAboutToBeReset() ),
             this, SIGNAL( modelAboutToBeReset() ) );
    connect( d->deviceModel, SIGNAL( modelReset() ),
             this, SIGNAL( modelReset() ) );
    connect( k3bcore->deviceManager(), SIGNAL( changed( K3b::Device::DeviceManager* ) ),
             this, SLOT( slotDevicesChanged( K3b::Device::DeviceManager* ) ) );
    slotDevicesChanged( k3bcore->deviceManager() );
}


K3b::PlacesModel::~PlacesModel()
{
    delete d;
}


KFileItem K3b::PlacesModel::itemForIndex( const QModelIndex& index ) const
{
    KDirModel* model = qobject_cast<KDirModel*>( subModelForIndex( index ) );
    if ( model ) {
        return model->itemForIndex( mapToSubModel( index ) );
    }
    return KFileItem();
}


K3b::Device::Device* K3b::PlacesModel::deviceForIndex( const QModelIndex& index ) const
{
    if ( qobject_cast<K3b::DeviceModel*>( subModelForIndex( index ) ) == d->deviceModel ) {
        return d->deviceModel->deviceForIndex( mapToSubModel( index ) );
    }
    return 0;
}


QModelIndex K3b::PlacesModel::indexForDevice( K3b::Device::Device* dev ) const
{
    return mapFromSubModel( d->deviceModel->indexForDevice( dev ) );
}


void K3b::PlacesModel::expandToUrl( const KUrl& url )
{
    kDebug() << url;
    // search for a place that contains this URL
    foreach( KDirModel* model, d->dirModels ) {
        if ( model->dirLister()->url().isParentOf( url ) ) {
            kDebug() << model->dirLister()->url() << "will be expanded.";
            model->expandToUrl( url );
            break;
        }
    }
}


void K3b::PlacesModel::addPlace( const QString& name, const KIcon& icon, const KUrl& rootUrl )
{
    KDirModel* model = new KDirModel( this );
    connect( model, SIGNAL( expand( const QModelIndex& ) ), this, SLOT( slotExpand( const QModelIndex& ) ) );
    model->dirLister()->setDirOnlyMode( true );
    model->dirLister()->openUrl( rootUrl, KDirLister::Keep );
    d->dirModels.append( model );
    addSubModel( name, icon, model );
}


void K3b::PlacesModel::slotExpand( const QModelIndex& index )
{
    kDebug();
    emit expand( mapFromSubModel( index ) );
}


void K3b::PlacesModel::slotDevicesChanged( K3b::Device::DeviceManager* dm )
{
    kDebug();
    d->deviceModel->setDevices( dm->allDevices() );
}

#include "k3bplacesmodel.moc"
