/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */


#ifndef __LWIP_SOCKETS_H__
#define __LWIP_SOCKETS_H__

#include "opt.h"

#if LWIP_SOCKET /* don't build if not configured for use in lwipopts.h */

#include <stddef.h> /* for size_t */
#include "ip_addr.h"
#include "inet.h"
#include "inet6.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A callback prototype to inform about events for a socket */
typedef void (* socket_callback)(int s, int evt, u16_t len);

//add by shimingrui 220711 编译
typedef void * sys_thread_t;

/* members are in network byte order */
struct sockaddr_in {
  u8_t sin_len;
  u8_t sin_family;
  u16_t sin_port;
  struct in_addr sin_addr;
#define SIN_ZERO_LEN 8
  char sin_zero[SIN_ZERO_LEN];
};

#if LWIP_IPV6
struct sockaddr_in6 {
  u8_t sin6_len;             /* length of this structure */
  u8_t sin6_family;          /* AF_INET6                 */
  u16_t sin6_port;           /* Transport layer port #   */
  u32_t sin6_flowinfo;       /* IPv6 flow information    */
  struct in6_addr sin6_addr; /* IPv6 address             */
};
#endif /* LWIP_IPV6 */

struct sockaddr {
  u8_t sa_len;
  u8_t sa_family;
#if LWIP_IPV6
  u8_t sa_data[22];
#else /* LWIP_IPV6 */
  u8_t sa_data[14];
#endif /* LWIP_IPV6 */
};

struct sockaddr_storage {
  u8_t        s2_len;
  u8_t        ss_family;
  char        s2_data1[2];
  u32_t       s2_data2[3];
#if LWIP_IPV6
  u32_t       s2_data3[3];
#endif /* LWIP_IPV6 */
};

/* If your port already typedef's socklen_t, define SOCKLEN_T_DEFINED
   to prevent this code from redefining it. */
#if !defined(socklen_t) && !defined(SOCKLEN_T_DEFINED)
typedef u32_t socklen_t;
#endif

#if !defined IOV_MAX
#define IOV_MAX 0xFFFF
#elif IOV_MAX > 0xFFFF
#error "IOV_MAX larger than supported by LwIP"
#endif /* IOV_MAX */

#if !defined(iovec)
struct iovec {
  void  *iov_base;
  size_t iov_len;
};
#endif

#if !defined(msghdr)
struct msghdr {
  void         *msg_name;
  socklen_t     msg_namelen;
  struct iovec *msg_iov;
  int           msg_iovlen;
  void         *msg_control;
  socklen_t     msg_controllen;
  int           msg_flags;
};
#endif

/* struct msghdr->msg_flags bit field values */
#define MSG_TRUNC   0x04
#define MSG_CTRUNC  0x08

struct in_pktinfo {
  unsigned int   ipi_ifindex;  /* Interface index */
  struct in_addr ipi_addr;     /* Destination (from header) address */
};

/* RFC 3542, Section 20: Ancillary Data */
struct cmsghdr {
  socklen_t  cmsg_len;   /* number of bytes, including header */
  int        cmsg_level; /* originating protocol */
  int        cmsg_type;  /* protocol-specific type */
};
/* Data section follows header and possible padding, typically referred to as
      unsigned char cmsg_data[]; */

/* cmsg header/data alignment. NOTE: we align to native word size (double word
size on 16-bit arch) so structures are not placed at an unaligned address.
16-bit arch needs double word to ensure 32-bit alignment because socklen_t
could be 32 bits. If we ever have cmsg data with a 64-bit variable, alignment
will need to increase long long */
#define ALIGN_H(size) (((size) + sizeof(long) - 1U) & ~(sizeof(long)-1U))
#define ALIGN_D(size) ALIGN_H(size)

#define CMSG_FIRSTHDR(mhdr) \
          ((mhdr)->msg_controllen >= sizeof(struct cmsghdr) ? \
           (struct cmsghdr *)(mhdr)->msg_control : \
           (struct cmsghdr *)NULL)

