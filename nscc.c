/************************************************************************************
* !     \file       nstcp.c
*
*		AUTHOR:		Mark Tripoli
*		DATE:		28 - JAN - 2018
*		LICENSE:	MIT License
*
*		Purpose:	This library is geared towards TCP/IP operations within
*					NonStop Guardian enviornments. Some of the code may need to be
*					tweaked, as this was geared towards compilation on a C++ compiler.
*
*		Notes:		Add your Trace Entrances/Exits and Debug Checks
*					If you are reading in your IPs, ports, etc. I would add that 
*					routine here, and call it whenever your process starts.
*
*		
*
*		REVISIONS:
*		VERSION		DATE	     COMMENTS
*		-------    ------       ---------------------------------------------------
*		1.0.0	  1/28/18		Initial Release
*************************************************************************************/

#ifdef __TANDEM
#include "=nscch"
#else

#include "nscc.h"

#endif

#ifdef __cplusplus
extern "C" {
#endif


/***************************************************************************************
*						FUNCTION PROTOTYPES AND DEFINITIONS
***************************************************************************************/

/***************************************************************
*
* @fn                       await_completion
*
* FUNCTION:                 Performs and awaitio after a socket
*                           operation, then checks the file info for an error.
*                           If an error is encountered, return a status
*
*
* @param sock_fn            The file number/descriptor of an open socket(file)
* @param buffer_addr        The address of the buffer specified when the operation
*                           was initiated
* @param buffer_size
* @param count_trnsfr       The count of the number of bytes transferred
* @param tag                The defined tag that was stored by the system when the I/O
*                           operation associated with this completion was initiated
* @param timeout            The delay used to wait for completion of an I/O, instead
*                           of checking for completion
* @param error_code
*
* @return short             The status of the FILE_GET_INFO
***************************************************************/
static void await_completion (
          signed long      *sock_fn
        , short            *buffer_addr
        , signed short     buffer_size
        , char             *count_trnsfr
        , signed long      *tag
        , signed long      timeout
        , signed short     *error_code)
{
    memset ( buffer_addr, 0, sizeof ( buffer_size ) );

    AWAITIOX ( &sock_fn
             , ( signed long * )    &buffer_addr
             , ( unsigned short * ) &count_trnsfr
             , &tag
             , timeout);

    FILE_GETINFO_ ( sock_fn, &error_code );
}

/* Add them here if you make some new routines */

/***************************************************************
*
* @fn                       Set_Proc
*
* FUNCTION:                 Specifies the name of the NonStop TCPIP
*                           or TCP6SAM process, which the socket
*                           will be using.
*
* NOTE:                     ex. $ZB27D, $ZB26C
*
* @param process_name       The NS process name
* @return void
***************************************************************/
static void Set_Inet_Name( INET_NAME process_name )
{
    socket_set_inet_name(process_name);
}


/*************************************************************************
*
* @fn                   Get_Sock
*
* FUNCTION:             Builds the sockaddr structure. The sockaddr structure
*                       holds all information required to perform operations
*                       w/ a client/server. It holds the IP, Port, Addr family
*                       in a network readible format.
*
* NOTE:                 ex. of address_family : AF_INET, PF_INET, etc.
*
* @param connection     The connection information needed in making a connection
* @param address_family The AF used to create a socket
* @return  void
*************************************************************************/
static void Set_SockAddr ( TCP_CONNECTION_INFO *connection, short address_family )
{
    /* allocate memory for the socket address structure */
    connection->sockaddr = malloc(sizeof(connection->sockaddr));
    /* zero it out */
    memset(connection->sockaddr, 0, sizeof(*connection->sockaddr));
    /* sometimes sa_data fills with junk which makes the server refuse the connection,*/
    /* so to be safe we zero it out                                                   */
    memset(connection->sockaddr->sa_data, '\0', sizeof(connection->sockaddr->sa_data));
    /* here is where we set the values in the structure into a network readable format*/
    connection->sockaddr->sin_family = address_family;
    connection->sockaddr->sin_port = htons(connection->port);
    connection->sockaddr->sin_addr.s_addr = inet_addr(connection->ipaddr);
}

/*******************************************************************
*
* @fn                     NewSocket
*
* FUNCTION:               Creates a new Socket/FD
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @param address_family   The address family of the socket connection
* @param socket_type      The type of socket this will be
* @param protocol         The protocol we would like to implement
* @return socket fn       The file descriptor (socket number)
*******************************************************************/
static int Create_Socket (TCP_CONNECTION_INFO *connection, int address_family, int socket_type, int protocol)
{
    int socket_num;

    /* Don't forge to call the freeing proc when done */
    memset( &connection->sock, '\0', sizeof ( *connection->sock ) );
    connection->sock = ( int * ) malloc ( sizeof ( int ) );

    socket_num = socket( address_family
                       , socket_type
                       , protocol );

    return socket_num;
}

/*******************************************************************
*
* @fn                     Get_Sock_NW
*
* FUNCTION:               Creates a new NOWAIT Socket/FD
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @param address_family   The address family of the socket connection
* @param socket_type      The type of socket this will be
* @param protocol         The protocol we would like to implement
* @param sync             Input value: NOT SUPPORTED ON GUARDIAN, MUST USE 0
* @return socket fn       The file descriptor (socket number)
*******************************************************************/
static int Get_Sock_NW( TCP_CONNECTION_INFO *connection, int address_family, int socket_type, int protocol, int sync )
{
    int socket_num;

    socket_num = socket_nw ( address_family
                           , socket_type
                           , protocol
                           , connection->flags
                           , sync );

    return socket_num;
}

/*******************************************************************
*
* @fn                      Set_Bind
*
* FUNCTION:                associates a socket with a specific
*                          local internet address and port
*                          This is primarily a server func but
*                          is optional on most client connections
*
* NOTE:
* @param connection       The connection information used to create the socket
* @return                 The error code returned
*******************************************************************/
static int Set_Bind (TCP_CONNECTION_INFO *connection)
{
    int status;

    status = bind ( *connection->sock
                  , ( struct sockaddr * ) connection->sockaddr
                  , connection->sockaddr_len );

    return status;
}


/*******************************************************************
*
* @fn                     Set_Bind_NW
*
* FUNCTION:               associates a socket with a specific
*                         local internet address and port
*                         This is primarily a server func but
*                         is optional on most client connections
*
*                         This is a NOWAIT call
*
* @param connection       The connection information used to create the socket
* @param tag              The tag parameter to be used for the nowait operation.
* @return                 The error code returned
*******************************************************************/
static int Set_Bind_NW ( TCP_CONNECTION_INFO *connection, signed long *tag )
{
    int status;

    status = bind_nw ( *connection->sock
                     , ( struct sockaddr * ) connection->sockaddr
                     , connection->sockaddr_len
                     , &tag );

    return status;
}


/*******************************************************************
*
* @fn                     Make_Connect
*
* FUNCTION:               Connects to specified address
*
* NOTE:                   Must be called AFTER newSocket()
*
* @param connection       The connection information used to create the socket
* @return                 The error code returned
*******************************************************************/
static int Make_Connect ( TCP_CONNECTION_INFO *connection )
{
    int status;

    /* we have to set sa_data, if not, seems like junk fills it
    * , which makes the server refuse the connection */
    memset ( connection->sockaddr->sa_data
           , '\0'
           , sizeof ( connection->sockaddr->sa_data ) );

    status = connect ( *connection->sock
                     , ( struct sockaddr * ) connection->sockaddr
                     , sizeof ( *connection->sockaddr ) );

    return status;
}

/*******************************************************************
*
* @fn                     Make_Connect_NW
*
* FUNCTION:               Connects to specified address NO Waited
*
* NOTE:                   Must be called AFTER newSocket_nw()
*
* @param connection       The connection information used to create the socket
* @param tag              The tag parameter to be used for the nowait operation.
* @return                 The error code returned
*******************************************************************/
static int Make_Connect_NW ( TCP_CONNECTION_INFO *connection, signed long *tag )
{
    int status;

    /* we have to set sa_data, if not, seems like junk fills it, which makes the server refuse the connection */
    memset ( connection->sockaddr->sa_data
           , '\0'
           , sizeof( connection->sockaddr->sa_data ) );

    status = connect_nw ( *connection->sock
                        , ( struct sockaddr * ) &connection->sockaddr
                        , connection->sockaddr_len
                        , &tag );

    return status;
}

/*******************************************************************
*
* @fn                     Set_Listen
*
* FUNCTION:               A server based func. Listens
*                         for incoming connections
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @return                 The error code returned
* *******************************************************************/
static int Set_Listen (TCP_CONNECTION_INFO *connection )
{
    int status;

    status = listen ( *connection->sock
                    , connection->queue_len );

    return status;
}

/*******************************************************************
*
* @fn                     New_Accept
*
* FUNCTION:               Checks for connections on an existing waited
*                         socket. When a connection requestarrives,
*                         accept creates a new socket to use for data
*                         transfer and accepts the connection on the
*                         new socket
*
*
* @param connection       The connection information used to create the socket
* @return                 The error code returned
* *******************************************************************/
static int New_Accept ( TCP_CONNECTION_INFO *connection, int *from_len_ptr )
{
    int status;

    status = accept ( *connection->sock
                    , ( struct sockaddr * ) connection->sockaddr
                    , from_len_ptr );

    return status;
}

/*******************************************************************
*
* @fn                     New_Accept_NW
*
* FUNCTION:               Checks for connections on an existing no waited
*                         socket. When a connection requestarrives,
*                         accept creates a new socket to use for data
*                         transfer and accepts the connection on the
*                         new socket
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @param tag             The tag parameter to be used for the nowait operation.
* @return                 The error code returned
* *******************************************************************/
static int New_Accept_NW ( TCP_CONNECTION_INFO *connection, signed long *tag )
{
    int status;

    status = accept_nw ( *connection->sock
                       , ( struct sockaddr * ) connection->sockaddr
                       , &connection->sockaddr_len
                       , &tag );

    return status;
}

/*********************************************************************
*
* @fn                     New_Accept_NW1
*
* FUNCTION:               Used instead of accept_nw; use accept_nw1
*                         to set the maximum connections in the queue
*                         awaiting acceptance on a socket.
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @param tag              The tag parameter to be used for the nowait operation.
* @return                 The error code returned
* *******************************************************************/
static int New_Accept_NW1 ( TCP_CONNECTION_INFO *connection, signed long *tag )
{
    int status;

    status = accept_nw1 ( *connection->sock
                        , ( struct sockaddr * ) connection->sockaddr
                        , &connection->sockaddr_len
                        , &tag
                        , ( short ) connection->queue_len);

    return status;
}

/*********************************************************************
*
* @fn                     New_Accept_NW2
*
* FUNCTION:               Accepts a connection on a new socket created
 *                        for nowait data transfer. Beforecalling this
 *                        procedure, a program should call accept_nw
 *                        on an existing socket and then call socket_nw
 *                        tocreate the new socket to be used by accept_nw2.
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @param tag              The tag parameter to be used for the nowait operation.
* @return                 The error code returned
* *******************************************************************/
static int New_Accept_NW2 ( TCP_CONNECTION_INFO *connection, signed long *tag )
{
    int status;

    status = accept_nw2 ( *connection->sock
                        , ( struct sockaddr * ) connection->sockaddr
                        , &tag );

    return status;
}

/*********************************************************************
*
* @fn                     New_Accept_NW3
*
* FUNCTION:
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @param me_ptr           Points to the local address and port number used by bind_nw.
* @param tag              The tag parameter to be used for the nowait operation.
* @return                 The error code returned
* *******************************************************************/
static int New_Accept_NW3 (TCP_CONNECTION_INFO *connection, struct sockaddr *me_ptr, signed long *tag )
{
    int status;

    status = accept_nw3 ( *connection->sock
                        , ( struct sockaddr * ) connection->sockaddr
                        , me_ptr
                        , &tag );

    return status;
}

/**********************************************************************
*
* @fn                     New_Send
*
* FUNCTION:               Sends data on a connected socket
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @param buffer_ptr       Points to the data to be sent..
* @param buffer_length    The size of the buffer pointed to by buffer_ptr.
* @return                 The error code returned
* *******************************************************************/
static int New_Send (TCP_CONNECTION_INFO *connection, char *buffer_ptr, int buffer_length )
{
    int status;

    status = send ( *connection->sock
                  , buffer_ptr
                  , buffer_length
                  , connection->flags );

    return status;
}

/*******************************************************************************
*
* @fn                     New_Send_NW
*
* FUNCTION:               This is a NOWAIT operation.
*                         Sends data on a connected socket
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @param buffer_ptr       Points to the data to be sent..
* @param buffer_length    The size of the buffer pointed to by buffer_ptr.
* @param tag              The tag parameter to be used for the nowait operation.
* @return                 The error code returned
* *****************************************************************************/
static int New_Send_NW ( TCP_CONNECTION_INFO *connection, char *buffer_ptr, int buffer_length, signed long *tag )
{
    int status;

    status = send_nw ( *connection->sock
            , buffer_ptr
            , buffer_length
            , connection->flags
            , &tag );

    return status;
}

/*********************************************************************************
*
* @fn                     New_Recv
*
* FUNCTION:               Receives data on a connected socket
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @param buffer_ptr       Points to the data to be sent..
* @param buffer_length    The size of the buffer pointed to by buffer_ptr.
* @param nrcvd            the number of bytes received by the recv function.
*                         This is the return value for recv.A zero length message
*                         indicates end of file (EOF).
* @return                 The error code returned
* *******************************************************************************/
static int New_Recv (TCP_CONNECTION_INFO *connection, char *buffer_ptr, int buff_length, int nrcvd )
{
    int status;

    nrcvd = recv ( *connection->sock
                  , buffer_ptr
                  , buff_length
                  , connection->flags );

    if ( nrcvd < 0 )
        return -1;
}

/*******************************************************************************
*
* @fn                     New_Recv_NW
*
* FUNCTION:               This is a NOWAIT operation.
*                         Receives data on a connected socket.
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @param buffer_ptr       Points to the data to be sent..
* @param buffer_length    The size of the buffer pointed to by buffer_ptr.
* @param length           The size of the buffer pointed to by buffer_ptr.
* @param tag              The tag parameter to be used for the nowait operation.
* @param nrcvd            the number of bytes received by the recv function.
*                         This is the return value for recv.A zero length message
*                         indicates end of file (EOF).
* @return                 The error code returned
* *****************************************************************************/
static int New_Recv_NW (TCP_CONNECTION_INFO *connection, char *buffer_ptr, int length, signed long *tag, int nrcvd)
{
    int status;

    nrcvd = recv_nw ( *connection->sock
                     , buffer_ptr
                     , length
                     , connection->flags
                     , &tag );

    if ( nrcvd < 0 )
        return -1;
}

/*********************************************************************************
*
* @fn                   Shutdown_Sock
*
* FUNCTION:             shuts down data transfer, partially
*                       or completely, on an actively connected
*                       TCP socket
*
* NOTE:                 for the "how"
*                       0 = stops recieving data over socket
*                       1 = stops sending data over socket
*                       3 = stops sends and recieves over socket
*
* @param connection     The connection information used to create the socket
* @param how            Specifies what kind of operations on the socket are to
*                       be shut down
* @return               The error code returned
* *******************************************************************************/
static int Shutdown_Sock (TCP_CONNECTION_INFO *connection, int how )
{
    int status;

    status = shutdown( *connection->sock, how );

    return status;
}

/*********************************************************************************
*
* @fn                   Shutdown_Sock
*
* FUNCTION:             shuts down data transfer, partially
*                       or completely, on an actively connected
*                       TCP socket
*
* NOTE:                 for the "how"
*                       0 = stops recieving data over socket
*                       1 = stops sending data over socket
*                       3 = stops sends and recieves over socket
*
* @param connection     The connection information used to create the socket
* @param how            Specifies what kind of operations on the socket are to
*                       be shut down
* @param tag            The tag parameter to be used for the nowait operation.
* @return               The error code returned
* *******************************************************************************/
static int Shutdown_Sock_NW (TCP_CONNECTION_INFO *connection, int how, signed long *tag )
{
    int status;

    status = shutdown_nw ( *connection->sock
                         , how
                         , &tag );

    return status;
}

/*********************************************************************************
*
* @fn                     Close_Sock
*
* FUNCTION:               closes the socket/fd. & sets it
*                         back to NULL
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @return                 The error code returned
* *******************************************************************************/
static int Close_Sock (TCP_CONNECTION_INFO *connection )
{
    int status;

    if ( !connection->sock )
    {
        *connection->sock = 0;
        memset ( &connection->sock
               , 0
               , sizeof ( *connection->sock ) );

        status = 0;
        return status;
    }

    /* We use the nonstop call here you can use close(), but sometimes its finickey*/
    status = FILE_CLOSE_ ( ( signed short ) *connection->sock );

    memset ( &connection->sock
           , 0
           , sizeof ( *connection->sock ) );

    return status;
}

/*********************************************************************************
*
* @fn                     Get_Sock_Name
*
* FUNCTION:               Get the address and port
*                         number to which a socket isbound
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @return                 The error code returned
* *******************************************************************************/
static int Get_Sock_Name (TCP_CONNECTION_INFO *connection )
{
    int status;

    status = getsockname ( *connection->sock
                         , ( struct sockaddr * ) connection->sockaddr
                         , &connection->sockaddr_len );

    return status;
}

/*********************************************************************************
*
* @fn                     Get_Sock_Name_NW
*
* FUNCTION:               Get the address and port
*                         number to which a socket isbound
*
* NOTE:
*
* @param connection       The connection information used to create the socket
* @param tag              The tag parameter to be used for the nowait operation.
* @return                 The error code returned
* *******************************************************************************/
static int Get_Sock_Name_NW ( TCP_CONNECTION_INFO *connection, signed long *tag )
{
    int status;

    status = getsockname_nw ( *connection->sock
                            , ( struct sockaddr * ) connection->sockaddr
                            , &connection->sockaddr_len
                            , &tag );

    return status;
}

/******************************************************************************************
*
* @fn                     Clean_Conn_Info
*
* FUNCTION:               Clean up memory allocated for sockaddr structure and socket.
*                         Will also zero out some other elements, for the next transaction.
*
* @param connection       The connection information used to create the socket
* @return                 void
******************************************************************************************/
static void Clean_Conn_Info ( TCP_CONNECTION_INFO *connection )
{
    if (connection->sockaddr != 0)
    {
        free(connection->sockaddr);
        connection->sockaddr = 0;
    }
    if (connection->sock != 0)
    {
        free(connection->sock);
        connection->sock = 0;
    }

    /* cleanup all data which is set each time a socket is created */
    connection->queue_len = '\0';
    connection->flags = '\0';
    connection->sockaddr_len = '\0';
    connection->tag = '\0';
}

/******************************************************************************************
*
* @fn                     Tcp_Set_Options
*
* FUNCTION:               Set's the rest of the addition elements for later use in calling
*                         process
*
* @param connection       The connection information used to create the socket
* @param flags
* @param queue_length
* @param sockaddr_len
* @return                 void
*****************************************************************************************/
static void Tcp_Set_Options ( TCP_CONNECTION_INFO *connection, int flags, int queue_length, long sockaddr_len)
{
    /* pre-set some additional data for later use in calling process */
    connection->flags = flags;
    connection->queue_len = queue_length;
    connection->sockaddr_len = sockaddr_len;
}

#pragma PAGE "init_tcpip"
/******************************************************************************************
*
* @fn                   initializeTCPIP
*
* FUNCTION:             This function will allocate a chunk of memory for the size of the
*                       TCPIP structure. Then all propreitary IP function definitions
*                       will inherit the properties of each standard socket / nonstop socket
*                       function call. Aka will look at the address of those standard functions.
*
* @return                 void
*****************************************************************************************/
TCP* intialize_tcp ( )
{
    TCP               *tcp;
    BOOLEAN            status;
    ERROR_MESSAGE      error_text;

    /* allocate memory */
    tcp = ( TCP * ) malloc ( sizeof ( TCP ) );

    /* redirect function calls to address of tcpip calls*/
    tcp->set_inet_name = Set_Inet_Name;
    tcp->get_sock = Create_Socket;
    tcp->get_sock_nw = Get_Sock_NW;
    tcp->set_bind = Set_Bind;
    tcp->set_bind_nw = Set_Bind_NW;
    tcp->make_connect = Make_Connect;
    tcp->make_connect_nw = Make_Connect_NW;
    tcp->set_listen = Set_Listen;
    tcp->new_accept = New_Accept;
    tcp->new_accept_nw = New_Accept_NW;
    tcp->new_accept_nw1 = New_Accept_NW1;
    tcp->new_accept_nw2 = New_Accept_NW2;
    tcp->new_accept_nw3 = New_Accept_NW3;
    tcp->new_send = New_Send;
    tcp->new_send_nw = New_Send_NW;
    tcp->new_recv = New_Recv;
    tcp->new_recv_nw = New_Recv_NW;
    tcp->shutdown_sock = Shutdown_Sock;
    tcp->shutdown_sock_nw = Shutdown_Sock_NW;
    tcp->close_sock = Close_Sock;
    tcp->get_sock_name = Get_Sock_Name;
    tcp->get_sock_name_nw = Get_Sock_Name_NW;
    tcp->clean_conn_info = Clean_Conn_Info;
    tcp->set_options = Tcp_Set_Options;
    tcp->set_sockaddr = Set_SockAddr;

    /* allocate memory for connection structure */
    tcp->tcp_connect = ( TCP_CONNECTION_INFO * ) malloc ( sizeof ( TCP_CONNECTION_INFO ) );

    return tcp;
}


#ifdef __cplusplus
}
#endif