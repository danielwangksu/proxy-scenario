/**************************************************************
*   Distributed Proxy Test -- Server Side Loading Process     *
*   by Daniel Wang                                            *
***************************************************************/

/*

+------------+                      +------------+         +------------+                     +------------+
|            |    +------------+ -->| Out-Proxy  |         |  In-Proxy  |-->+------------+    |            |
|   Server   |    |            |    |            | network |            |   |            |    |   Client   |
|  (id=100)  |<-->|Client-proxy|    +------------+ <-----> +------------+   |Server-proxy|<-->|  (id=101)  |
|            |    |  (id=101)  |    +------------+         +------------+   |  (id=100)  |    |            |
|            |    +------------+ <--|  In-Proxy  |         | Out-Proxy  |<--+------------+    |            |
+------------+                      |            |         |            |                     +------------+
                                    +------------+         +------------+ 

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <lib.h>
#include <sys/un.h>
#include <strings.h>
#include <signal.h>

#define MAX_ARGC  18
#define MAX_ARGS_LENGTH  10
#define INITIAL_ACID 100
int server_pid, client_pid, outproxy_pid, inproxy_pid;

// fault handler
static void bail(const char *on_what){
    perror(on_what);
    exit(1); 
}

// main function
void main(void){
    int server_acid, client_acid, outproxy_acid, inproxy_acid;
    char *argv_control[] = {"holder", 0};

    server_acid = INITIAL_ACID;
    client_acid = INITIAL_ACID + 1;
    outproxy_acid = INITIAL_ACID + 2;
    inproxy_acid = INITIAL_ACID + 3;

    if((server_pid = fork2(server_acid)) == 0){
        // server process
        if(execv("server", argv_control) == -1){
            printf("Return not expected.\n");
            bail("execv(server)");
        }
        exit(0);
    } else if((client_pid = fork2(client_acid)) == 0) {
        // client proxy process
        if(execv("client-proxy", argv_control) == -1){
            printf("Return not expected.\n");
            bail("execv(client)");
        }
        exit(0);
    }else if((outproxy_pid = fork2(outproxy_acid)) == 0){
        // cl-out-proxy process
        if(execv("outproxy", argv_control) == -1){
            printf("Return not expected.\n");
            bail("execv(outproxy)");
        }
        exit(0);

    }else if((inproxy_pid = fork2(inproxy_acid)) == 0){
        // cl-in-proxy process
        if(execv("inproxy", argv_control) == -1){
            printf("Return not expected.\n");
            bail("execv(inproxy)");
        }
        exit(0);
    }else{
        // parent process
        int server_ep, client_ep, outproxy_ep, inproxy_ep;
        int child_status;
        int t_pid;

        server_ep = getendpoint(server_pid);
        client_ep = getendpoint(client_pid);
        outproxy_ep = getendpoint(outproxy_pid);
        inproxy_ep = getendpoint(inproxy_pid);


        printf("[PARENT]: server_pid=%d, client_pid=%d, outproxy_pid=%d, inproxy_pid=%d, server_ep=%d, client_ep=%d, outproxy_ep=%d, inproxy_ep=%d\n", server_pid, client_pid, outproxy_pid, inproxy_pid, server_ep, client_ep, outproxy_ep, inproxy_ep);

        t_pid = wait(&child_status);
        kill(server_pid, SIGKILL);
        kill(client_pid, SIGKILL);
        kill(outproxy_pid, SIGKILL);
        kill(inproxy_pid, SIGKILL);

        exit(0);
    }   
}