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

#define _POSIX_C_SOURCE	199309L // for nananosleep

#include <errno.h>       // errno, EINTR
#include <fcntl.h>       // F_GETFL, F_SETFL, fcntl()
#include <unistd.h>      // close(), read(), write()
#include <string.h>      // memset(), memcpy(), strlen()
#include <signal.h>      // signal(), SIG_IGN, SIGPIPE
#include <stdlib.h>      // malloc(), free()
#include <pthread.h>     // pthread_*()
#include <sys/socket.h>  // socket(), bind(), listen(), accept(), shutdown()
#include <netdb.h>       // gethostbyname()
#include <netinet/tcp.h> // TCP_NODELAY
#include <netinet/in.h>  // struct sockaddr_in
#include <arpa/inet.h>   // htons(), htonl(), ntohl()
#include <time.h>        // nanosleep()
#include <sys/time.h>    // struct timeval

#include "snl.h"

#define h_addr h_addr_list[0] // for backward compatibility
#define SA struct sockaddr    // for shorter lines

#define INITIAL_PAYLOAD_SIZE 1<<12 //  4KB
#define PACKED_PAYLOAD_SIZE  1<<10 //  1KB
#define UDP_PAYLOAD_SIZE     1<<16 // 64KB

static int send_timeout       = 3; // socket write timeout in seconds
static int connect_timeout    = 5; // connect timeout in seconds
static int connection_backlog = 3; // max queue length for pending connections

static pthread_attr_t thread_attr;

static void *worker_thread(void *arg);

enum {
   WORKER_THREAD_UNKNOWN,
   WORKER_THREAD_IDLE,
   WORKER_THREAD_READ,
   WORKER_THREAD_RECEIVE,
   WORKER_THREAD_LISTEN
};

static void
msleep(int ms) {
   struct timespec tm;

   tm.tv_sec = ms / 1000;
   tm.tv_nsec = (ms % 1000) * 1000000;

   nanosleep(&tm, NULL);
}

snl_socket_t *
snl_socket_new(int proto, SNL_EVENT_CB(*cb), void *data) {
   snl_socket_t *skt;

   if (!(skt = malloc(sizeof (snl_socket_t)))) {
      return (NULL);
   }

   memset(skt, 0, sizeof (snl_socket_t));
   skt->file_descriptor = -1;
   skt->protocol        = proto;
   skt->user_data       = data;
   skt->event_callback  = cb;

   if (pthread_create(&skt->worker_tid, &thread_attr, &worker_thread, skt)) {
      free(skt);
      return (NULL);
   }

   return (skt);
}

int
snl_socket_delete(snl_socket_t *skt) {
   // signal worker to stop
   skt->worker_stop = 1;

   snl_disconnect(skt);

   // pthread_join() will return an error, if it is
   // called from within the same thread. we can use
   // this, to detect that we are called from within
   // one of the worker thread callbacks.

   if (pthread_join(skt->worker_tid, NULL)) {
      // calling socket destructor from within worker,
      // detaching thread and committing suicide

      free(skt->data_buffer);
      free(skt);

      pthread_detach(skt->worker_tid);
      pthread_exit(NULL); // WILL NOT RETURN
   } else {
      // destructor was not called from thread callback,
      // wait for worker to terminate by itself

      while (!skt->worker_stoped) {
         msleep(5);
      }

      free(skt->data_buffer);
      free(skt);
   }

   return (SNL_ERROR_OK);
}

int
snl_accept(snl_socket_t *skt) {
   int fd, flg = 1, cnt = 1, ivl = 3, lng = 10;
   struct timeval sto;

   // hardcode send timeout
   sto.tv_sec  = send_timeout;
   sto.tv_usec = 0;

   // socket already in use
   if (skt->worker_type != WORKER_THREAD_UNKNOWN) {
      return (SNL_ERROR_BUSY);
   }

   fd = skt->file_descriptor;

   // set all kinds of fancy socket options
   if ((skt->protocol == SNL_PROTO_MSG) || (skt->protocol == SNL_PROTO_TCP)) {
      setsockopt(fd, SOL_SOCKET,  SO_SNDTIMEO,   &sto, sizeof (sto));
      setsockopt(fd, SOL_SOCKET,  SO_KEEPALIVE,  &flg, sizeof (flg));
      setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,   &cnt, sizeof (cnt));
      setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,  &ivl, sizeof (ivl));
      setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &ivl, sizeof (ivl));
      setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,   &flg, sizeof (flg));
      setsockopt(fd, IPPROTO_TCP, TCP_LINGER2,   &lng, sizeof (lng));
   }

   skt->worker_type = WORKER_THREAD_READ;

   return (SNL_ERROR_OK);
}

