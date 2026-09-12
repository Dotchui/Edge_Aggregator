#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include "../include/shared_payload.h"

volatile sig_atomic_t   keep_running = 1;

void    intHandler(int dummy)
{
    (void) dummy;
    keep_running = 0;
}

int     main(void)
{
    struct sockaddr_in  server_addr, client_addr;
    socklen_t           addr_len;
    struct env_payload  payload;
    int                 sockfd;
    struct sigaction    sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = intHandler;
    sigaction(SIGINT, &sa, NULL);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit (1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror ("Bind failed");
        close(sockfd);
        exit(1);
    }
    printf("Listening on UDP port 8080\n");

    while (keep_running)
    {
        memset(&payload, 0, sizeof(payload));
        addr_len = sizeof(client_addr);
        ssize_t read = recvfrom(sockfd, &payload, sizeof(payload), 0, (struct sockaddr *)&client_addr, &addr_len);
        if (read > 0)
        {
            printf("[Time: %u ms] Temp: %d.%06d C | Press: %d.%06d kPa\n", payload.timestamp, payload.temperature.val1,
                payload.temperature.val2, payload.pressure.val1, payload.pressure.val2);
        }
        else if (read < 0)
        {
            if (keep_running == 0)
                break ;
            perror("recvfrom failed");
        }
    }
    close(sockfd);
    printf("\nDaemon terminated cleanly.\n");
    return (0);
}