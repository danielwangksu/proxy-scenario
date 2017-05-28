/**************************************************************
*   Distributed Proxy Test -- Client-delegation Process       *
*   by Daniel Wang                                            *
***************************************************************/
#define _MINIX_SYSTEM 1
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

message m;
message m_out, m_in;
static endpoint_t who_e;    /* caller's proc number */
static int callnr;  /* call number */
int outproxy_ep, inproxy_ep, server_ep;

/*===========================================================================*
 *              initialize                      *
 *===========================================================================*/
void initialize(){
    memset(&m, 0, sizeof(m));
    memset(&m_out, 0, sizeof(m_out));
    memset(&m_in, 0, sizeof(m_in));
    server_ep = getendpoint_name("server");
    outproxy_ep = getendpoint_name("outproxy");
    inproxy_ep = getendpoint_name("inproxy");
    printf("[CLIENT-PROXY]: server_ep=%d, outproxy_ep=%d, inproxy_ep=%d\n", server_ep, outproxy_ep, inproxy_ep);
}


/*===========================================================================*
 *              confirm                         *
 *===========================================================================*/
void confirm()
{
    int type, result, original_from; 
    // message uses m_m9
    type = m.m_type;
    result = m.m_m9.m9ull1;
    original_from = m.m_m9.m9ull2;
    printf("[CLIENT-PROXY]: the confirm message has been received from outproxy, type=%d, result=%d, original_from=%d\n", type, result, original_from);
}


/*===========================================================================*
 *              send_network                    *
 *===========================================================================*/
/* client <-> proxy uses m_m9 format */
// message format:
//  m.m_type = PROXY_SENDTO
//  m_m9.m9ull1 = original source endpoint (e.g. server)
//  m_m9.m9l1 = original message type   (e.g. remote procedure call)
//  m_m9.m9l2 = original message content (e.g. remote procedure call index)
//  m_m9.m9l3 = message size for future extension (current pass 0)
void send_network()
{
    int r;
    memset(&m_out, 0, sizeof(m_out));
    m_out.m_type = PROXY_SENDTO;
    m_out.m_m9.m9ull1 = who_e;
    m_out.m_m9.m9l1 = m.m_type;
    m_out.m_m9.m9l2 = m.m_m1.m1i1;
    m_out.m_m9.m9l3 = 0;
    printf("[CLIENT-PROXY]: proxy_type:%d, from:%d, m_type:%d, data:%d\n", m_out.m_type, m_out.m_m9.m9ull1,  m_out.m_m9.m9l1, m_out.m_m9.m9l2);

    r = ipc_send(outproxy_ep, &m_out);
    if(r != OK)
        printf("[CLIENT-PROXY]: send failed\n");
}


/*===========================================================================*
 *              receive_network                 *
 *===========================================================================*/
/* proxy <-> client uses m_m9 format */
// message format:
//  m.type = PROXY_RECEIVEFROM
//  m_m9.m9ull1 = original destination endpoint (e.g. server [from client's response])
//  m_m9.m9l1 = original message type   (e.g. remote procedure call)
//  m_m9.m9l2 = original message content (e.g. remote procedure call index)
//  m_m9.m9l3 = message size for future extension (current pass 0)
void receive_network()
{
    int r, dest_ep, size;
    memset(&m_in, 0, sizeof(m_in));
    dest_ep = m.m_m9.m9ull1;
    m_in.m_type = m.m_m9.m9l1;
    m_in.m_m1.m1i1 = m.m_m9.m9l2;
    size = m.m_m9.m9l3;

    r = ipc_send(dest_ep, &m_in);
    if(r != OK)
        printf("[CLIENT-PROXY]: send failed\n");
}


/*===========================================================================*
 *              main                            *
 *===========================================================================*/
void main(int argc, char ** argv){
    int r, status;
    int i = 0;

    initialize();

    while(1){
        r = ipc_receive(ANY, &m, &status);
        if(r != OK)
            continue;

        who_e = m.m_source; /* message arrived! set sender */
        callnr = m.m_type;  /* set function call number */

        if(who_e == server_ep)
            send_network();
        else if (who_e == outproxy_ep)
            confirm();
        else if (who_e == inproxy_ep)
            receive_network();
        else
            printf("[CLIENT-PROXY]: warning, got illegal request from %d\n", m.m_source);
    }
    exit(0);
}