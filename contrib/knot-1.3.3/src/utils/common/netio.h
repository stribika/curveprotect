/*  Copyright (C) 2011 CZ.NIC, z.s.p.o. <knot-dns@labs.nic.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*!
 * \file netio.h
 *
 * \author Daniel Salzman <daniel.salzman@nic.cz>
 *
 * \brief Networking abstraction for utilities.
 *
 * \addtogroup knot_utils
 * @{
 */

#ifndef _UTILS__NETIO_H_
#define _UTILS__NETIO_H_

#include <stdint.h>			// uint_t
#include <netdb.h>			// addrinfo

#include "common/lists.h"		// node
#include "utils/common/params.h"	// params_t

/*! \brief Structure containing server information. */
typedef struct {
	/*! List node (for list container). */
	node	n;
	/*! Name or address of the server. */
	char	*name;
	/*! Name or number of the service. */
	char	*service;
} server_t;

typedef struct {
	/*! Socket descriptor. */
	int	sockfd;

	/*! IP protocol type. */
	int	iptype;
	/*! Socket type. */
	int	socktype;
	/*! Timeout for all network operations. */
	int	wait;

	/*! Local interface parameters. */
	const server_t *local;
	/*! Remote server parameters. */
	const server_t *remote;

	/*! Local description string (used for logging). */
	char *local_str;
	/*! Remote description string (used for logging). */
	char *remote_str;

	/*! Output from getaddrinfo for remote server. If the server is
	 *  specified using domain name, this structure may contain more
	 *  results.
	 */
	struct addrinfo *remote_info;
	/*! Currently used result from remote_info. */
	struct addrinfo *srv;
	/*! Output from getaddrinfo for local address. Only first result is
	 *  used.
	 */
	struct addrinfo *local_info;
} net_t;

/*!
 * \brief Creates and fills server structure.
 *
 * \param name		Address or host name.
 * \param service	Port number or service name.
 *
 * \retval server	if success.
 * \retval NULL		if error.
 */
server_t* server_create(const char *name, const char *service);

/*!
 * \brief Destroys server structure.
 *
 * \param server	Server structure to destroy.
 */
void server_free(server_t *server);

/*!
 * \brief Translates enum IP version type to int version.
 *
 * \param ip		IP version to convert.
 *
 * \retval AF_INET, AF_INET6 or AF_UNSPEC.
 */
int get_iptype(const ip_t ip);

/*!
 * \brief Translates enum IP protocol type to int version in context to the
 *        current DNS query type.
 *
 * \param proto		IP protocol type to convert.
 * \param type		DNS query type number.
 *
 * \retval SOCK_STREAM or SOCK_DGRAM.
 */
int get_socktype(const protocol_t proto, const uint16_t type);

/*!
 * \brief Translates int socket type to the common string one.
 *
 * \param socktype	Socket type (SOCK_STREAM or SOCK_DGRAM).
 *
 * \retval "TCP" or "UDP".
 */
const char* get_sockname(const int socktype);

/*!
 * \brief Initializes network structure and resolves local and remote addresses.
 *
 * \param local		Local address and service description.
 * \param remote	Remote address and service description.
 * \param iptype	IP version.
 * \param socktype	Socket type.
 * \param wait		Network timeout interval.
 * \param net		Network structure to initialize.
 *
 * \retval KNOT_EOK	if success.
 * \retval errcode	if error.
 */
int net_init(const server_t *local,
             const server_t *remote,
             const int      iptype,
             const int      socktype,
             const int      wait,
             net_t          *net);

/*!
 * \brief Creates socket and connects (if TCP) to remote address specified
 *        by net->srv.
 *
 * \param net		Connection parameters.
 *
 * \retval KNOT_EOK	if success.
 * \retval errcode	if error.
 */
int net_connect(net_t *net);

/*!
 * \brief Sends data to connected remote server.
 *
 * \param net		Connection parameters.
 * \param buf		Data to send.
 * \param buf_len	Length of the data to send.
 *
 * \retval KNOT_EOK	if success.
 * \retval errcode	if error.
 */
int net_send(const net_t *net, const uint8_t *buf, const size_t buf_len);

/*!
 * \brief Receives data from connected remote server.
 *
 * \param net		Connection parameters.
 * \param buf		Buffer for incomming data.
 * \param buf_len	Length of the buffer.
 *
 * \retval >=0		length of successfully received data.
 * \retval errcode	if error.
 */
int net_receive(const net_t *net, uint8_t *buf, const size_t buf_len);

/*!
 * \brief Closes current network connection.
 *
 * \param net		Connection parameters.
 */
void net_close(net_t *net);

/*!
 * \brief Cleans up network structure.
 *
 * \param net		Connection parameters.
 */
void net_clean(net_t *net);

#endif // _UTILS__NETIO_H_

/*! @} */
