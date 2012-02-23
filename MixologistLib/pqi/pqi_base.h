/****************************************************************
 *  Copyright 2010, Fair Use, Inc.
 *  Copyright 2004-6, Robert Fernie
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

#ifndef PQI_BASE_ITEM_HEADER
#define PQI_BASE_ITEM_HEADER

#include <list>
#include <string>
#include <iostream>
#include <functional>
#include <algorithm>
#include <inttypes.h>

#include "pqi/pqinetwork.h"

/*** Base DataTypes: ****/
#include "serialiser/serial.h"


#define PQI_MIN_PORT 1024
#define PQI_MAX_PORT 50000
#define PQI_MIN_RAND_PORT 10000
#define PQI_MAX_RAND_PORT 30000
#define PQI_DEFAULT_PORT 1680

/**** Consts for pqiperson */

const uint32_t PQI_CONNECT_TCP = 0x0001;
const uint32_t PQI_CONNECT_UDP = 0x0002;

int getPQIsearchId();
int fixme(char *str, int n);

/*********************** PQI INTERFACE ******************************\
The basic exchange interface.
Includes methods for getting and sending items, ongoing maintenance,
notification from NetInterfaces and bandwidth control.
Types include pqistreamer for taking structured data and converting it into binary data to send over a network,
pqiperson which holds multiple pqistreamers (one for each connection method) and uses PQInterface as an interface to pass through to them,
and pqiloopback.
*/

class NetInterface;

class PQInterface {
public:
    PQInterface(std::string id, int _librarymixer_id)
        :cert(id), librarymixer_id(_librarymixer_id),bw_in(0), bw_out(0), bwMax_in(0), bwMax_out(0) {
        return;
    }
    virtual ~PQInterface() {
        return;
    }

    virtual int SendItem(NetItem *) = 0;
    virtual NetItem *GetItem() = 0;

    virtual int tick() {return 0;}

    virtual std::string PeerId() {return cert;}

    virtual int LibraryMixerId() {return librarymixer_id;}

    //Called by NetInterfaces to inform the PQInterface of connection events.
    virtual int notifyEvent(NetInterface *ni, int event) {
        (void)ni;
        (void)(event);
        return 0;
    }

    virtual float getRate(bool in) {
        if (in) return bw_in;
        return bw_out;
    }

    virtual float getMaxRate(bool in) {
        if (in) return bwMax_in;
        return bwMax_out;
    }

    virtual void setMaxRate(bool in, float val) {
        if (in) bwMax_in = val;
        else bwMax_out = val;
    }

protected:

    void setRate(bool in, float val) {
        if (in) bw_in = val;
        else bw_out = val;
    }

private:

    std::string cert;
    unsigned int librarymixer_id;
    float bw_in, bw_out, bwMax_in, bwMax_out;
};


/*
 * NetInterface
 *
 * Basic interface for handling control of a connection to a particular friend, whether or not connected.
 * It is passed a pointer to a PQInterface *parent, which it uses to notify the system of connect/disconnect Events.
 */

//Possible notification events
static const int NET_CONNECT_RECEIVED     = 1;
static const int NET_CONNECT_SUCCESS      = 2;
static const int NET_CONNECT_UNREACHABLE  = 3;
static const int NET_CONNECT_FIREWALLED   = 4;
static const int NET_CONNECT_FAILED       = 5;

class NetInterface {
public:
    NetInterface(PQInterface *p_in, std::string cert, int _librarymixer_id)
        :p(p_in), cert(cert), librarymixer_id(_librarymixer_id) {
        return;
    }

    virtual ~NetInterface() {
        return;
    }

    /* Attempt a connection to the friend at the specified address.
       Returns 1 on success, 0 on failure, -1 on error. */
    virtual int connect(struct sockaddr_in raddr) = 0;

    /* Begin listening for connections from this friend.
       Returns 1 on success, 0 on not ready, and -1 on error. */
    virtual int listen() = 0;

    /* Stops listening for connections from this friend.
       Returns 1 on success or if already not listening, and -1 on error. */
    virtual int stoplistening() = 0;

    /* Closes any existing connection with the friend, and resets to initial condition.
       Can be called even if not currently connected. */
    virtual void reset() = 0;

    virtual std::string PeerId() {return cert;}

    virtual int LibraryMixerId() {return librarymixer_id;}

    /* Sets the specified parameter to value.
       Returns false if no such parameter applies to the NetInterface.

       NET_PARAM_CONNECT_DELAY: It is useful to be able to delay TCP connections to insure
       simultaneous connections to one user via their different addresses don't occur
       simultaneously. According to comments written by the RetroShare devs, this can
       cause connection failures. Therefore, by delaying one or more of the addresses
       we can ensure this doesn't happen. */
    enum netParameters {
        NET_PARAM_CONNECT_DELAY   = 1,
        NET_PARAM_CONNECT_PERIOD  = 2,
        NET_PARAM_CONNECT_TIMEOUT = 3
    };
    virtual bool setConnectionParameter(netParameters type, uint32_t value) = 0;

protected:
    PQInterface *parent() {
        return p;
    }

private:
    PQInterface *p;
    std::string cert;
    unsigned int librarymixer_id;
};

/********************** Binary INTERFACE ****************************
The binary interface used by Network/loopback/file interfaces
*/

#define BIN_FLAGS_NO_CLOSE  0x0001
#define BIN_FLAGS_READABLE  0x0002
#define BIN_FLAGS_WRITEABLE 0x0004
#define BIN_FLAGS_NO_DELETE 0x0008
#define BIN_FLAGS_HASH_DATA 0x0010

class BinInterface {
public:
    BinInterface() {
        return;
    }
    virtual ~BinInterface() {
        return;
    }

    virtual int tick() = 0;

    /* Sends data with length.
       Returns the amount of data sent, or -1 on error. */
    virtual int senddata(void *data, int length) = 0;

    /* Returns -1 on errors, otherwise returns the size in bytes of the packet this is read. */
    virtual int readdata(void *data, int len) = 0;

    /* Returns true when connected. */
    virtual bool isactive() = 0;

    /* Returns true when there is more data available to be read by the connection. */
    virtual bool moretoread() = 0;

    /* Returns true when more data is ready to be sent by the connection. */
    virtual bool cansend() = 0;

    /* Closes any existing connection with the friend, and resets to initial condition.
       Can be called even if not currently connected. */
    virtual void close() = 0;

    /* If true, this connection is subject to fair usage balancing between other connections in pqistreamer. */
    virtual bool bandwidthLimited() {return true;}
};

/*
A complete connection, which implements both NetInterface for connection control,
as well as BinInterface for data transfer.
Implementations include pqissl and pqissludp.
*/

class NetBinInterface: public NetInterface, public BinInterface {
public:
    NetBinInterface(PQInterface *parent, std::string id, unsigned int librarymixer_id)
        :NetInterface(parent, id, librarymixer_id) {
        return;
    }
    virtual ~NetBinInterface() {
        return;
    }
};

#define CHAN_SIGN_SIZE 16
#define CERTSIGNLEN 16       /* actual byte length of signature */
#define PQI_PEERID_LENGTH 32 /* When expanded into a string */


#endif // PQI_BASE_ITEM_HEADER

