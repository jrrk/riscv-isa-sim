/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <getopt.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/stat.h>
#include <signal.h>
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
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>

#include "devices.h"
#include "eth.h"

#define MY_DEST_MAC0	0x00
#define MY_DEST_MAC1	0x00
#define MY_DEST_MAC2	0x00
#define MY_DEST_MAC3	0x00
#define MY_DEST_MAC4	0x00
#define MY_DEST_MAC5	0x00

#define DEFAULT_IF	"eth0"
#define ETHER_TYPE	0x0800
#define BUF_SIZ		1536

enum {DEBUG=0, qlen=4095};

struct inq {
  uint8_t buf[BUF_SIZ];
  int len;
} inq[qlen+1];

static  uint8_t mac_addr_rev[ETH_HLEN];
static 	int sockfd, ret, i;
static 	ssize_t numbytes;
static 	struct ifreq ifopts;	/* set promiscuous mode */
static 	struct sockaddr_storage their_addr;
static 	char ifName[IFNAMSIZ];
static 	int packet_socket;
static 	struct ifreq if_idx;
static 	struct ifreq if_mac;
static 	char sendbuf[BUF_SIZ];
static int head, tail, irq;

// FILE *logf;

void inqueue(const uint8_t *buf, int len)
{
  inq[head].len = len;
  memcpy(inq[head++].buf, buf, len);
  head &= qlen;
#if 0
  /* Print packet */
  fprintf(logf, "\tData:");
  for (i=0; i<len; i++) fprintf(logf, "%02x:", buf[i]);
  fprintf(logf, "\n");
  fflush(logf);
#endif  
}

void die(const char *s)
{
    perror(s);
    exit(1);
}