int
snl_send(snl_socket_t *skt, const void *buf, unsigned int len) {
   int error = SNL_ERROR_OK;
   unsigned int length;
   int on = 1, off = 0;

   if (skt->protocol == SNL_PROTO_UDP) {
      // check for packet size overflow
      if (len > UDP_PAYLOAD_SIZE) {
         return (SNL_ERROR_SEND);
      }

      if (send(skt->file_descriptor, buf, len, 0) != (int)len) {
         error = SNL_ERROR_SEND;
      } else {
         // update stats
         skt->xfer_sent += len;
      }

      return (error);
   }

   // save real packet length
   length = len;

   // convert packet length to network byte order
   len = htonl(len);

   // disable sending of partial frames
   setsockopt(skt->file_descriptor, IPPROTO_TCP, TCP_CORK, &on, sizeof (on));

   if (skt->protocol != SNL_PROTO_TCP) {
      // send packet header
      if (snl_write(skt->file_descriptor, &len, sizeof (len))) {
         error = SNL_ERROR_CLOSED;
         goto cleanup;
      }
   }

   // send packet payload
   if (snl_write(skt->file_descriptor, buf, length)) {
      error = SNL_ERROR_CLOSED;
      goto cleanup;
   }

   // update stats
   skt->xfer_sent += length;

cleanup:

   // flush send buffer
   setsockopt(skt->file_descriptor, IPPROTO_TCP, TCP_CORK, &off, sizeof (off));

   return (error);
}

int
snl_write(int fd, const void *buf, unsigned int len) {
   unsigned int remaining = len;
   char *ptr = (char *)buf;
	int written;

   while (remaining) {
      if ((written = write(fd, ptr, remaining)) == -1) {
         if (errno == EINTR) continue;
         return (SNL_ERROR_SEND);
      }

      ptr += written;
      remaining -= written;
   }

   return (SNL_ERROR_OK);
}

