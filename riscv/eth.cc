/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 It may be useful to throttle the ethernet in conjunction with this program to
  take into account the latency of the target processor, for example:

  sudo tc qdisc add dev eth0 root tbf rate 4.0mbit latency 50ms burst 50kb mtu 10000

 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <openssl/md5.h>

#include "devices.h"
#include "eth.h"

static uint32_t machi, maclo, txlen, txfcs, mdio, rxfcs, rsr, rxlen, reset;
static int s, sockfd;

int select_wait(int sockfd)
{
  fd_set rfds;
  struct timeval tv;
  int retval;
  
  /* Watch sockfd to see when it has input. */
  FD_ZERO(&rfds);
  FD_SET(sockfd, &rfds);
  
  /* Wait up to five seconds. */
  tv.tv_sec = 0;
  tv.tv_usec = 10000;
  
  retval = select(1, &rfds, NULL, NULL, &tv);
  /* Don't rely on the value of tv now! */
  
  if (retval == -1)
    {
    perror("select()");
    return -1;
    }
  else if (retval)
    return 1;
  else
    return 0;
}

#define SERVER "192.168.0.51"
#define BUFLEN 1536  //Max length of buffer
#define PORT 8888   //The port on which to send data
#define CHUNK_SIZE 1024

void die(const char *s)
{
    perror(s);
    exit(1);
}

#define ETHER_TYPE	0x0800
#define DEFAULT_IF	"eth0"
#define BUF_SIZ		1536

static struct sockaddr_in si_other;
static char message[BUFLEN];
static char ifName[IFNAMSIZ];

void send_message(int s, uint16_t idx)
{
  memcpy(message+CHUNK_SIZE, &idx, sizeof(uint16_t));
  //send the message
  if (sendto(s,
	     message,
	     CHUNK_SIZE+sizeof(uint16_t),
	     0,
	     (struct sockaddr *) &si_other,
	     sizeof(si_other)) == -1)
    die("sendto()");
  usleep(10000);
}

int recv_message(int sockfd, int typ)
{
  int update = 0;
  uint32_t numbytes;
  uint8_t buf[BUF_SIZ];
  /* Header structures */
  struct ether_header *eh = (struct ether_header *) buf;
  struct iphdr *iph = (struct iphdr *) (buf + sizeof(struct ether_header));
  struct udphdr *udph = (struct udphdr *) (buf + sizeof(struct iphdr) + sizeof(struct ether_header));
  uint8_t *payload = (buf + sizeof(struct iphdr) + sizeof(struct ether_header) + sizeof(struct udphdr));
  do {
    numbytes = recvfrom(sockfd, buf, BUF_SIZ, 0, NULL, NULL);
    if ((numbytes > sizeof(struct iphdr) + sizeof(struct ether_header) + sizeof(struct udphdr)) &&
        (ntohs(eh->ether_type)==ETH_P_IP) &&
        (iph->protocol==IPPROTO_UDP))
      {
        int len = ntohs(udph->len) - sizeof(struct udphdr);
        int sport = ntohs(udph->source);
        int dport = ntohs(udph->dest);
        /* Check the packet is for me */
          if ((sport == 8888) && (dport == 8888)) {
          /* UDP payload length */
#if 1
          printf("listener: got packet %u bytes\n", len);
#endif
	  update = len > 0;
        }
      }
  }
  while ((numbytes > 0) && !update);
  return update;
}

void eth_close()
{
  close(s);
  close(sockfd);
}

eth_device_t::eth_device_t()
{
  char sender[INET6_ADDRSTRLEN];
  int i, ret = 0;
  int sockopt, restart = 0;
  int oldpercent = -1;
  struct ifreq ifopts;	/* set promiscuous mode */
  struct ifreq if_ip;	/* get ip addr */
  struct sockaddr_storage their_addr;
  uint8_t buf[BUF_SIZ];
  socklen_t peer_addr_size;
  int cfd, len, chunks, fd, slen, rslt;
  int incomplete = 1;
  uint16_t idx;
  char *m;

  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(PORT);
     
  if (inet_aton(SERVER, &si_other.sin_addr) == 0) 
    {
      fprintf(stderr, "inet_aton() failed\n");
      exit(1);
    }    
    
  if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) die("socket");

  strcpy(ifName, DEFAULT_IF);
  /* Header structures */
  struct ether_header *eh = (struct ether_header *) buf;
  struct iphdr *iph = (struct iphdr *) (buf + sizeof(struct ether_header));
  struct udphdr *udph = (struct udphdr *) (buf + sizeof(struct iphdr) + sizeof(struct ether_header));
  
  memset(&if_ip, 0, sizeof(struct ifreq));
  
  /* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
  if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETHER_TYPE))) == -1) {
    perror("listener: socket");	
    die("fatal");
  }
  
  /* Set interface to promiscuous mode - do we need to do this every time? */
  strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
  /* Bind to device */
  if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1)	{
    perror("SO_BINDTODEVICE");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  /* set read timeout */
  struct timeval read_timeout;
  read_timeout.tv_sec = 0;
  read_timeout.tv_usec = 100;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
  machi = 0x2301;
  maclo = 0x00890702;
}

bool eth_device_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
  int rslt, cnt;
  if (addr + len > 0x8000)
    return false;

  switch(addr)
    {
    case MACLO_OFFSET: printf("eth_load(MACLO_OFFSET);\n"); memcpy(bytes, &maclo, len); break;
    case MACHI_OFFSET: printf("eth_load(MACHI_OFFSET);\n"); memcpy(bytes, &maclo, len); break;
    case TPLR_OFFSET: printf("eth_load(TPLR_OFFSET);\n"); memcpy(bytes, &txlen, len); break;
    case TFCS_OFFSET: printf("eth_load(TFCS_OFFSET);\n"); memcpy(bytes, &txfcs, len); break;
    case MDIOCTRL_OFFSET: printf("eth_load(MDIOCTRL_OFFSET);\n"); memcpy(bytes, &mdio, len); break;
    case RFCS_OFFSET: printf("eth_load(RFCS_OFFSET);\n"); memcpy(bytes, &rxfcs, len); break;
    case RSR_OFFSET: printf("eth_load(RSR_OFFSET);\n"); memcpy(bytes, &rsr, len); break;
    case RPLR_OFFSET: printf("eth_load(RPLR_OFFSET);\n"); memcpy(bytes, &rxlen, len); break;
      
    }
  return true;
}

bool eth_device_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
  if (addr + len > 0x8000)
    return false;

  switch(addr)
    {
    case MACLO_OFFSET: printf("eth_store(MACLO_OFFSET);\n"); memcpy(&maclo, bytes, len); break;
    case MACHI_OFFSET: printf("eth_store(MACHI_OFFSET);\n"); memcpy(&machi, bytes, len); break;
    case TPLR_OFFSET: printf("eth_store(TPLR_OFFSET);\n"); memcpy(&txlen, bytes, len); break;
    case TFCS_OFFSET: printf("eth_store(TFCS_OFFSET);\n"); /* cancel */ break;
    case MDIOCTRL_OFFSET: printf("eth_store(MDIOCTRL_OFFSET);\n"); memcpy(&mdio, bytes, len); break;
    case RFCS_OFFSET: printf("eth_store(RFCS_OFFSET);\n"); /* unused */ break;
    case RSR_OFFSET: printf("eth_store(RSR_OFFSET);\n"); memcpy(&reset, bytes, len); break;
    case RPLR_OFFSET: printf("eth_store(RPLR_OFFSET);\n"); /* unused */ break;
      
    }

  return true;
}