int recv(void *dummy)
{
  enum {macrev=1, promisc=1};
  int sockopt;
  //  logf = fopen("recv.log", "w");

  /* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
  if ((packet_socket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
    die("listener: socket");	
  }
  
  /* Set interface to promiscuous mode (or not) - do we need to do this every time? */
  strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
  ioctl(packet_socket, SIOCGIFFLAGS, &ifopts);
  if (promisc)
    ifopts.ifr_flags |= IFF_PROMISC;
  else
    ifopts.ifr_flags &= ~IFF_PROMISC;
  ioctl(packet_socket, SIOCSIFFLAGS, &ifopts);
  
  /* Get the MAC address of the interface to send on */
  if (ioctl(packet_socket, SIOCGIFHWADDR, &ifopts) < 0)
    perror("SIOCGIFHWADDR");
  
  for (i = 0; i < ETH_ALEN; i++)
    {
      uint8_t crnt = ((uint8_t *)&ifopts.ifr_hwaddr.sa_data)[i];
      if (macrev)
        {
          /* reverse the MAC addr (quick hack to distinguish from host comms!) */
          mac_addr_rev[i] = crnt;
        }
      else
        {
          mac_addr_rev[ETH_ALEN-1-i] = crnt;
        }
      
      if (DEBUG) printf("mac[%d] = %.2X\n", i, mac_addr_rev[i]);
    }
  
  /* Allow the socket to be reused - incase connection is closed prematurely */
  if (setsockopt(packet_socket, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
    perror("setsockopt");
    close(packet_socket);
    exit(EXIT_FAILURE);
  }
  /* Bind to device */
  if (setsockopt(packet_socket, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1)	{
    perror("SO_BINDTODEVICE");
    close(packet_socket);
    exit(EXIT_FAILURE);
  }
  while (1)
    {
      //      if (irq)
        {
          uint8_t buf[BUF_SIZ];
          /* Header structures */
          struct ether_header *eh = (struct ether_header *) buf;
          struct iphdr *iph = (struct iphdr *) (buf + sizeof(struct ether_header));
          struct udphdr *udph = (struct udphdr *) (buf + sizeof(struct iphdr) + sizeof(struct ether_header));
          numbytes = recvfrom(packet_socket, buf, BUF_SIZ, 0, NULL, NULL);
          //	printf("listener: got packet %lu bytes\n", numbytes);
          
          /* Check the packet is for me */
            if (eh->ether_dhost[0] == mac_addr_rev[5] &&
                eh->ether_dhost[1] == mac_addr_rev[4] &&
                eh->ether_dhost[2] == mac_addr_rev[3] &&
                eh->ether_dhost[3] == mac_addr_rev[2] &&
                eh->ether_dhost[4] == mac_addr_rev[1] &&
                eh->ether_dhost[5] == mac_addr_rev[0]) {
            //		printf("Correct destination MAC address\n");
            inqueue(buf, numbytes);
          } else if ((    eh->ether_dhost[0] &
                          eh->ether_dhost[1] &
                          eh->ether_dhost[2] &
                          eh->ether_dhost[3] &
                          eh->ether_dhost[4] &
                          eh->ether_dhost[5]) == 0xFF) {
            
            //		printf("Broadcast destination MAC address\n");
            inqueue(buf, numbytes);
          }
          else {
            /*		printf("Wrong destination MAC: %x:%x:%x:%x:%x:%x\n",
                        eh->ether_dhost[0],
                        eh->ether_dhost[1],
                        eh->ether_dhost[2],
                        eh->ether_dhost[3],
                        eh->ether_dhost[4],
                        eh->ether_dhost[5]);
            */
          }
          //          usleep(10000);
        }
    }
  return 0;
}

int send(int tx_len)
{
  struct ether_header *eh = (struct ether_header *) sendbuf;
  /* Construct the Ethernet header */
  struct sockaddr_ll socket_address;
  memset(&socket_address, 0, sizeof(socket_address));
  
  /* Index of the network device */
  socket_address.sll_ifindex = if_idx.ifr_ifindex;
  /* Address length*/
  socket_address.sll_halen = ETH_ALEN;
  /* Destination MAC */
  for (i = 0; i < ETH_ALEN; i++)
    socket_address.sll_addr[i] = eh->ether_dhost[i];

  /* Send packet */
  if (sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
    printf("Send failed\n");
#if 0
  /* Print packet */
  printf("\tData(%d):", tx_len);
  for (i=0; i<tx_len; i++) printf("%02x:", sendbuf[i]);
  printf("\n");
#endif
  return 0;
}

static uint32_t txlen, txfcs, mdio;
static struct termios oldtty;

eth_device_t::eth_device_t()
{
  int receiver;
  strcpy(ifName, DEFAULT_IF);
 
  /* Open RAW socket to send on */
  if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
    perror("socket");
  }
  
  /* Get the index of the interface to send on */
  memset(&if_idx, 0, sizeof(struct ifreq));
  strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
  if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
    perror("SIOCGIFINDEX");
  
  receiver = clone(recv, malloc(65536), CLONE_VM|CLONE_FILES|CLONE_FS|CLONE_IO|CLONE_SIGHAND|CLONE_THREAD, NULL);
  if (receiver < 0)
    die("clone");
}

bool eth_device_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
  uint16_t machi0;
  uint32_t machi, rxfcs, rsr, rxlen;
  int rslt, cnt;
  if (addr + len > 0x8000)
    return false;

  switch(addr)
    {
    case MACLO_OFFSET:
      if (DEBUG) printf("eth_load(MACLO_OFFSET);\n");
      memcpy(bytes, mac_addr_rev, len);
      break;
    case MACHI_OFFSET:
      if (DEBUG) printf("eth_load(MACHI_OFFSET);\n");
      memcpy(&machi0, mac_addr_rev+4, 2);
      machi = machi0;
      memcpy(bytes, &machi, len);
      break;
    case TPLR_OFFSET:
      //      if (DEBUG) printf("eth_load(TPLR_OFFSET);\n");
      memcpy(bytes, &txlen, len);
      break;
    case TFCS_OFFSET:
      if (DEBUG) printf("eth_load(TFCS_OFFSET);\n");
      memcpy(bytes, &txfcs, len);
      break;
    case MDIOCTRL_OFFSET:
      if (DEBUG) printf("eth_load(MDIOCTRL_OFFSET);\n");
      memcpy(bytes, &mdio, len);
      break;
    case RFCS_OFFSET:
      if (DEBUG) printf("eth_load(RFCS_OFFSET);\n");
      rxfcs = 0xc704dd7b;
      memcpy(bytes, &rxfcs, len);
      break;
    case RSR_OFFSET:
      if (DEBUG) printf("eth_load(RSR_OFFSET);\n");
      rsr = (head != tail) ? RSR_RECV_DONE_MASK : 0;
      memcpy(bytes, &rsr, len);
      break;
    case RPLR_OFFSET:
      rxlen = inq[tail].len + 4;
      memcpy(bytes, &rxlen, len);
      //      printf("rxlen=%d\n", rxlen);
      break;
    default:
      memcpy(bytes, inq[tail].buf+addr, len);
      break;        
    }
  return true;
}

bool eth_device_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
  uint32_t machi;
  if (addr + len > 0x8000)
    return false;

  switch(addr)
    {
    case MACLO_OFFSET:
      if (DEBUG) printf("eth_store(MACLO_OFFSET);\n");
      //      memcpy(&maclo, bytes, len);
      break;
    case MACHI_OFFSET:
      if (DEBUG) printf("eth_store(MACHI_OFFSET);\n");
      memcpy(&machi, bytes, len);
      irq = machi & MACHI_IRQ_EN ? 1 : 0;
      break;
    case TPLR_OFFSET:
      //      if (DEBUG) printf("eth_store(TPLR_OFFSET);\n");
      memcpy(&txlen, bytes, len);
      send(txlen);
      break;
    case TFCS_OFFSET:
      if (DEBUG) printf("eth_store(TFCS_OFFSET);\n");
      /* cancel */
      break;
    case MDIOCTRL_OFFSET:
      if (DEBUG) printf("eth_store(MDIOCTRL_OFFSET);\n");
      memcpy(&mdio, bytes, len);
      break;
    case RFCS_OFFSET:
      if (DEBUG) printf("eth_store(RFCS_OFFSET);\n");
      /* unused */
      break;
    case RSR_OFFSET:
      if (DEBUG) printf("eth_store(RSR_OFFSET);\n");
      if (head != tail)
        {
          ++tail;
          tail &= qlen;
        }
      break;
    case RPLR_OFFSET:
      if (DEBUG) printf("eth_store(RPLR_OFFSET);\n");
      /* unused */
      break;
    default:
      memcpy(sendbuf+addr-TXBUFF_OFFSET, bytes, len); break;  
    }

  return true;
}
