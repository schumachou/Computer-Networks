#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "4109" // the port client will be connecting to
#define PORT_STU1 "22209"
#define PORT_STU2 "22309"
#define PORT_STU3 "22409"
#define PORT_STU4 "22509"
#define PORT_STU5 "22609"
#define MAXBUFLEN 100

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void){
  
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET_ADDRSTRLEN];
  
  //use for getsockname
  struct sockaddr_in sin;
  socklen_t sin_len;
  
  //server info
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo("nunki.usc.edu", PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  
  for(int i = 0; i < 5; i++){
    if(fork() == 0){

      // loop through all the results and connect to the first we can
      for(p = servinfo; p != NULL; p = p->ai_next) {
	if ((sockfd = socket(p->ai_family, p->ai_socktype,
			     p->ai_protocol)) == -1) {
	  perror("client: socket\n");
	  continue; 
	}
	if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
	  close(sockfd);
	  perror("client: connect\n");
	  continue;
	}
	break; 
      }
      if (p == NULL) {
	fprintf(stderr, "client: failed to connect\n");
	return 2;
      }
      
      inet_ntop(p->ai_family,
		get_in_addr((struct sockaddr *)p->ai_addr),
		s, sizeof s);
      getsockname(sockfd, (struct sockaddr *)&sin,&sin_len);//retrieve the locally-bound port number wherever ports are assigned dynamically
      
      printf("<Student%d> has TCP port %d and IP address %s\n",
	      i+1, (int) ntohs(sin.sin_port), s);
      
      freeaddrinfo(servinfo); // all done with this structure     
      
      char infile[20];
          
      switch(i){
      case 0:
	strcpy(infile, "student1.txt");
	break;
      case 1:
	strcpy(infile, "student2.txt");
	break;
      case 2:
	strcpy(infile, "student3.txt");
	break;
      case 3:
	strcpy(infile, "student4.txt");
	break;
      default:
	strcpy(infile, "student5.txt");
      }
            
      //send data
      FILE *fp;
      char line[20];
      
      fp = fopen(infile,"r");
  
      if(fp != NULL){
	while(fgets(line,20,fp)){
	//for(int i =0; i<3; i++){
	  //fgets(line,20,fp);
	  char* token= strtok(line,"\n");
	  if(send(sockfd, token , strlen(token), 0) == -1){
	    perror("send\n");
	  }
	  /*
	  //test
	  printf("len of token: %d\n", strlen(token));
	  printf("token: %s\n",token);
	  printf("len of line: %d\n", strlen(line));
	  printf("sent: %s\n",line);
	  */
	  
	}
	char complete[] = "complete";
	if(send(sockfd, complete , sizeof(complete) , 0) == -1){
	  perror("send\n");
	}
	printf("Completed sending application for <Student%d>\n", i+1);
	
	
	//receive ACK
	int numbytes;
	char buf[12];
	if ((numbytes = recv(sockfd, buf, 2, 0)) == -1) {
	  exit(1);
	}	
	//buf[numbytes] = '\0';
	printf("<Student%d> has received the reply from the admission office\n",i+1);
	//printf("recieved ACK: %s\n", buf);//test    

	//write ack to file
	/*	
	FILE *ack;
	ack = fopen("p_ack1.txt","w");
	if(ack != NULL){
	  fprintf(ack, "%s\n", buf);
	  //printf("write ack\n");
	}
	else{
	  printf("fail to write ACK\n");
	}
	fclose(ack);
	*/
      }
      else{
	printf("Fail to open infile\n");
      } 
      fclose(fp);
      close(sockfd);
      exit(0);
    }
    close(sockfd);
  }
  /*
  int status1;
  wait(&status1);
  */
    int status1, status2, status3, status4, status5;
    wait(&status1);
    wait(&status2);
    wait(&status3);
    wait(&status4);
    wait(&status5);

  //******************** UDP CONNECTION ********************

  for(int i = 0; i < 5; i++){
    if(!fork()){
      //char infile[20];
      char port[10];
      
      switch(i){
      case 0:
	//strcpy(infile, "p_result_stu1.txt");
	strcpy(port, PORT_STU1);
	break;
      case 1:
	//strcpy(infile, "p_result_stu2.txt");
	strcpy(port, PORT_STU2);
	break;
      case 2:
	//strcpy(infile, "p_result_stu3.txt");
	strcpy(port, PORT_STU3);
	break;
      case 3:
	// strcpy(infile, "p_result_stu4.txt");
	strcpy(port, PORT_STU4);   
	break;
      default:
	//strcpy(infile, "p_result_stu5.txt");
	strcpy(port, PORT_STU5);
      }
      
      struct sockaddr_storage their_addr;
      socklen_t addr_len;
      
      char buf[MAXBUFLEN];
      int numbytes;
      
      memset(&hints, 0, sizeof hints);
      hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
      hints.ai_socktype = SOCK_DGRAM;
      hints.ai_flags = AI_PASSIVE; // use my IP
      if ((rv = getaddrinfo("nunki.usc.edu", port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
      }
      
      // loop through all the results and bind to the first we can
      for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
	  perror("listener: socket");
	  continue; }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
	  close(sockfd);
	  perror("listener: bind");
	  continue; }
        break; }
      if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
      }
      //show ip and port
      //memset(s, 0, sizeof(s));
      inet_ntop(p->ai_family,
		get_in_addr((struct sockaddr *)p->ai_addr),
		s, sizeof s);
      printf("<Student%d> has UDP port %s and IP address %s for Phase 2\n",
	     i+1, port, s );
      
      freeaddrinfo(servinfo);
      
      // printf("test: listener: waiting to recvfrom...\n");//test
      
      //receive the reusult from admission office
      addr_len = sizeof their_addr;
      if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
			       (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
      }
      buf[numbytes] = '\0';
      /*
	printf("listener: got packet from %s\n",
	inet_ntop(their_addr.ss_family,
	get_in_addr((struct sockaddr *)&their_addr),
	s, sizeof s));
      */
      //*****test*****
      // printf("test: listener: packet is %d bytes long\n", numbytes);
      // printf("test: student%d: \"%s\"\n", i+1, buf);
      //**************
      printf("<Student%d> has received the application result\n", i+1);
      printf("End of phase 2 for <Student%d>\n", i+1);
      close(sockfd);
      exit(0);
    }
    close(sockfd);
   
  }
  int status1_udp, status2_udp, status3_udp, status4_udp, status5_udp;
  wait(&status1_udp);
  wait(&status2_udp);
  wait(&status3_udp);
  wait(&status4_udp);
  wait(&status5_udp);  
  
  return 0;
}
