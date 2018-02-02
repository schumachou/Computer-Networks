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

#define PORT "4109" // the port of admission office
#define PORT_DEPA "21909"
#define PORT_DEPB "22009"
#define PORT_DEPC "22109"
#define MAXBUFLEN 100

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
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
  
  if ((rv = getaddrinfo("nunki.usc.edu", PORT, &hints, &servinfo)) != 0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  
  for(int i=0; i<3;i++){
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
      
      freeaddrinfo(servinfo); // all done with this structure     
      
      char infile[20];
      char department;
      
      switch(i){
      case 0:
	strcpy(infile, "departmentA.txt");
	department = 'A';
	break;
      case 1:
	strcpy(infile, "departmentB.txt");
	department = 'B';
	break;
      default:
	strcpy(infile, "departmentC.txt");
	department = 'C';
      }

      inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
		s, sizeof s);      
      getsockname(sockfd, (struct sockaddr *)&sin,&sin_len); //retrieve the locally-bound port number wherever ports are assigned dynamically

      printf( "<Department%c> has TCP port %d and IP address %s for Phase 1\n",
	      department , (int) ntohs(sin.sin_port), s);      
      printf("<Department%c> is now connected to the admission office\n",
	     department);
      
      
      //send data
      FILE *fp;
      char line[20];
      
      fp = fopen(infile,"r");
  
      if(fp != NULL){

	for(int i =0; i<3; i++){
	  fgets(line,20,fp);
	  char* token= strtok(line,"\n");
	  if(send(sockfd, token , strlen(token), 0) == -1){
	    perror("send\n");
	  }
	  printf("<Department%c has sent %s to the admission office\n", 
		 department, strtok(line,"#"));
	}
	printf("Updating the admission office is done for <Department%c>\n",
	       department);

      }
      else{
	printf("Fail to open the file\n");
      }  
      fclose(fp);
      close(sockfd);
      printf("End of Phase 1 for <Department%c>\n",department); 
      exit(0);
    }
    close(sockfd);
  }
  
  int status1, status2, status3;
  wait(&status1);
  wait(&status2);
  wait(&status3);
  
  //******************** UDP CONNECTION ********************

  for(int i = 0; i < 3; i++){
    if(!fork()){
      char port[10];
      char dep;
      
      switch(i){
      case 0:
	strcpy(port, PORT_DEPA);
	dep = 'A';
	break;
      case 1:
	strcpy(port, PORT_DEPB);
	dep = 'B';
	break;
      default:
	strcpy(port, PORT_DEPC);
	dep = 'C';
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
	  perror("department: socket");
	  continue; 
	}
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
	  close(sockfd);
	  perror("department: bind");
	  continue;
	}
        break; 
      }
      if (p == NULL) {
        fprintf(stderr, "department%c: failed to bind socket\n", dep);
        return 2;
      }
      //show ip and port
      //memset(s, 0, sizeof(s));
      inet_ntop(p->ai_family,
		get_in_addr((struct sockaddr *)p->ai_addr),
		s, sizeof s);
      printf("<Department%c> has UDP port %s and IP address %s for Phase 2\n",
	     dep, port, s );
      
      freeaddrinfo(servinfo);
      
      // printf("test: listener: waiting to recvfrom...\n");//test
      
      //receive the reusult from admission office
      addr_len = sizeof their_addr;
      numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
			  (struct sockaddr *)&their_addr, &addr_len);
      while(numbytes != 9){// 9 stands for "complete"
	if ( numbytes == -1) {
	  perror("recvfrom");
	  exit(1);
	}
	buf[numbytes] = '\0';
	
	//*****test*****
	// printf("test: listener: packet is %d bytes long\n", numbytes);
	// printf("test: department%c: \"%s\"\n", dep, buf);
	//**************
	
	printf("<Student%c> has been admitted to <Department%c>\n",buf[7], dep);
	numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
			    (struct sockaddr *)&their_addr, &addr_len);
      }
      printf("End of phase 2 for <Department%c>\n", dep);
      close(sockfd);
      exit(0);
    }
    close(sockfd);
   
  }
  int status1_udp, status2_udp, status3_udp;
  wait(&status1_udp);
  wait(&status2_udp);
  wait(&status3_udp);
  
  
  
  return 0;
}
