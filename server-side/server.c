/**************************************************************
*   Distributed Proxy Test -- Server Process                  *
*   by Daniel Wang                                            *
***************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <lib.h>
#include <stdlib.h>
#include <strings.h>
#include "../msgtype.h"

message m;
int client_ep;

/*===========================================================================*
 *              initialize                      *
 *===========================================================================*/
void initialize(){
    memset(&m, 0, sizeof(m));
    client_ep = getendpoint_name("client-proxy");
    printf("[SERVER]: client_ep=%d\n", client_ep);
}


/*===========================================================================*
 *              prepare                         *
 *===========================================================================*/
/* server <-> client uses m_m1 format */
void prepare(int data){
    memset(&m, 0, sizeof(m));
    m.m_type = SER_CLI_SPECIFIC;
    m.m_m1.m1i1 = data;
    printf("[SERVER]: sending data m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);
}


/*===========================================================================*
 *              main                           *
 *===========================================================================*/
void main(int argc, char ** argv){
    int r, status;
    int i = 0;
    int data[] = {1, 11, 22, 33, 44, 55, 666, 87, 90, 100, -1};

    initialize();

    while(1){
        sleep(2);
        prepare(data[i]);
        i++;
        ipc_send(client_ep, &m);
        // poll receive and decide
        r = ipc_receive(client_ep, &m, &status);
        printf("[SERVER]: receive data m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);

        if(data[i] == -1)
            exit(0);
    }
    exit(0);
}