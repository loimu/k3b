/* 

    SPDX-FileCopyrightText: 2003-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#ifndef K3BSPLASH_H
#define K3BSPLASH_H

#include <QWidget>

class QLabel;
class QMouseEvent;
class QPaintEvent;
class QString;


namespace K3b {
class Splash : public QWidget
{
    Q_OBJECT

public:
    explicit Splash( QWidget* parent = 0 );
    ~Splash() override;

public Q_SLOTS:
    void show();
    void hide();
    void addInfo( const QString& );

protected:
    void mousePressEvent( QMouseEvent* ) override;
    //  void paintEvent( QPaintEvent* );

private:
    QLabel* m_infoBox;
};
}

#endif
