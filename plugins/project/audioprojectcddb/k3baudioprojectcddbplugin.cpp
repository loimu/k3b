/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3baudioprojectcddbplugin.h"

#include <config-k3b.h>

// the k3b stuff we need
#include "k3bcore.h"
#include "k3bglobals.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3btoc.h"
#include "k3btrack.h"
#include "k3bmixeddoc.h"
#include "k3bmsf.h"
#include "k3bcddb.h"
#include "k3bplugin_i18n.h"

#include <KConfig>
#include <KMessageBox>

#include <QDebug>
#include <QString>
#include <QProgressDialog>

#include <KCDDB/CDInfo>


K_PLUGIN_CLASS_WITH_JSON(K3bAudioProjectCddbPlugin , "k3baudioprojectcddbplugin.json")



K3bAudioProjectCddbPlugin::K3bAudioProjectCddbPlugin( QObject* parent, const QVariantList& )
    : K3b::ProjectPlugin( K3b::Doc::AudioProject, false, parent )
{
    setText( i18n("Query CDDB") );
    setToolTip( i18n("Query a CDDB entry for the current audio project.") );
    setIcon( QIcon::fromTheme( "view-refresh" ) );
}


K3bAudioProjectCddbPlugin::~K3bAudioProjectCddbPlugin()
{
}


void K3bAudioProjectCddbPlugin::activate( K3b::Doc* doc, QWidget* parent )
{
    if( K3b::MixedDoc* mixedDoc = dynamic_cast<K3b::MixedDoc*>( doc ) )
        m_doc = mixedDoc->audioDoc();
    else
        m_doc = dynamic_cast<K3b::AudioDoc*>( doc );

    m_parentWidget = parent;
    m_canceled = false;

    if( !m_doc || m_doc->numOfTracks() == 0 ) {
        KMessageBox::error( parent, i18n("Please select a non-empty audio project for a CDDB query.") );
    }
    else {
        if( !m_progress ) {
            m_progress.reset( new QProgressDialog( i18n("Query CDDB"), i18n("Cancel"), 0, 0, parent ) );
            m_progress->setWindowTitle( i18n("Audio Project") );
        } else {
            m_progress->reset();
        }

        K3b::CDDB::CDDBJob* job = K3b::CDDB::CDDBJob::queryCddb( m_doc->toToc() );
        connect( job, SIGNAL(result(KJob*)),
                 this, SLOT(slotCddbQueryFinished(KJob*)) );

        m_progress->exec();
    }
}


void K3bAudioProjectCddbPlugin::slotCddbQueryFinished( KJob* job )
{
    if( !m_progress->wasCanceled() ) {

        if( !job->error() && m_doc ) {
            K3b::CDDB::CDDBJob* cddbJob = dynamic_cast<K3b::CDDB::CDDBJob*>( job );
            KCDDB::CDInfo cddbInfo = cddbJob->cddbResult();

            // save the entry to the doc
            m_doc->setTitle( cddbInfo.get( KCDDB::Title ).toString() );
            m_doc->setPerformer( cddbInfo.get( KCDDB::Artist ).toString() );
            m_doc->setCdTextMessage( cddbInfo.get( KCDDB::Comment ).toString() );

            int i = 0;
            for( K3b::AudioTrack* track = m_doc->firstTrack(); track; track = track->next() ) {
                KCDDB::TrackInfo info = cddbInfo.track( i );
                track->setTitle( info.get( KCDDB::Title ).toString() );
                track->setPerformer( info.get( KCDDB::Artist ).toString() );
                track->setCdTextMessage( info.get( KCDDB::Comment ).toString() );

                ++i;
            }

            // and enable cd-text
            m_doc->writeCdText( true );
        }
        else {
            KMessageBox::information( m_parentWidget, job->errorString(), i18n("CDDB error") );
        }
    }

    // make sure the progress dialog does not get deleted by it's parent
    m_progress.reset();
    m_doc.clear();
    m_parentWidget.clear();
}

#include "k3baudioprojectcddbplugin.moc"