#define CMSG_NXTHDR(mhdr, cmsg) \
        (((cmsg) == NULL) ? CMSG_FIRSTHDR(mhdr) : \
         (((u8_t *)(cmsg) + ALIGN_H((cmsg)->cmsg_len) \
                            + ALIGN_D(sizeof(struct cmsghdr)) > \
           (u8_t *)((mhdr)->msg_control) + (mhdr)->msg_controllen) ? \
          (struct cmsghdr *)NULL : \
          (struct cmsghdr *)((void*)((u8_t *)(cmsg) + \
                                      ALIGN_H((cmsg)->cmsg_len)))))

#define CMSG_DATA(cmsg) ((void*)((u8_t *)(cmsg) + \
                         ALIGN_D(sizeof(struct cmsghdr))))

#define CMSG_SPACE(length) (ALIGN_D(sizeof(struct cmsghdr)) + \
                            ALIGN_H(length))

#define CMSG_LEN(length) (ALIGN_D(sizeof(struct cmsghdr)) + \
                           length)

/* Socket protocol types (TCP/UDP/RAW) */
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3

/*
 * Option flags per-socket. These must match the SOF_ flags in ip.h (checked in init.c)
 */
#define SO_DEBUG        0x0001 /* Unimplemented: turn on debugging info recording */
#define SO_ACCEPTCONN   0x0002 /* socket has had listen() */
#define SO_REUSEADDR    0x0004 /* Allow local address reuse */
#define SO_KEEPALIVE    0x0008 /* keep connections alive */
#define SO_DONTROUTE    0x0010 /* Unimplemented: just use interface addresses */
#define SO_BROADCAST    0x0020 /* permit to send and to receive broadcast messages (see IP_SOF_BROADCAST option) */
#define SO_USELOOPBACK  0x0040 /* Unimplemented: bypass hardware when possible */
#define SO_LINGER       0x0080 /* linger on close if data present */
#define SO_DONTLINGER   ((int)(~SO_LINGER))
#define SO_OOBINLINE    0x0100 /* Unimplemented: leave received OOB data in line */
#define SO_REUSEPORT    0x0200 /* Unimplemented: allow local address & port reuse */
#define SO_IPSEC        0x0400 /* denote ipsec using */
#define SO_FLOWCTRL     0x0800 /* denote flow control using */

#define SO_SNDBUF       0x1001  /* Unimplemented: send buffer size */
#define SO_RCVBUF       0x1002  /* receive buffer size */
#define SO_SNDLOWAT     0x1003  /* Unimplemented: send low-water mark */
#define SO_RCVLOWAT     0x1004  /* Unimplemented: receive low-water mark */
#define SO_SNDTIMEO     0x1005  /* Unimplemented: send timeout */
#define SO_RCVTIMEO     0x1006  /* receive timeout */
#define SO_ERROR        0x1007  /* get error status and clear */
#define SO_TYPE         0x1008  /* get socket type */
#define SO_CONTIMEO     0x1009  /* Unimplemented: connect timeout */
#define SO_NO_CHECK     0x100a  /* don't create UDP checksum */
#define SO_BINDTODEVICE 0x100b /* bind to device */
#define SO_BIO          0x100c  /* set socket into blocking mode */
#define SO_NBIO         0x100d  /* set socket into non-blocking mode */
#define SO_NONBLOCK     0x100e  /* set/get socket blocking mode via optval param*/

/*
 * Structure used for manipulating linger option.
 */
struct linger {
   int l_onoff;                /* option on/off */
   int l_linger;               /* linger time , second as unit*/
};

/*
 * Structure used for get tcp_info.
 */
