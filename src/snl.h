/*
   The SNL (Simple Network Layer) provides an API for network programming.
   Copyright (C) 2001 - 2014 Clemens Kirchgatterer <clemens@1541.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/**
   \file
   \author  Clemens Kirchgatterer <clemens@1541.org>
   \version 2.0.0
   \date    2013-11-26
   \brief   An user friendly asyncronous (threaded) network library.

   This is a very user friendly library to hide all complicating things about
   network programming behind a neat api. It starts the threads on incoming
   connections, sends and receives data over a socket and takes care of large
   enough buffers. This way, a buffer overflow is definitly impossible to
   happen, as long as only library functions are used to read from and write
   to the socket filedescriptors.
*/

#ifndef _SNL_H_
#define _SNL_H_

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   \brief   Struct for all Connection related information

   The snl_socket_t struct holds all informations related to a
   specific connection, like fd, port and ip numbers, ...
*/
typedef struct snl_socket_t {
   int event_code;
   int error_code;
   int file_descriptor;
   int protocol;
   void *data_buffer;
   unsigned int data_length;
   unsigned int buffer_length;
   unsigned int xfer_sent;
   unsigned int xfer_rcvd;
   unsigned short client_port;
   unsigned int client_ip;
   int client_fd;
   int worker_type;
   int worker_stoped;
   int worker_stop;
   pthread_t worker_tid;
   void *user_data;
   void (*event_callback)();
} snl_socket_t;

/**
   \brief Connection type enumeration.

   Enumeration of all possible connection types.
*/
enum {
   SNL_PROTO_MSG,       ///< framed messages over stream socket
   SNL_PROTO_UDP,       ///< unframed datagram socket
   SNL_PROTO_TCP        ///< stream socket without packet header
};

/**
   \brief Event type enumeration.

   Enumeration of all possible events.
*/
enum {
   SNL_EVENT_UNKNOWN,
   SNL_EVENT_ERROR,
   SNL_EVENT_ACCEPT,
   SNL_EVENT_RECEIVE,
   SNL_EVENT_READ
};

/**
   \brief   Error Codes definitions.
*/
enum {
   SNL_ERROR_OK,           ///<  0: no error
   SNL_ERROR_OPEN,         ///<  1: could'nt open socket
   SNL_ERROR_CONNECT,      ///<  2: connecting to remote socket failed
   SNL_ERROR_LISTEN,       ///<  3: error while listening to socket
   SNL_ERROR_BIND,         ///<  4: couldn't bind to socket
   SNL_ERROR_ACCEPT,       ///<  5: error while accepting connection
   SNL_ERROR_RECEIVE,      ///<  6: couldn't read from socket
   SNL_ERROR_SEND,         ///<  7: failed to send datagram
   SNL_ERROR_CLOSED,       ///<  8: peer closed connection
   SNL_ERROR_BUFFER,       ///<  9: out of memory (buffer allocation error)
   SNL_ERROR_ADDRESS,      ///< 10: could not get address from hostname
   SNL_ERROR_DISCONNECT,   ///< 11: error while closing socket
   SNL_ERROR_PROTOCOL,     ///< 12: protocol mismatch
   SNL_ERROR_THREAD,       ///< 13: could not start worker thread
   SNL_ERROR_TIMEOUT,      ///< 14: timeout error
   SNL_ERROR_BUSY,         ///< 15: socket is already connected or listening
};

/**
   \brief   Macro to shorten a callback function prototype.

   The SNL_EVENT_CB() macro expands a callback function to its full
   prototype to make the sourcecode shorter and easier to read.
*/
#define SNL_EVENT_CB(CB) void (CB)(snl_socket_t *skt)

/**
   \brief   Create new socket object.
   \param   proto <int> selects the used protocol (TCP, UDP or RAW)
   \param   cb <void (*)()> a pointer to the callback function
   \param   data <void *> user specific data
   \return  pointer to new socket object

   \note
   UDP and RAW socket packets do not have the usual 4 byte header, so do
   not make any assumtions over packet correctness or ordering. Also UDP
   packets \b must not be longer than 65535 bytes (64kb).
*/
snl_socket_t *snl_socket_new(int proto, SNL_EVENT_CB(*cb), void *data);

/**
   \brief   Destroy the socket object.
   \param   skt <snl_socket_t *> pointer to socket
   \return  0 on success or a negative error code

   The socket destructor closes a pending connection and terminates the
   associated worker thread.
*/
int snl_socket_delete(snl_socket_t *skt);

/**
   \brief   Send a datagram over the socket connection
   \param   skt <snl_socket_t *> pointer to socket
   \param   buf <const void *> pointer to the data to send
   \param   len <unsigned int> length of that data
   \return  0 on success or a negative error code

   This function is used to send a datagram over the network connection.
   Thereby the library ensures, that the hole datagram will get sent at
   once and that the other side receives all data correctly.
   In case of UDP no framing header will be sent in front of the datagram,
   because UDP garantees the whole datagram is sent unfragmented.
*/
int snl_send(snl_socket_t *skt, const void *buf, unsigned int len);

/**
   \brief   Start a seperate thread to handle exact one socket connection
   \param   skt <snl_socket_t *> pointer to socket
   \return  0 on success or a negative error code

   The accept function registers a new client connection from a listening
   socket...

   \note client specific information for the new connection is stored
         in the
*/
int snl_accept(snl_socket_t *skt);

/**
	\brief   Send raw data over the wire
	\param   fd <int> socket filedescriptor
	\param   buf <const void *> pointer to buffer start
	\param   len <unsigned int> length of data to send
	\return 0 on success or negative error code

	This function is used mainly internally but, you ever want to send
	data over the wire all by yourself, you can use this function.
*/
int snl_write(int fd, const void *buf, unsigned int len);

/**
   \brief   Start a thread to listen for incoming connections
   \param   skt <snl_socket_t *> pointer to socket
   \param   port <unsigned short> port number the server should listen on
   \return  0 on success or a negative error code

   To start the listening thread, this function must be used. Listening
   means either waiting for incoming connections for SNL_PROTO_TCP or
   SNL_PROTO_MSG sockets, or waiting for incoming datagrams if the
   socket protocol is set to SNL_PROTO_UDP.
*/
int snl_listen(snl_socket_t *skt, unsigned short port);

/**
   \brief   Connect to a listening socket
   \param   skt <snl_socket_t *> pointer to socket
   \param   host <const char *> hostname or ip address to connect
   \param   port <unsigned short> the port the server is listening on
   \return  0 on success or a negative error code

   For connecting to a snl server, one has to call this fuction. The first
   paramter can be a hostname or an ipaddress.
*/
int snl_connect(snl_socket_t *skt, const char *host, unsigned short port);

/**
   \brief   Close a socket connection
   \param   skt <snl_socket_t *> pointer to socket descriptor
   \return  0 on success or a negative error code

   This function has to be used for shutting down a socket connection. The
	socket can be reused (i.e. to connect somewhere else to).
*/
int snl_disconnect(snl_socket_t *skt);

/**
   \brief   Convert error code to string
   \param   error <int> snl error code
   \return  human readable error string

	%snl_error_string() converts a SNL error code to a human readable string.
*/
const char *snl_error_string(int error);

/**
   \brief   Initialize the SNL library
   \return  0 on success or a negative error code

   This function initializes the SNL library. For instance this will reduce
   the default stack size for internal threads and set SIGPIPE to ignore.
*/
int snl_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _SNL_H_ */
