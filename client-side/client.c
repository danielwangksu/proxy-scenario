/**************************************************************
*   Distributed Proxy Test -- Client Process                  *
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
int server_ep;

/*===========================================================================*
 *              initialize                     *
 *===========================================================================*/
void initialize(){
    memset(&m, 0, sizeof(m));
    server_ep = getendpoint_name("server-proxy");
    printf("[CLIENT]: server_ep=%d\n", server_ep);
}


/*===========================================================================*
 *              response                       *
 *===========================================================================*/
int response(int data)
{
    memset(&m, 0, sizeof(m));
    m.m_type = CLI_SER_SPECIFIC;
    m.m_m1.m1i1 = data;
    return ipc_send(server_ep, &m);
}



/*===========================================================================*
 *              main                           *
 *===========================================================================*/
void main(int argc, char ** argv){
    int r, status;
    int i = 0;
    int data = 0;

    initialize();

    while(1){
        sleep(2);
        r = ipc_receive(server_ep, &m, &status);
        printf("[CLIENT]: receive data m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);
        data = m.m_m1.m1i1;

        if(data == -1)
            exit(0);

        data += 1;
        printf("[CLIENT]: do work...\n");

        r = response(data);
    }
    exit(0);
}