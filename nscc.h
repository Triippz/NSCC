/************************************************************************************
*
*		AUTHOR:		Mark Tripoli
*		DATE:		28 - JAN - 2018
*		LICENSE:	GNU GPL v3 License
*		
*		Purpose:	This library is geared towards TCP/IP operations within
*					NonStop Guardian enviornments. Some of the code may need to be
*					tweaked, as this was geared towards compilation on a C++ compiler.
*					
*		Notes:		When utilizing this library, please bear in mind, except for 
*					initialization, you will be operating through the TCP structure
*					when making calls. It is similar to working with object-based code.
*					
*		PS:			Sorry if you don't like my "java-like" naming style.
*
*		REVISIONS:
*		VERSION		DATE	     COMMENTS
*		-------    ------       ---------------------------------------------------
*		1.0.0	  1/28/18		Initial Release 
*************************************************************************************/

#ifndef _NSCCH_INCLUDE_

#ifdef __TANDEM
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <socket.h>
#include <netdb.h>
#include <inet.h>
#include <time.h>
#include <stdint.h>
#include <cextdecs>
#include <route.h>
#include <if.h>
#include <in.h>
#include <in6.h>
#include <ioctl.h>
#else
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
/* fill in what you would like here....*/
#endif


/* Type Definitions */
/**
 * @var SERVER_ADDR
 * Holds the IP/Hostname of the connection
 * */
typedef char			SERVER_ADDR[20];
/**
 * @var INET_NAME
 * Holds the TCP/IP process name
 * */
typedef char			INET_NAME[9];
/**
 * @var SERVER_ADDR
 * Holds the port of the connection
 * */
typedef unsigned short	TCP_PORT;
/**
 * @var SERVER_ADDR
 * Holds an error message for logging
 * */
typedef char			ERROR_MESSAGE[128];
/**
 * @var SERVER_ADDR
 * The timeout value for NoWait operations
 * */
typedef signed long     TIMEOUT;

/**
 * @enum BOOLEAN
 * True/False values (0/1)
 * These ones below you may already have,
*  feel free to remove if you need to
*  if using C99 --> #include<stdbool.h> */
typedef enum {
    FAIL,
    SUCCESS
} BOOLEAN;

/* Defines */
/**
 * @def ERR_UNINIT_NW
 * The error value when a socket has created a NW socket
 * and attempts to perform a blocking call
 * */
#define                 ERR_UNINIT_NW 26
/**
 * @def ERR_TIMEOUT
 * The error returned from FILE_GETINFO_ when a socket times out
 * */
#define                 ERR_TIMEOUT   40


/***************************************************************
*
*	@struct		TCP_CONNECTION_OPTS
*	Purpose:	Sets the various timeouts which may be needed
*	            during NoWait operations. These values will
*	            be used for AWAITIOX calls after specific
*	            network operations.
*
*	            The "variable_to" is a flexible timeout that
*	            can either be used uniformly across the the
*	            call process, or for specific points
*
*	            TIMEOUT = signed long
*
***************************************************************/
typedef struct _timeout_opts
{
    TIMEOUT     recv_to;
    TIMEOUT     send_to;
    TIMEOUT     socket_to;
    TIMEOUT     connect_to;
    TIMEOUT     bind_to;
    TIMEOUT     accept_to;
    TIMEOUT     variable_to;
} TIMEOUT_OPTS;

/***************************************************************
*
*	@struct		TCP_CONNECTION_INFO
*	Purpose:	Contains all the specific information
*				which will be used during TCP/IP operations
*				Most of these you will set initially, but
*				you are free to change throughout your process
*				as needed.
*
***************************************************************/
typedef struct _tcp_connection_info
{
    SERVER_ADDR			ipaddr;
    TCP_PORT			port;
    INET_NAME		    process_name;
    int				    *sock;
    long				sockaddr_len;
    int				    flags;
    int				    queue_len;
    long				tag;
    struct sockaddr_in	*sockaddr;
    int				    sock_shutdown_how;
    TIMEOUT_OPTS        timeout_opts;
} TCP_CONNECTION_INFO;

/***************************************************************
*
*	@struct		TCP
*	Purpose:	Contains all the function pointers which will
*				be utilized throughout this library. It also
*				contains a pointer to the TCP_CONNECTION_INFO
*				structure, so you can operate solely from the 
*				TCP structure pointer.
*
***************************************************************/
typedef	struct _tcp
{
    TCP_CONNECTION_INFO				*tcp_connect;
    void(*set_inet_name)			(INET_NAME);
    int(*get_sock)					(TCP_CONNECTION_INFO *, int, int, int);
    int(*get_sock_nw)				(TCP_CONNECTION_INFO *, int, int, int, int);
    int(*set_bind)					(TCP_CONNECTION_INFO *);
    int(*set_bind_nw)				(TCP_CONNECTION_INFO *, signed long *);
    int(*make_connect)				(TCP_CONNECTION_INFO *);
    int(*make_connect_nw)			(TCP_CONNECTION_INFO *, signed long *);
    int(*set_listen)				(TCP_CONNECTION_INFO *);
    int(*new_accept)				(TCP_CONNECTION_INFO *, int*);
    int(*new_accept_nw)				(TCP_CONNECTION_INFO *, signed long *);
    int(*new_accept_nw1)			(TCP_CONNECTION_INFO *, signed long *);
    int(*new_accept_nw2)			(TCP_CONNECTION_INFO *, signed long *);
    int(*new_accept_nw3)			(TCP_CONNECTION_INFO *, struct sockaddr *, signed long *);
    int(*new_send)					(TCP_CONNECTION_INFO *, char*, int);
    int(*new_send_nw)				(TCP_CONNECTION_INFO *, char*, int, signed long *);
    int(*new_recv)					(TCP_CONNECTION_INFO *, char*, int, int);
    int(*new_recv_nw)				(TCP_CONNECTION_INFO *, char*, int, signed long *, int);
    int(*shutdown_sock)				(TCP_CONNECTION_INFO *, int);
    int(*shutdown_sock_nw)			(TCP_CONNECTION_INFO *, int, signed long *);
    int(*close_sock)				(TCP_CONNECTION_INFO *);
    int(*get_sock_name)				(TCP_CONNECTION_INFO *);
    int(*get_sock_name_nw)			(TCP_CONNECTION_INFO *, signed long *);
    void(*clean_conn_info)			(TCP_CONNECTION_INFO *);
    void(*set_options)	     		(TCP_CONNECTION_INFO *, int, int, long);
    void(*set_sockaddr)				(TCP_CONNECTION_INFO *, short);
} TCP;

/**********************************************************
*		Function Prototype Definition(s)
**********************************************************/
TCP *intialize_tcp ( void );
static void await_completion ( signed long      *sock_fn
                             , short            *buffer_addr
                             , signed short     buffer_size
                             , char             *count_trnsfr
                             , signed long      *tag
                             , signed long      timeout
                             , short            *error_code);

enum
{
    INFO = 0,
    WARNING = 1,
    ERROR = 2
};

#endif // !_NSCCH_INCLUDE_