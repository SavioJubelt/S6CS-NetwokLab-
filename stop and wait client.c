#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>  // for timeout functionality

typedef struct packet {
  char data[1024];
} Packet;

typedef struct frame {
  int frame_kind; // ACK:0, SEQ:1 FIN:2
  int sq_no;
  int ack;
  Packet packet;
} Frame;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <port>", argv[0]);
    exit(0);
  }

  int port = atoi(argv[1]);
  int sockfd;
  struct sockaddr_in serverAddr;
  char buffer[1024];
  socklen_t addr_size;

  int frame_id = 0;
  Frame frame_send;
  Frame frame_recv;
  int ack_recv = 1;

  // Create UDP socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket creation failed");
    exit(1);
  }

  // Set receive timeout for reliable data transfer
  struct timeval timeout;
  timeout.tv_sec = 5; // Set timeout to 5 seconds
  timeout.tv_usec = 0;  // No microseconds
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
    perror("setsockopt failed");
    exit(1);
  }

  memset(&serverAddr, '\0', sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  while (1) {

    if (ack_recv == 1) {
      frame_send.sq_no = frame_id;
      frame_send.frame_kind = 1; // SEQ (data frame)
      frame_send.ack = 0;

       printf("Enter Data/exit: ");
       fgets(buffer,1024, stdin);

      strcpy(frame_send.packet.data, buffer);

      // Send the frame with timeout handling
      int bytes_sent = sendto(sockfd, &frame_send, sizeof(Frame), 0,
                              (struct sockaddr*)&serverAddr, sizeof(serverAddr));
      if (bytes_sent < 0) {
        perror("sendto failed");
        exit(1);
      }
	if (strncmp(buffer, "exit", 4) == 0)
                           break;


      printf("[+]Frame Send\n");
    }

    addr_size = sizeof(serverAddr);
    int f_recv_size; // Use a variable to capture return value

    // Receive data with timeout check
    f_recv_size = recvfrom(sockfd, &frame_recv, sizeof(frame_recv), 0,
                           (struct sockaddr*)&serverAddr, &addr_size);

    // Handle potential timeout or other errors
    if (f_recv_size < 0) {
      // Check for timeout specifically
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        printf("[-] Timeout on  waiting for acknowledgement resend the data \n");
      } else {
        perror("recvfrom failed");
        exit(1);
      }
    } else { // Data received successfully
      if (f_recv_size > 0 && frame_recv.sq_no == 0 && frame_recv.ack == frame_id + 1) {
        printf("[+]Ack Received\n");
        ack_recv = 1;
      } else {
        printf("[-] Unexpected packet or incorrect acknowledgement recieved resnd\n");
        // Handle this case appropriately, e.g., resend the frame
      }
    }

    frame_id++;
  }

  close(sockfd);
  return 0;
}
