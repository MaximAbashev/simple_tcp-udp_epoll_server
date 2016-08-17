#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/epoll.h>


#define SRV_PORT 5555
#define CLNT_PORT 4444
#define NUM_EVENTS 10

int main () {
	const int on = 1;
	int tcp_sock, udp_sock, from_len, ready_tcp, ready_udp, ready, contact, len, epfd, fd, i;
	int timeout_msec = 500;
	char buf[30];
	struct sockaddr_in s_addr, clnt_addr, new_s_addr;
	struct epoll_event ev, events[NUM_EVENTS];

	epfd = epoll_create (NUM_EVENTS); /*epoll descriptor init*/
/*
 * Fabricate socket and set socket options.
 */	
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
	s_addr.sin_port = htons (SRV_PORT);
/*
 * TCP-socket part.
 */
	tcp_sock = socket (AF_INET, SOCK_STREAM, 0);
	setsockopt (tcp_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));
	if (bind (tcp_sock, (struct sockaddr *)&s_addr, sizeof (s_addr)) < 0)
	{
		perror ("TCP bind error!\n");
		exit (1);
	}
	ev.events = EPOLLIN | EPOLLPRI | EPOLLET;
	ev.data.fd = tcp_sock;
	ready_tcp = epoll_ctl (epfd, EPOLL_CTL_ADD, tcp_sock, &ev);
	if (ready_tcp < 0)
	{
		perror ("Epoll_ctl TCP error!\n");
		exit (1);
	}
	listen(tcp_sock, 1);
/*
 * UDP-socket part.
 */
	udp_sock = socket (AF_INET, SOCK_DGRAM, 0);
	if (bind (udp_sock, (struct sockaddr *)&s_addr, sizeof (s_addr)) < 0)
	{
		perror ("UDP bind error!\n");
		exit (1);
	}
	ev.events = EPOLLIN | EPOLLPRI | EPOLLET;
	ev.data.fd = udp_sock;
	ready_udp = epoll_ctl (epfd, EPOLL_CTL_ADD, udp_sock, &ev);
	if (ready_udp < 0)
	{
		perror ("Epoll_ctl UDP error!\n");
		exit (1);
	}
/*
 * Client service loop
 */
	while (1)
	{
		ready = epoll_wait (epfd, events, NUM_EVENTS, timeout_msec);
		if ( ready < 0)
		{
			printf ("Epoll_wait error!\n");
			exit (1);
		}
		for (i = 0; i < ready; i++)
		{
			if (events[i].data.fd == tcp_sock) /*tcp-client service*/
			{
				len = sizeof (s_addr);
				contact = accept (tcp_sock, 
					(struct sockaddr *)&s_addr, &len);
				if(contact == (-1))
				{
					perror ("Connect TCP error!\n");
					exit (1);
				}
				from_len = recv (contact, buf, 21, 0);
				write (1, buf, from_len);
				send (contact, "It's for TCP client!\n", 22, 0);
				close (contact);	
			}
			if (events[i].data.fd == udp_sock) /*udp-client service*/
			{
				fd = events[2].data.fd;
				len = sizeof (s_addr);
				while (2)
				{
					from_len = recvfrom (udp_sock, buf,
					21, 0, (struct sockaddr *)&s_addr,
					&len);
					if(from_len > 0)
					{
						write (1, buf, from_len);
						break;
					}
				}
				sendto(udp_sock, "It's for UDP client!\n", 22,
				0, (struct sockaddr *)&s_addr, sizeof(s_addr));
			}
		}
	}	
	close (tcp_sock);
	close (udp_sock);
	close (epfd);
	return 1;
}