struct tcp_info {
    u8_t tcpi_state;      //tcp_pcb state: SYN_SENT, ESTABLISHED, etc, see tcp_state enum define in tcp.h
    u8_t tcpi_nrtx;       //tcp_pcb nrtx: number of retransmissions
    u8_t tcpi_dupacks;    //tcp_pcb dupacks: number of dupacks
    u8_t tcpi_last_timer; //tcp_pcb last_timer
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define  SOL_SOCKET  0xfff    /* options for socket level */


#define AF_UNSPEC       0
#define AF_INET         2
#if LWIP_IPV6
#define AF_INET6        10
#else /* LWIP_IPV6 */
#define AF_INET6        AF_UNSPEC
#endif /* LWIP_IPV6 */
#define PF_INET         AF_INET
#define PF_INET6        AF_INET6
#define PF_UNSPEC       AF_UNSPEC

#define IPPROTO_IP      0
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17
#if LWIP_IPV6
#define IPPROTO_IPV6    41
#endif /* LWIP_IPV6 */
#define IPPROTO_UDPLITE 136

/* Flags we can use with send and recv. */
#define MSG_PEEK       0x01    /* Peeks at an incoming message */
#define MSG_WAITALL    0x02    /* Unimplemented: Requests that the function block until the full amount of data requested can be returned */
#define MSG_OOB        0x04    /* Unimplemented: Requests out-of-band data. The significance and semantics of out-of-band data are protocol-specific */
#define MSG_DONTWAIT   0x08    /* Nonblocking i/o for this operation only */
#define MSG_MORE       0x10    /* Sender will send more */


/*
 * Options for level IPPROTO_IP
 */
#define IP_TOS             1
#define IP_TTL             2
#define IP_PKTINFO         8

#if LWIP_TCP
/*
 * Options for level IPPROTO_TCP
 */
#define TCP_NODELAY         0x01    /* don't delay send to coalesce packets */
#define TCP_KEEPALIVE       0x02    /* send KEEPALIVE probes when idle for pcb->keep_idle milliseconds */
#define TCP_KEEPIDLE        0x03    /* set pcb->keep_idle  - Same as TCP_KEEPALIVE, but use seconds for get/setsockopt */
#define TCP_KEEPINTVL       0x04    /* set pcb->keep_intvl - Use seconds for get/setsockopt */
#define TCP_KEEPCNT         0x05    /* set pcb->keep_cnt   - Use number of probes sent for get/setsockopt */
#define TCP_TIMEROUT        0x06    /* set pcb->TCP_TIMEROUT   - Use number of probes sent for get/setsockopt */
#define TCP_MSS_VALUE       0x07    /* set pcb->mss   - Use number of probes only for get/setsockopt */
#define TCP_RTO_VALUE       0x08    /* set pcb->rto   - Use number of probes only for get/setsockopt */
#define TCP_TXB_UNACK       0x09    /* set (pcb->snd_lbb - pcb->lastack) - how much byte remaind in buffer for unacked */
#define TCP_TXB_REST        0x0A    /* set (pcb->snd_buf) - how much byte remaind in tx buffer can writen by app */
#define TCP_TXB_ACKED       0x0B    /* set (pcb->acked_sum) - the number of accumulation of acked */
#define TCP_CLOSE_TIMEROUT  0x0C    /* set (pcb->close_timeout) - the timerout set of tcp close all */
#define TCP_RST_TRIGGER     0x0D    /* set reset man trigger - call tcp_reset_man_trigger() */
#define TCP_INFO            0x0E    /* get tcp_info */
#define TCP_MAX_NRTX        0x0F    /* set (pcb->nrtx_max) - max nrtx for tcp */
#define TCP_MAX_SYN_NRTX    0x10    /* set (pcb->nrtx_syn_max) - max syn nrtx for tcp */

#endif /* LWIP_TCP */

#if LWIP_IPV6
/*
 * Options for level IPPROTO_IPV6
 */
#define IPV6_V6ONLY 27 /* RFC3493: boolean control to restrict AF_INET6 sockets to IPv6 communications only. */
#endif /* LWIP_IPV6 */

#if LWIP_UDP && LWIP_UDPLITE
/*
 * Options for level IPPROTO_UDPLITE
 */
#define UDPLITE_SEND_CSCOV 0x01 /* sender checksum coverage */
#define UDPLITE_RECV_CSCOV 0x02 /* minimal receiver checksum coverage */
#endif /* LWIP_UDP && LWIP_UDPLITE*/


#if LWIP_IGMP
/*
 * Options and types for UDP multicast traffic handling
 */
#define IP_ADD_MEMBERSHIP  3
#define IP_DROP_MEMBERSHIP 4
#define IP_MULTICAST_TTL   5
#define IP_MULTICAST_IF    6
#define IP_MULTICAST_LOOP  7

typedef struct ip_mreq {
    struct in_addr imr_multiaddr; /* IP multicast address of group */
    struct in_addr imr_interface; /* local IP address of interface */
} ip_mreq;
#endif /* LWIP_IGMP */

/*
 * The Type of Service provides an indication of the abstract
 * parameters of the quality of service desired.  These parameters are
 * to be used to guide the selection of the actual service parameters
 * when transmitting a datagram through a particular network.  Several
 * networks offer service precedence, which somehow treats high
 * precedence traffic as more important than other traffic (generally
 * by accepting only traffic above a certain precedence at time of high
 * load).  The major choice is a three way tradeoff between low-delay,
 * high-reliability, and high-throughput.
 * The use of the Delay, Throughput, and Reliability indications may
 * increase the cost (in some sense) of the service.  In many networks
 * better performance for one of these parameters is coupled with worse
 * performance on another.  Except for very unusual cases at most two
 * of these three indications should be set.
 */
#define IPTOS_TOS_MASK          0x1E
#define IPTOS_TOS(tos)          ((tos) & IPTOS_TOS_MASK)
#define IPTOS_LOWDELAY          0x10
#define IPTOS_THROUGHPUT        0x08
#define IPTOS_RELIABILITY       0x04
#define IPTOS_LOWCOST           0x02
#define IPTOS_MINCOST           IPTOS_LOWCOST

/*
 * The Network Control precedence designation is intended to be used
 * within a network only.  The actual use and control of that
 * designation is up to each network. The Internetwork Control
 * designation is intended for use by gateway control originators only.
 * If the actual use of these precedence designations is of concern to
 * a particular network, it is the responsibility of that network to
 * control the access to, and use of, those precedence designations.
 */
#define IPTOS_PREC_MASK                 0xe0
#define IPTOS_PREC(tos)                ((tos) & IPTOS_PREC_MASK)
#define IPTOS_PREC_NETCONTROL           0xe0
#define IPTOS_PREC_INTERNETCONTROL      0xc0
#define IPTOS_PREC_CRITIC_ECP           0xa0
#define IPTOS_PREC_FLASHOVERRIDE        0x80
#define IPTOS_PREC_FLASH                0x60
#define IPTOS_PREC_IMMEDIATE            0x40
#define IPTOS_PREC_PRIORITY             0x20
#define IPTOS_PREC_ROUTINE              0x00


/*
 * Commands for ioctlsocket(),  taken from the BSD file fcntl.h.
 * lwip_ioctl only supports FIONREAD and FIONBIO, for now
 *
 * Ioctl's have the command encoded in the lower word,
 * and the size of any in or out parameters in the upper
 * word.  The high 2 bits of the upper word are used
 * to encode the in/out status of the parameter; for now
 * we restrict parameters to at most 128 bytes.
 */
#if !defined(FIONREAD) || !defined(FIONBIO)
#define IOCPARM_MASK    0x7fU           /* parameters must be < 128 bytes */
#define IOC_VOID        0x20000000UL    /* no parameters */
#define IOC_OUT         0x40000000UL    /* copy out parameters */
#define IOC_IN          0x80000000UL    /* copy in parameters */
#define IOC_INOUT       (IOC_IN|IOC_OUT)
                                        /* 0x20000000 distinguishes new &
                                           old ioctl's */
#define _IO(x,y)        (IOC_VOID|((x)<<8)|(y))

#define _IOR(x,y,t)     (IOC_OUT|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))

#define _IOW(x,y,t)     (IOC_IN|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))
#endif /* !defined(FIONREAD) || !defined(FIONBIO) */

#ifndef FIONREAD
#define FIONREAD    _IOR('f', 127, unsigned long) /* get # bytes to read */
#endif
#ifndef FIONBIO
#define FIONBIO     _IOW('f', 126, unsigned long) /* set/clear non-blocking i/o */
#endif

/* Socket I/O Controls: unimplemented */
#ifndef SIOCSHIWAT
#define SIOCSHIWAT  _IOW('s',  0, unsigned long)  /* set high watermark */
#define SIOCGHIWAT  _IOR('s',  1, unsigned long)  /* get high watermark */
#define SIOCSLOWAT  _IOW('s',  2, unsigned long)  /* set low watermark */
#define SIOCGLOWAT  _IOR('s',  3, unsigned long)  /* get low watermark */
#define SIOCATMARK  _IOR('s',  7, unsigned long)  /* at oob mark? */
#endif

/* commands for fnctl */
#ifndef F_GETFL
#define F_GETFL 3
#endif
#ifndef F_SETFL
#define F_SETFL 4
#endif

/* File status flags and file access modes for fnctl,
   these are bits in an int. */
#ifndef O_NONBLOCK
#define O_NONBLOCK  1 /* nonblocking I/O */
#endif
#ifndef O_NDELAY
#define O_NDELAY    1 /* same as O_NONBLOCK, for compatibility */
#endif

#ifndef SHUT_RD
  #define SHUT_RD   0
  #define SHUT_WR   1
  #define SHUT_RDWR 2
#endif

#define NUM_SOCKETS         MEMP_NUM_NETCONN
#define NUM_EVENTS          MEMP_NUM_NETCONN
#define LWIP_SOCKET_OFFSET  (0) /*note: must set to 0*/
#define LWIP_EVENT_OFFSET   (NUM_SOCKETS + LWIP_SOCKET_OFFSET)

/* FD_SET used for lwip_select */
#ifndef FD_SET
#undef  FD_SETSIZE
/* Make FD_SETSIZE match NUM_SOCKETS in socket.c */
#define FD_SETSIZE    MEMP_NUM_NETCONN
#define FDSETSAFESET(n, code)  do {\
    if ((((int)(n) - LWIP_SOCKET_OFFSET) < (MEMP_NUM_NETCONN * 2)) \
       && (((int)(n) - LWIP_SOCKET_OFFSET) >= 0)) {code;}\
    } while(0)