int
snl_listen(snl_socket_t *skt, unsigned short port) {
   int type = (skt->protocol == SNL_PROTO_UDP) ? SOCK_DGRAM : SOCK_STREAM;
   int error = SNL_ERROR_OK, flg = 1, fd = -1;
   struct sockaddr_in addr;

   // socket already in use
   if (skt->worker_type != WORKER_THREAD_UNKNOWN) {
      return (SNL_ERROR_BUSY);
   }

   // sanity check
   if (!port) {
      return (SNL_ERROR_LISTEN);
   }

   // open socket
   if ((fd = socket(AF_INET, type, 0)) < 0) {
      error = SNL_ERROR_OPEN;
      goto cleanup;
   }

   // set reuse address flag
   if ((skt->protocol == SNL_PROTO_MSG) || (skt->protocol == SNL_PROTO_TCP)) {
      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flg, sizeof (flg));
   }

   // set non blocking
   fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);

   // prepare socket address
   memset(&addr, 0, sizeof (addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons((int)port);
   addr.sin_addr.s_addr = htonl(INADDR_ANY);

   // bind the socket
   if (bind(fd, (SA *)&addr, sizeof (addr))) {
      error = SNL_ERROR_BIND;
      goto cleanup;
   }

   if ((skt->protocol == SNL_PROTO_MSG) || (skt->protocol == SNL_PROTO_TCP)) {
      if (listen(fd, connection_backlog)) {
         error = SNL_ERROR_LISTEN;
         goto cleanup;
      }
   }

cleanup:

   if (error && (fd >= 0)) {
      close(fd);

      return (error);
   }

   if (fd < 0) return (SNL_ERROR_LISTEN);

   skt->file_descriptor = fd;

   switch (skt->protocol) {
      case SNL_PROTO_TCP:
      case SNL_PROTO_MSG:
         skt->worker_type = WORKER_THREAD_LISTEN;
      break;

      case SNL_PROTO_UDP:
         skt->worker_type = WORKER_THREAD_RECEIVE;
      break;
   }

   return (SNL_ERROR_OK);
}

const char *
snl_error_string(int error) {
   switch (error) {
      case SNL_ERROR_OK:         return ("no error");
      case SNL_ERROR_OPEN:       return ("couldn't open socket");
      case SNL_ERROR_CONNECT:    return ("connecting to remote socket failed");
      case SNL_ERROR_LISTEN:     return ("error while listening on socket");
      case SNL_ERROR_BIND:       return ("couldn't bind to socket");
      case SNL_ERROR_ACCEPT:     return ("error while accepting connection");
      case SNL_ERROR_RECEIVE:    return ("couldn't read from socket");
      case SNL_ERROR_SEND:       return ("failed to send datagram");
      case SNL_ERROR_CLOSED:     return ("peer closed connection");
      case SNL_ERROR_BUFFER:     return ("out of memory");
      case SNL_ERROR_ADDRESS:    return ("hostname resolution failed");
      case SNL_ERROR_DISCONNECT: return ("error while closing socket");
      case SNL_ERROR_PROTOCOL:   return ("protocol mismatch");
      case SNL_ERROR_THREAD:     return ("could not start worker thread");
      case SNL_ERROR_TIMEOUT:    return ("timeout error");
      case SNL_ERROR_BUSY:       return ("socket already in use");
   }

   return ("unknown error");
}

int
snl_connect(snl_socket_t *skt, const char *host, unsigned short port) {
   int type = (skt->protocol == SNL_PROTO_UDP) ? SOCK_DGRAM : SOCK_STREAM;
   int fd, error = SNL_ERROR_OK, flg = 1, cnt = 1, ivl = 3;
   socklen_t len = sizeof (struct timeval);
   char addrstr[INET_ADDRSTRLEN];
   struct hostent *hent = NULL;
   struct timeval to, cto, sto;
   struct sockaddr_in addr;
   int broadcast = 0;

   // set connect timeout
   cto.tv_sec  = connect_timeout;
   cto.tv_usec = 0;

   // set send timeout
   sto.tv_sec  = send_timeout;
   sto.tv_usec = 0;

   // socket already in use
   if (skt->worker_type != WORKER_THREAD_UNKNOWN) {
      return (SNL_ERROR_BUSY);
   }

   // sanity check
   if (!port) {
      return (SNL_ERROR_CONNECT);
   }

   // check, if we should broadcast
   if (!host) {
      if (skt->protocol == SNL_PROTO_UDP) broadcast = 1;
      if ((skt->protocol == SNL_PROTO_MSG) || (skt->protocol == SNL_PROTO_TCP)) {
         return (SNL_ERROR_CONNECT);
      }
   }

   // open socket
   if ((fd = socket(AF_INET, type, 0)) < 0) {
      error = SNL_ERROR_OPEN;
      goto cleanup;
   }

   // set all kinds of fancy socket options
   if ((skt->protocol == SNL_PROTO_MSG) || (skt->protocol == SNL_PROTO_TCP)) {
      setsockopt(fd, SOL_SOCKET,  SO_SNDTIMEO,   &sto, sizeof (sto));
      setsockopt(fd, SOL_SOCKET,  SO_KEEPALIVE,  &flg, sizeof (flg));
      setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,   &cnt, sizeof (cnt));
      setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,  &ivl, sizeof (ivl));
      setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &ivl, sizeof (ivl));
      setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,   &flg, sizeof (flg));
   }

   if (broadcast) {
      setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &flg, sizeof (flg));
   }

   // prepare socket address
   memset(&addr, 0, sizeof (addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons((int)port);

   if (broadcast) {
      // set destination address directly to the broadcast address
      addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
   } else {
      // try to resolve the hostname
      if (!(hent = gethostbyname(host))) {
         error = SNL_ERROR_ADDRESS;
         goto cleanup;
      }
      if (!inet_ntop(AF_INET, hent->h_addr, addrstr, sizeof (addrstr))) {
         error = SNL_ERROR_CONNECT;
         goto cleanup;
      }
      if (!inet_pton(AF_INET, addrstr, &addr.sin_addr)) {
         error = SNL_ERROR_CONNECT;
         goto cleanup;
      }
   }

   if ((skt->protocol == SNL_PROTO_MSG) || (skt->protocol == SNL_PROTO_TCP)) {
      // save default connect timeout
      getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &to, &len);
      // set new (shorter) connect timeout
      setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &cto, sizeof (cto));
   }

   // connect socket
   while ((connect(fd, (SA *)&addr, sizeof (addr)) == -1) && (errno != EISCONN)) {
      if (errno != EINTR ) {
         error = SNL_ERROR_CONNECT;
         goto cleanup;
      }
   }

   // reset socket connect timeout
   if ((skt->protocol == SNL_PROTO_MSG) || (skt->protocol == SNL_PROTO_TCP)) {
      setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof (to));
   }

   // trigger worker thread
   switch (skt->protocol) {
      case SNL_PROTO_TCP:
      case SNL_PROTO_MSG:
         skt->worker_type = WORKER_THREAD_READ;
      break;
      case SNL_PROTO_UDP:
         skt->worker_type = WORKER_THREAD_IDLE;
      break;
   }

