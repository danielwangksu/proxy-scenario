/**************************************************************
*   Distributed Proxy Test -- Cl-In-Proxy Process             *
*   by Daniel Wang                                            *
***************************************************************/
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <lib.h>
#include <stdlib.h>
#include <strings.h>
#include "../msgtype.h"

#define OK 0
#define TRUE 1
#define FALSE 0

#define MAX_NET_PACKET 1024

#define DEST_UDP_PORT 9090 /* 9090 receive port */
#define SELF_IP "192.168.67.190" // this is the client-application side
#define CLIENT_IP "192.168.67.189"

// shared network packet structure
typedef struct{
    int dest_acid;
    int source_acid;
    int type;
    int callnr;
    int net_size;
    char data[MAX_NET_PACKET];
} net_packet;

message m;
static endpoint_t source_e;
static endpoint_t dest_e;
static int source_acid;
static int dest_acid;
static int callnr;  /* call number */

static void bail(const char *on_what) {
    perror(on_what);
    exit(1); 
}

/*===========================================================================*
 *              initialize                   *
 *===========================================================================*/
void initialize(void){
    memset(&m, 0, sizeof(m));
}


/*===========================================================================*
 *              endpoint_to_acid             *
 *===========================================================================*/
int endpoint_to_acid(endpoint_t ep, int *acid)
{
    int r;
    int pid;

    pid = getpidfromendpoint(ep);
    *acid = getacidfrompid(pid); //need implement new system call to support it
    return OK;

}


/*===========================================================================*
 *              acid_to_endpoint             *
 *===========================================================================*/
int acid_to_endpoint(int acid, endpoint_t *ep)
{
    int r;
    int pid;

    pid = getpidfromacid(acid);
    *ep = getendpoint(pid);
    return r;
}


/*===========================================================================*
 *              deserialize                    *
 *===========================================================================*/
int deserialize(net_packet *packet) 
{
    memset(&m, 0, sizeof(m));
    dest_acid = ntohl(packet->dest_acid);
    source_acid = ntohl(packet->source_acid);
    m.m_m9.m9l1 = ntohl(packet->type);
    m.m_m9.m9l2 = ntohl(packet->callnr);
    m.m_m9.m9l3 = ntohl(packet->net_size);

    if (m.m_m9.m9l3 != 0)
        // need deserialize large data struct
        return EINVAL;
    return OK;
}


/*===========================================================================*
 *              deliver_message              *
 *===========================================================================*/
/* need to switch source and destination: 
   ipc_send to source_e and instruct it to send to dest_e 
*/
int deliver_message()
{
    int r;
    r = acid_to_endpoint(dest_acid, &dest_e);
    r = acid_to_endpoint(source_acid, &source_e);
    printf("[INPROXY]: dest_e:%d, source_e:%d\n", dest_e, source_e);
    m.m_type = PROXY_RECEIVEFROM;
    m.m_m9.m9ull1 = dest_e;

    r = ipc_send(source_e, &m);

    return r;
}


/*===========================================================================*
 *              main                         *
 *===========================================================================*/
void main(int argc, char ** argv){
    int r, status;
    int result;

    // socket related variables
    int receive_status;
    struct sockaddr_in client_adr;
    struct sockaddr_in local_adr;
    socklen_t client_adr_len;
    socklen_t local_adr_len;

    char *local_adr_char = NULL;

    int socket_fd;
    net_packet packet;

    local_adr_char = SELF_IP;

    initialize();

    // build socket
    memset(&client_adr, 0, sizeof client_adr);
    memset(&local_adr, 0, sizeof local_adr);
    local_adr.sin_family = AF_INET;
    local_adr.sin_port = htons(DEST_UDP_PORT);
    local_adr.sin_addr.s_addr = inet_addr(local_adr_char);
    local_adr_len = sizeof local_adr;

    if (local_adr.sin_addr.s_addr == INADDR_NONE)
        bail("[INPROXY]: bad address");

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1)
        bail("[INPROXY]: socket()");

    if(bind(socket_fd, (struct sockaddr *) &local_adr, sizeof(local_adr)) < 0)
        bail("[INPROXY]: bind()");

    while(1){
        client_adr_len = sizeof(client_adr);

        printf("[INPROXY]: start receiving socketID=%d, %s\n", socket_fd, inet_ntoa(local_adr.sin_addr));
        receive_status = recvfrom(socket_fd, &packet, sizeof(net_packet), 0, (struct sockaddr *)&client_adr, &client_adr_len);
        if (receive_status < 0)
            bail("[INPROXY]: recvfrom(2)");
        // decrypt message, check message, and send to server
        r = deserialize(&packet);
        if (r != OK)
            continue;
        printf("[IN_PROXY]: %d, %d, %ld, %ld, %ld\n", dest_acid, source_acid, m.m_m9.m9l1, m.m_m9.m9l2, m.m_m9.m9l3);

        r = deliver_message();
        if (r != OK)
            continue;
    }
}