#define FDSETSAFEGET(n, code) (((((int)(n) - LWIP_SOCKET_OFFSET) < (MEMP_NUM_NETCONN * 2)) \
       && (((int)(n) - LWIP_SOCKET_OFFSET) >= 0)) ? (code) : 0)

#define FD_SET(n, p)  FDSETSAFESET(n, (p)->fd_bits[((n) - LWIP_SOCKET_OFFSET)/8] |=  (1 << (((n) - LWIP_SOCKET_OFFSET) & 7)))
#define FD_CLR(n, p)  FDSETSAFESET(n, (p)->fd_bits[((n) - LWIP_SOCKET_OFFSET)/8] &= ~(1 << (((n) - LWIP_SOCKET_OFFSET) & 7)))
#define FD_ISSET(n,p) FDSETSAFEGET(n, (p)->fd_bits[((n) - LWIP_SOCKET_OFFSET)/8] &   (1 << (((n) - LWIP_SOCKET_OFFSET) & 7)))
#define FD_ZERO(p)    memset((void*)(p),0,sizeof(*(p)))

typedef struct fd_set {
  unsigned char fd_bits [(FD_SETSIZE * 2 + 7)/8];
} fd_set;

#endif /* FD_SET */

/** LWIP_TIMEVAL_PRIVATE: if you want to use the struct timeval provided
 * by your system, set this to 0 and include <sys/time.h> in cc.h */