cleanup:

   if (error && (fd >= 0)) close(fd);

   skt->file_descriptor = fd;

   return (error);
}

int
snl_disconnect(snl_socket_t *skt) {
   shutdown(skt->file_descriptor, SHUT_RDWR);

   if (close(skt->file_descriptor)) return (SNL_ERROR_DISCONNECT);

   return (SNL_ERROR_OK);
}

int
snl_init(void) {
   // catch SIGPIPE to avoid aborting on broken pipes.
   signal(SIGPIPE, SIG_IGN);

   // set per thread stack size
   pthread_attr_init(&thread_attr);
   pthread_attr_setstacksize(&thread_attr, 4 * 65536); // 256 KB

   return (0);
}

static void *
worker_thread(void *arg) {
   int remaining, received, new_fd, max_fd, fd, error;
   snl_socket_t *skt = (snl_socket_t *)arg;
   struct sockaddr_in addr;
   unsigned int length;
   struct sockaddr *sa;
   struct timeval tv;
   socklen_t len;
   fd_set fds;
   char *ptr;

worker_start:

   error = SNL_ERROR_OK;
   max_fd = -1;

   // wait for worker thread to get the right type
   while (skt->worker_type == WORKER_THREAD_UNKNOWN) {
      if (skt->worker_stop) {
         skt->worker_stoped = 1;
         return (NULL);
      }
      msleep(5);
   }

   switch (skt->worker_type) {

      default:
         // TODO use condition signal instead
         while (!skt->worker_stop) {
            msleep(5);
         }

         goto worker_stop;
      break;

      case WORKER_THREAD_READ:
         // set initial buffer size
         skt->buffer_length = INITIAL_PAYLOAD_SIZE;

         // allocate buffer for received data
         if (!(skt->data_buffer = (char *)realloc(skt->data_buffer, skt->buffer_length))) {
            error = SNL_ERROR_BUFFER;
            goto worker_stop;
         }

         fd = skt->file_descriptor;

         // we repeat until the connection has been closed
         while (!skt->worker_stop) {
            if (skt->protocol == SNL_PROTO_TCP) {
               // read one line
               length = 0;
               ptr = skt->data_buffer;
               received = read(fd, ptr, skt->buffer_length);
               if (received == 0) {
                  error = SNL_ERROR_CLOSED;
                  goto worker_stop;
               } else if (received < 0) {
                  if (errno == EINTR) continue;

                  error = SNL_ERROR_RECEIVE;
                  goto worker_stop;
               } else {
                  length = received;
               }
            } else {
               // read length of next datagram
               ptr = (char *)&length;
               remaining = sizeof (length);
               while (remaining) {
                  received = read(fd, ptr, remaining);

                  if (received == 0) {
                     error = SNL_ERROR_CLOSED;
                     goto worker_stop;
                  } else if (received < 0) {
                     if (errno == EINTR) continue;

                     error = SNL_ERROR_RECEIVE;
                     goto worker_stop;
                  } else {
                     ptr += received;
                     remaining -= received;
                  }
               }

               // convert back to host byte order
               length = ntohl(length);

               // increase buffer size if necessary
               if (length > skt->buffer_length) {
                  skt->buffer_length = length * 2;
                  if (!(skt->data_buffer = realloc(skt->data_buffer, skt->buffer_length))) {
                     error = SNL_ERROR_BUFFER;
                     goto worker_stop;
                  }
               }

               // read datagram
               ptr = skt->data_buffer;
               remaining = length;
               while (remaining) {
                  received = read(fd, ptr, remaining);

                  if (received == 0) {
                     error = SNL_ERROR_CLOSED;
                     goto worker_stop;
                  } else if (received < 0) {
                     if (errno == EINTR) continue;

                     error = SNL_ERROR_RECEIVE;
                     goto worker_stop;
                  } else {
                     ptr += received;
                     remaining -= received;
                  }
               }
            }

            if (skt->worker_stop) {
               goto worker_stop;
            }

            // update counter
            skt->xfer_rcvd += length;

            skt->error_code = SNL_ERROR_OK;
            skt->event_code = SNL_EVENT_RECEIVE;
            skt->data_length = length;

            skt->event_callback(skt);
         }
      break;

      case WORKER_THREAD_LISTEN:
         memset(&addr, 0, sizeof (addr));

         fd = skt->file_descriptor;

         // wait for connections
         while (!skt->worker_stop) {
            tv.tv_sec = 0;
            tv.tv_usec = 5000; // 5 ms

            FD_ZERO(&fds);
            if (fd >= 0) {
               FD_SET(fd, &fds);
               if (fd > max_fd) max_fd = fd;
            }

            // test if blocking in accept would time out
            if ((select(max_fd + 1, &fds, NULL, NULL, &tv)) <= 0) continue;

            if (skt->worker_stop) {
               goto worker_stop;
            }

            sa = (SA *)&addr; len = sizeof (addr);
            if (fd >= 0 && FD_ISSET(fd, &fds)) {
               new_fd = accept(fd, sa, &len);

               if (new_fd < 0) {
                  if ((errno == EAGAIN) || (errno == EINTR)) continue;

                  skt->error_code = SNL_ERROR_ACCEPT;
                  skt->event_code = SNL_EVENT_ERROR;
               } else {
                  skt->error_code = SNL_ERROR_OK;
                  skt->event_code = SNL_EVENT_ACCEPT;

                  skt->client_port = addr.sin_port;
                  skt->client_ip = ntohl(addr.sin_addr.s_addr);
                  skt->client_fd = new_fd;
               }

               skt->event_callback(skt);
            }
         }
      break;

      case WORKER_THREAD_RECEIVE:
         memset(&addr, 0, sizeof (addr));

         fd = skt->file_descriptor;

         // set buffer size to maximum size of udp datagrams
         skt->buffer_length = UDP_PAYLOAD_SIZE;

         // allocate buffer for received data
         if (!(skt->data_buffer = realloc(skt->data_buffer, skt->buffer_length))) {
            error = SNL_ERROR_BUFFER;
            goto worker_stop;
         }

         // wait for messages
         while (!skt->worker_stop) {
            tv.tv_sec = 0;
            tv.tv_usec = 5000; // 5 ms

            FD_ZERO(&fds);
            if (fd >= 0) {
               FD_SET(fd, &fds);
               if (fd > max_fd) max_fd = fd;
            }

            // test if blocking in recvfrom would time out
            if ((select(max_fd + 1, &fds, NULL, NULL, &tv)) <= 0) continue;

            if (skt->worker_stop) {
               goto worker_stop;
            }

            sa = (SA *)&addr; len = sizeof (addr);
            if (fd >= 0 && FD_ISSET(fd, &fds)) {
               received = recvfrom(fd, skt->data_buffer, skt->buffer_length, 0, sa, &len);

               if (received < 0) {
                  if ((errno == EAGAIN) || (errno == EINTR)) continue;

                  skt->error_code = SNL_ERROR_RECEIVE;
                  skt->event_code = SNL_EVENT_ERROR;
               } else {
                  length = received;

                  skt->client_port = addr.sin_port;
                  skt->client_ip = ntohl(addr.sin_addr.s_addr);
                  skt->client_fd = fd;

                  // update counter
                  skt->xfer_rcvd += length;

                  skt->error_code = SNL_ERROR_OK;
                  skt->event_code = SNL_EVENT_RECEIVE;

                  skt->data_length = length;
               }
 
               skt->event_callback(skt);
            }
         }
      break;

   } // switch

worker_stop:

   skt->worker_type = WORKER_THREAD_UNKNOWN;

   if (error && !skt->worker_stop) {
      skt->error_code = error;
      skt->event_code = SNL_EVENT_ERROR;
      skt->event_callback(skt);
   }

   goto worker_start;

   return (NULL);
}

#undef SA
