/****************************************************************
 *  Copyright 2010, Fair Use, Inc.
 *  Copyright 2004-8, Robert Fernie
 *
 *  This file is part of the Mixologist.
 *
 *  The Mixologist is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  The Mixologist is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the Mixologist; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef P3_PEER_INTERFACE_H
#define P3_PEER_INTERFACE_H

#include "interface/peers.h"
#include "pqi/authmgr.h"
#include <QString>

/*
 * p3Peers.
 *
 * Top level friend interface.
 *
 * Implements Peers interface for control from the GUI.
 *
 */

class p3Peers: public Peers {
public:

    p3Peers(QString &ownName);
    virtual ~p3Peers() {}

    /* Returns the logged in Mixologist user's own encryption certificate id. */
    virtual std::string getOwnCertId();

    /* Returns the logged in Mixologist user's own LibraryMixer id. */
    virtual unsigned int getOwnLibraryMixerId();

    /* Returns the logged in user's own name. */
    virtual QString getOwnName();

    /* Returns whether our connection is currently ready. */
    virtual bool getConnectionReadiness();

    /* Returns whether auto-connection config is enabled. */
    virtual bool getConnectionAutoConfigEnabled();

    /* Returns our current connection status. */
    virtual ConnectionStatus getConnectionStatus();

    /* Shuts down our current connection and starts it up again. */
    virtual void restartOwnConnection();

    /* List of LibraryMixer ids for all online friends. */
    virtual void getOnlineList(QList<unsigned int> &friend_ids);

    /* List of LibraryMixer ids for all friends with encryption keys. */
    virtual void getSignedUpList(QList<unsigned int> &friend_ids);

    /* List of LibraryMixer ids for all friends. */
    virtual void getFriendList(QList<unsigned int> &friend_ids);

    /* Returns true if that id belongs to a friend. */
    virtual bool isFriend(unsigned int librarymixer_id);

    /* Returns true if that id belongs to a friend, and that friend is online. */
    virtual bool isOnline(unsigned int librarymixer_id);

    /* Returns the name of the friend with that LibraryMixer id.
       Returns an empty string if no such friend. */
    virtual QString getPeerName(unsigned int librarymixer_id);

    /* Fills in PeerDetails for user with librarymixer_id
       Can be used for friends or self.
       Returns true, or false if unable to find user with librarymixer_id*/
    virtual bool getPeerDetails(unsigned int librarymixer_id, PeerDetails &d);

    /* Either adds a new friend, or updates the existing friend. */
    virtual bool addUpdateFriend(unsigned int librarymixer_id, const QString &cert, const QString &name,
                                 const QString &localIP, ushort localPort,
                                 const QString &externalIP, ushort externalPort);

    /* Immediate retry to connect to all offline friends. */
    virtual void connectAll();

private:
    /* Master storage in the Mixologist for a user's own name. */
    QString ownName;
};

#endif