#ifndef LWIP_TIMEVAL_PRIVATE
#define LWIP_TIMEVAL_PRIVATE 1
#endif

#ifndef _TIMEVAL_DEFINED
#define _TIMEVAL_DEFINED
struct timeval
{
	int tv_sec;        /* seconds */
	int tv_usec;       /* and microseconds */
};
#endif /* LWIP_TIMEVAL_PRIVATE */

void lwip_socket_init(void);
int lwip_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int lwip_bind(int s, const struct sockaddr *name, socklen_t namelen);
int lwip_shutdown(int s, int how);
int lwip_shutdown2(int s, int how, char *filename, int line);
int lwip_getpeername (int s, struct sockaddr *name, socklen_t *namelen);
int lwip_getsockname (int s, struct sockaddr *name, socklen_t *namelen);
int lwip_getsockopt (int s, int level, int optname, void *optval, socklen_t *optlen);
int lwip_setsockopt (int s, int level, int optname, const void *optval, socklen_t optlen);
int lwip_close(int s);
int lwip_close2(int s, char *filename, int line);
int lwip_connect(int s, const struct sockaddr *name, socklen_t namelen);
int lwip_listen(int s, int backlog);
int lwip_recv(int s, void *mem, size_t len, int flags);
int lwip_read(int s, void *mem, size_t len);
int lwip_recvfrom(int s, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
int lwip_send(int s, const void *dataptr, size_t size, int flags);
int lwip_sendto(int s, const void *dataptr, size_t size, int flags, const struct sockaddr *to, socklen_t tolen);
int lwip_socket(int domain, int type, int protocol);
int lwip_socket_with_callback(int domain, int type, int protocol, socket_callback callback);
int lwip_write(int s, const void *dataptr, size_t size);
int lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);
int lwip_ioctl(int s, long cmd, void *argp);
int lwip_fcntl(int s, int cmd, int val);
int lwip_eventfd(unsigned int initval, int flags);
int lwip_getthreaderrno(int s);
int lwip_getsockvalid(int s);
int lwip_getsockerrno(int s);
int lwip_getsocktype(int s);
struct netconn * lwip_getsocknetconn(int s);
struct tcp_pcb * lwip_getsocktcppcb(int s);
struct udp_pcb * lwip_getsockudppcb(int s);
int lwip_get_sock_localport(int s);
int lwip_get_sock_remoteport(int s);
int lwip_get_sock_isipv6(int s);
int lwip_get_sock_pdu_offset(int s);
int lwip_trigger_process(char *name, sys_thread_t threadid);
ipX_addr_t* lwip_get_sock_localipXaddr(int s);
ipX_addr_t* lwip_get_sock_remoteipXaddr(int s);

