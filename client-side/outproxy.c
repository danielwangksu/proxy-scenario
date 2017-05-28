/**************************************************************
*   Distributed Proxy Test -- Cl-Out-Proxy Process            *
*   by Daniel Wang                                            *
***************************************************************/
#define _MINIX_SYSTEM 1
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <lib.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include "../msgtype.h"

#define OK 0
#define TRUE 1
#define FALSE 0
// message type
#define MAX_NET_PACKET 1024

#define DEST_UDP_PORT 9090 /* 9090 receive port */
#define SELF_IP "192.168.67.190" // this is the server-application side
#define DEST_IP "192.168.67.189"

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
static endpoint_t source_e; /* source's endpoint (e.g. server) */
static endpoint_t dest_e;    /* caller's endpoint (e.g. client-proxy) */
static int source_acid;
static int dest_acid;
static int callnr;  /* call number */

// socket related variables
struct sockaddr_in dest_adr;
int dest_adr_len;
int socket_fd;
int target_socket_fd;

/* Declare some local functions. */
static int do_send(int socket_fd);
static void reply(endpoint_t whom);

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
    int pid;

    pid = getpidfromendpoint(ep);
    *acid = getacidfrompid(pid);
    return OK;

}


/*===========================================================================*
 *              acid_to_endpoint             *
 *===========================================================================*/
int acid_to_endpoint(int acid, endpoint_t *ep)
{
    int pid;

    pid = getpidfromacid(acid);
    *ep = getendpoint(pid);
    return OK;
}


/*===========================================================================*
 *              main                         *
 *===========================================================================*/
void main(int argc, char ** argv){
    int r, status;
    int result;
    char *dest_adr_char = NULL;

    dest_adr_char = DEST_IP;

    // init code goes here
    initialize();

    // build socket
    memset(&dest_adr, 0, sizeof dest_adr);
    dest_adr.sin_family = AF_INET;
    dest_adr.sin_port = htons(DEST_UDP_PORT);
    dest_adr.sin_addr.s_addr = inet_addr(dest_adr_char);
    dest_adr_len = sizeof dest_adr;

    if (dest_adr.sin_addr.s_addr == INADDR_NONE)
        bail("[OUT-PROXY]: bad address");

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1)
        bail("[OUT-PROXY]: socket()");

    // main while loop, get work done and loop forever
    while(TRUE){
        r = ipc_receive(ANY, &m, &status);
        if(r != OK)
            continue;
        dest_e = m.m_source; /* message arrived! set sender */
        callnr = m.m_type;  /* set function call number */
        printf("[OUT-PROXY]: received message from %d, callnr: %d, from: %d, type: %d, data: %d\n", m.m_source, m.m_type, m.m_m9.m9ull1, m.m_m9.m9l1, m.m_m9.m9l2);

        switch(callnr){
            case PROXY_SENDTO:
                r = endpoint_to_acid(dest_e, &dest_acid);
                if(r != OK)
                    continue;
                printf("[OUT-PROXY]: start calling do_send()\n");
                // target_socket_fd = find_target_socket(dest_acid);
                result = do_send(socket_fd);
                break;
            default:
                printf("[OUT-PROXY]: warning, got illegal request from %d\n", m.m_source);
                continue;
        }

send_reply:
      /* Finally send reply message, unless disabled. */
    if(result != EDONTREPLY){
        memset(&m, 0, sizeof(m));
        m.m_type = PROXY_CONFORM;  /* build reply message */
        m.m_m9.m9ull1 = result;
        m.m_m9.m9ull2 = source_e;
        reply(dest_e);   /* send it away */
      }
    }
    close(socket_fd);
    exit(0);
}

/*===========================================================================*
 *              serialize                    *
 *===========================================================================*/
int serialize(net_packet *packet) 
{
    packet->dest_acid = htonl(dest_acid);
    packet->source_acid = htonl(source_acid);
    packet->type = htonl(m.m_m9.m9l1);
    packet->callnr = htonl(m.m_m9.m9l2);
    packet->net_size = htonl(m.m_m9.m9l3);

    return OK;
}

/*===========================================================================*
 *              do_send                      *
 *===========================================================================*/
int do_send(int socket_fd)
{
    int send_status, packet_status, r;
    net_packet packet;

    source_e = m.m_m9.m9ull1;
    r = endpoint_to_acid(source_e, &source_acid);
    if(r != OK)
    {
        printf("reach error\n");
        return EINVAL;
    }

    packet_status = serialize(&packet);
    if(packet_status != OK)
    {
        printf("reach error\n");
        return EINVAL;
    }

    // start sending
    printf("[OUTPROXY]: start sending socketID=%d, %s\n", socket_fd, inet_ntoa(dest_adr.sin_addr));
    send_status = sendto(socket_fd, &packet, sizeof(net_packet), 0, (struct sockaddr *)&dest_adr, dest_adr_len);
    if (send_status < 0)
        bail("[OUTPROXY]: sendto() failed");
    else
        return OK;
}

/*===========================================================================*
 *              reply                        *
 *===========================================================================*/
static void reply(endpoint_t who_e)
{
    int s = ipc_send(who_e, &m);    /* send the message */
    if (OK != s)
        printf("[OUT-PROXY]: unable to send reply to %d: %d\n", who_e, s);
}