int readn(int fd, void *buf, size_t count);
int writen(int fd, const void *buf, size_t count);
int readline(int fd, void *buf, size_t maxline);

int lwip_readv(int s, const struct iovec *iov, int iovcnt);
int lwip_writev(int s, const struct iovec *iov, int iovcnt);
int lwip_recvmsg(int s, struct msghdr *message, int flags);
int lwip_sendmsg(int s, const struct msghdr *message, int flags);

#if LWIP_COMPAT_SOCKETS
#define accept(a,b,c)         lwip_accept(a,b,c)
#define bind(a,b,c)           lwip_bind(a,b,c)
#define shutdown(a,b)         lwip_shutdown(a,b)
#define closesocket(s)        lwip_close(s)
#define connect(a,b,c)        lwip_connect(a,b,c)
#define getsockname(a,b,c)    lwip_getsockname(a,b,c)
#define getpeername(a,b,c)    lwip_getpeername(a,b,c)
#define setsockopt(a,b,c,d,e) lwip_setsockopt(a,b,c,d,e)
#define getsockopt(a,b,c,d,e) lwip_getsockopt(a,b,c,d,e)
#define listen(a,b)           lwip_listen(a,b)
#define recv(a,b,c,d)         lwip_recv(a,b,c,d)
#define recvfrom(a,b,c,d,e,f) lwip_recvfrom(a,b,c,d,e,f)
#define send(a,b,c,d)         lwip_send(a,b,c,d)
#define sendto(a,b,c,d,e,f)   lwip_sendto(a,b,c,d,e,f)
#define socket(a,b,c)         lwip_socket(a,b,c)
#define select(a,b,c,d,e)     lwip_select(a,b,c,d,e)
#define eventfd(a, b)         lwip_eventfd(a,b)
#define ioctlsocket(a,b,c)    lwip_ioctl(a,b,c)
#define socket_with_callback(a,b,c,d)   lwip_socket_with_callback(a,b,c,d)

#define readv(s,iov,iovcnt)             lwip_readv(s,iov,iovcnt)
#define writev(s,iov,iovcnt)            lwip_writev(s,iov,iovcnt)
#define recvmsg(s,message,flags)        lwip_recvmsg(s,message,flags)
#define sendmsg(s,message,flags)        lwip_sendmsg(s,message,flags)

#if LWIP_POSIX_SOCKETS_IO_NAMES
#define read(a,b,c)           lwip_read(a,b,c)
#define write(a,b,c)          lwip_write(a,b,c)
#define close(s)              lwip_close(s)
#define fcntl(a,b,c)          lwip_fcntl(a,b,c)
#define eventfd(a, b)         lwip_eventfd(a,b)
#endif /* LWIP_POSIX_SOCKETS_IO_NAMES */

#endif /* LWIP_COMPAT_SOCKETS */

#undef  errno
//add by shimingrui 220713
extern int get_lwip_errno();
#define errno get_lwip_errno()
//#define errno lwip_errno
//extern u16_t lwip_errno;

#ifdef __cplusplus
}
#endif

#endif /* LWIP_SOCKET */

#endif /* __LWIP_SOCKETS_H__ */
