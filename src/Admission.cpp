#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>

#define PORT "4109"  // the port users will be connecting to
#define PORT_DEPA "21909"
#define PORT_DEPB "22009"
#define PORT_DEPC "22109"
#define PORT_STU1 "22209"
#define PORT_STU2 "22309"
#define PORT_STU3 "22409"
#define PORT_STU4 "22509"
#define PORT_STU5 "22609"
#define BACKLOG 20   // how many pending connections queue will hold
#define MAXDATASIZE 20 // max number of bytes we can get at once


struct program{
  char name[10];
  double requirement;
  struct program *next;
};

//reap dead processes
void sigchld_handler(int s){
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void){

  // listen on sock_fd, new connection on new_fd
  int sockfd, new_fd;
  struct addrinfo hints, *servinfo, *p;  

  // use for accept
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;  

  //reaping zombie processes that appear as the fork()ed child processes exit
  struct sigaction sa;
  
  int yes=1;// use for reuse the port when bind
  char s[INET_ADDRSTRLEN];
  int rv; // use for getaddrinfo return
    
  // load up address structs with getaddrinfo():
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP
  
  if ((rv = getaddrinfo("nunki.usc.edu", PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  
  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
			 p->ai_protocol)) == -1) {
      perror("server: socket\n");
      continue; 
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
		   sizeof(int)) == -1) {
      perror("setsockopt\n");
      exit(1); }
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind\n");
      continue; 
    }
    break; 
  }
  if (p == NULL)  {
    fprintf(stderr, "server: failed to bind\n");
    return 2;
  }
  
  //show TCP port and ip for phase1
  inet_ntop(p->ai_family,
	    get_in_addr((struct sockaddr *)p->ai_addr),
	    s, sizeof s);
  printf("The admission office has TCP port %s and IP address %s\n",
	 PORT, s);
  
  // freeaddrinfo(servinfo); // all done with this structure

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen\n");
    exit(1);
  }
  
  // reap all dead processes
  sa.sa_handler = sigchld_handler; 
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigactio\n");
    exit(1); 
  }
  
  // accept from department
  for(int i=0; i<3; i++){  
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept\n");
      continue;
    }
    if(!fork()){
      close(sockfd);

      // receive
      int numbytes;
      char buf[MAXDATASIZE];
      FILE *fp;
      fp = fopen("program.txt","a");
      if(fp != NULL){
	for(int j = 0; j < 3; j++){
	  if ((numbytes = recv(new_fd, buf, 6, 0)) == -1) {
	    exit(1);
	  }
	  buf[numbytes] = '\0';
	  
	  //printf("received: %s\n", buf);//test

	  //store in file
	  fprintf(fp, "%s\n", buf);
	  /*
	  //store in array
	  char *token;
	  
	  token = strtok(buf, "#");     
	  strcpy(PROG[j].name, token );
	  
	  token = strtok(NULL, "#");
	  PROG[j].requirement = atof(token);
	  */
	}
      }
      else{
	printf("Fail to save data\n");
      }
      printf("Received the program list from <Department%c>\n",65+i);

      fclose(fp);
      close(new_fd);
      exit(0);
    }
    close(new_fd);//parent socket result from fork()   
  }
  int status1, status2, status3;
  wait(&status1);
  wait(&status2);
  wait(&status3); 
 
  printf("End of Phase 1 for the admission office\n");
  



  //read data from a file to a linked list
  char line[20];
  char *token;
  struct program *PROG = NULL;
  FILE *fp;
  fp = fopen("program.txt","r");

  if(fp != NULL){
    while(fgets(line,20,fp)){
      struct program *prog =(struct program *)malloc(sizeof(struct program));
      token = strtok(line, "#");
      strcpy(prog->name, token);
      token = strtok(NULL, "\0");
      prog->requirement =atof(token);
      prog->next = PROG;
      PROG = prog;
    }
  }
  else{
    printf("Fail to read data\n");
  }

  /*
  //test for the linked list
  printf("*****test for linked list*****\n");
  struct program *ptr = NULL;
  for(ptr = PROG; ptr != NULL; ptr = ptr->next){
    printf("name: %s\n", ptr->name);
    printf("gpa: %2.1f\n", ptr->requirement);
  }
  printf("******************************\n");
  */

  //show TCP port and ip for phase2
  inet_ntop(p->ai_family,
	    get_in_addr((struct sockaddr *)p->ai_addr),
	    s, sizeof s);
  printf("The admission office has TCP port %s and IP address %s\n",
	 PORT, s);
  
  freeaddrinfo(servinfo); // all done with this structure

  // accept from student
  for(int i = 0; i < 5; i++){  
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }
    if(!fork()){
      close(sockfd);
      
      // receive
      int numbytes;
      char buf[12];
      char *token;

      struct apply{
	double gpa;
	std::string prog[3];
      }studentApply;
      int numApply = 0;

      //get GPA
      numbytes = recv(new_fd, buf, 7, 0);
      if (numbytes == -1) {
	exit(1);
      }
      buf[numbytes] = '\0';
      // printf("received: %s\n", buf);//test

      token = strtok(buf, ":");
      token = strtok(NULL, "\0");
      studentApply.gpa = atof(token);
      
      //get interest
      numbytes = recv(new_fd, buf, 12, 0);
      while(numbytes != 9){// 9 stands for "complete"
	if (numbytes == -1) {
	  exit(1);
	}
        buf[numbytes] = '\0';
  	//printf("received: %s\n", buf);//test
	  	  
	token = strtok(buf, ":");   
	token = strtok(NULL, "\0");
	studentApply.prog[numApply++] = token;

	numbytes = recv(new_fd, buf, 12, 0);
      }
     
      printf("Admission office receive the application from <Student%d>\n",i+1);
      /*
      //test for student data 
      printf("*****test for stuent data*****\n");
      printf("gpa: %2.1f\n", studentApply.gpa);
      for(int j = 0; j < numApply; j++){
	printf("interest%d: %s\n", j+1, studentApply.prog[j].c_str());
      } 
      printf("number of application: %d\n", numApply);
      printf("******************************\n");
      
      
      //test for the linked list
      printf("*****test for linked list*****\n");
      struct program *ptr = NULL;
      for(ptr = PROG; ptr != NULL; ptr = ptr->next){
	printf("name: %s\n", ptr->name);
	printf("gpa: %2.1f\n", ptr->requirement);
      }
      printf("******************************\n");
      */

      //review application
      bool accept = false;
      bool valid = false;
      struct program *tmp = NULL;
      for(int j = 0; j < numApply; j++){
	for(tmp = PROG; tmp != NULL; tmp = tmp->next){
	  // printf("interest%d: %s\n", j+1, student.prog[j].c_str());//
	  // printf("program: %s\n", tmp->name);//
	  // printf("------------------------\n");//
	  if(!strcmp(studentApply.prog[j].c_str(), tmp->name)){
	    // printf("student gpa: %2.1f\n", student.gpa);//
	    //printf("requirement: %2.1f\n", tmp->requirement);//
	    valid = true;
	    if(studentApply.gpa >= tmp->requirement){
	      accept = true;
	    }
	    break;
	  }
	}
	if(accept){
	  break;  
        }
      }
      /*
      //test for review result
      printf("*****test for review result*****\n");
      printf("accept: %d\n", accept);
      printf("valid: %d\n", valid);
      if(tmp != NULL){
      printf("program: %s, requirement: %2.1f, stuent GPA: %2.1f\n",
	       tmp->name,tmp->requirement, studentApply.gpa);
      }
      printf("********************************\n");	
      */
      if(tmp != NULL){
	char file_stu[20];
	switch(i){
	case 0:
	  strcpy(file_stu, "p_result_stu1.txt");
	  break;
	case 1:
	  strcpy(file_stu, "p_result_stu2.txt");
	  break;
	case 2:
	  strcpy(file_stu, "p_result_stu3.txt");
	  break;
	case 3:
	  strcpy(file_stu, "p_result_stu4.txt");
	  break;
	default:
	  strcpy(file_stu, "p_result_stu5.txt");
	}
       
	char file_dep[20];
	switch(tmp->name[0]){
	case 'A':
	  strcpy(file_dep, "p_result_depA.txt");
	  break;
	case 'B':
	  strcpy(file_dep, "p_result_depB.txt");
	  break;
	default:
	  strcpy(file_dep, "p_result_depC.txt");
	}
	
	//store the result in file sent to student
	fp = fopen(file_stu,"w");
	if(fp != NULL){
	  if(accept){
	    fprintf(fp, "Accept#%s#department%c\n", tmp->name, tmp->name[0]);
	    // printf("store: Accept#%s#department%c\n", tmp->name, tmp->name[0]);//test
	  }
	  else{
	    fprintf(fp, "Reject");
	    // printf("store: Reject\n");//test
	  } 
	}
	else{
	  printf("fail to write result for student\n");
	}
	fclose(fp);
	
	//store the result in file sent to department
	fp = fopen(file_dep,"a");
	if(fp != NULL){
	  if(accept){
	    fprintf(fp, "Student%d#%2.1f#%s\n",
		  i+1, studentApply.gpa, tmp->name);
	    // printf("store: Student%d#%2.1f#%s\n", i+1, studentApply.gpa, tmp->name);//
	  }
	}
	else{
	  printf("fail to write the result for department\n");
	}
	fclose(fp);
      }//end of if(tmp != NULL)
      
      
      //send ACK
      if(!valid){
	if(send(new_fd, "0" , 2, 0) == -1){
	    perror("send\n");
	}
      }
      else{
	if(send(new_fd, "1" , 2, 0) == -1){
	    perror("send\n");
	}  
      }                 
      close(new_fd);
      exit(0);
      }
      close(new_fd);//parent socket result from fork()   
    }
  /*
  int status_std1;
  wait(&status_std1);
  */
  int status_std1, status_std2, status_std3, status_std4, status_std5;
  wait(&status_std1);
  wait(&status_std2);
  wait(&status_std3); 
  wait(&status_std4); 
  wait(&status_std5);   
 
  free(PROG);

  //***************** UDP CONNECTION FOR STUDENT *********************

  // for students
  for(int i = 0; i < 5; i++){
    if(fork() == 0){
      char infile[20];
      char port[10];
      
      switch(i){
      case 0:
	strcpy(infile, "p_result_stu1.txt");
	strcpy(port, PORT_STU1);
	break;
      case 1:
	strcpy(infile, "p_result_stu2.txt");
	strcpy(port, PORT_STU2);
	break;
      case 2:
	strcpy(infile, "p_result_stu3.txt");
	strcpy(port, PORT_STU3);
	break;
      case 3:
	strcpy(infile, "p_result_stu4.txt");
	strcpy(port, PORT_STU4);   
	break;
      default:
	strcpy(infile, "p_result_stu5.txt");
	strcpy(port, PORT_STU5);
      }
      
      memset(&hints, 0, sizeof hints);
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_DGRAM;
      
      if ((rv = getaddrinfo("nunki.usc.edu", port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
      }
      
      // loop through all the results and make a socket
      for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
	  perror("admission: socket");
	  continue;
        }
        break;
      }
      if (p == NULL) {
        fprintf(stderr, "admission: failed to bind socket\n");
        return 2;
      }
      
      //send the result to student
      fp =fopen(infile,"r");
      if(fp != NULL){
	int numbytes;
	char packet[22];
	fgets(packet,22,fp);
	
	if ((numbytes = sendto(sockfd, packet, sizeof(packet), 0,
			       p->ai_addr, p->ai_addrlen)) == -1) {
	  perror("admission: sendto");
	  exit(1); 
	}
	
	// printf("talker: sent %d bytes to %s\n", numbytes, "listener");//test
	// printf("%s\n",packet);//test
        
	//show UDP ip and port 
	struct sockaddr_in sin;
	socklen_t sin_len;
	
	sin_len = sizeof sin;
	inet_ntop(p->ai_family,get_in_addr((struct sockaddr *)p->ai_addr),
		  s, sizeof s);
	getsockname(sockfd, (struct sockaddr *)&sin,&sin_len);//retrieve the locally-bound port number wherever ports are assigned dynamically
	printf("The admission office has UDP port %d and IP address %s for Phase 2\n", (int)ntohs(sin.sin_port), s);
	printf("The admission office has send the application result to <Student%d>\n", i+1);
	
      }
      else{
	printf("fail to read file\n");
      }
      fclose(fp);
      freeaddrinfo(servinfo);
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

 //*************** UDP CONNECTION FOR DEPARTMENT *******************

  for(int i= 0; i<3; i++){
    if(!fork()){
      char infile[20];
      char port[10];
      char dep;
      
      switch(i){
      case 0:
	strcpy(infile, "p_result_depA.txt");
	strcpy(port, PORT_DEPA);
	dep = 'A';
	break;
      case 1:
	strcpy(infile, "p_result_depB.txt");
	strcpy(port, PORT_DEPB);
	dep = 'B';
	break;
	
      default:
	strcpy(infile, "p_result_depC.txt");
	strcpy(port, PORT_DEPC);
	dep = 'C';
      }
      
      memset(&hints, 0, sizeof hints);
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_DGRAM;
      
      if ((rv = getaddrinfo("nunki.usc.edu", port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
      }
      
      // loop through all the results and make a socket
      for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
	  perror("talker: socket");
	  continue;
        }
        break;
      }
      if (p == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
      }
      
      //send the result to student
      fp =fopen(infile,"r");
      if(fp != NULL){
	int numbytes;
	char packet[20];
	while(fgets(packet,20,fp)){
	  char* token= strtok(packet,"\n");
	  if ((numbytes = sendto(sockfd, token, strlen(token), 0,
				 p->ai_addr, p->ai_addrlen)) == -1) {
	    perror("admmsion: sendto");
	    exit(1); 
	  }
	}
	char complete[] = "complete";
	if ((numbytes = sendto(sockfd, complete, sizeof(complete), 0,
			       p->ai_addr, p->ai_addrlen)) == -1) {
	  perror("admmsion: sendto");
	  exit(1); 
	}
	
	// printf("talker: sent %d bytes to %s\n", numbytes, "listener");//test
	// printf("%s\n",packet);//test
	
	//show UDP ip and port 
	struct sockaddr_in sin;
	socklen_t sin_len;
	
	sin_len = sizeof sin;
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
		  s, sizeof s);
	getsockname(sockfd, (struct sockaddr *)&sin,&sin_len);//retrieve the locally-bound port number wherever ports are assigned dynamically
	printf("The admission office has UDP port %d and IP address %s for Phase 2\n", (int)ntohs(sin.sin_port), s);
	
	printf("The admission office has send the application result to <Department%c>\n", dep);
	
      }
      else{
	printf("fail to read file\n");
      }
      fclose(fp);
      freeaddrinfo(servinfo);
      close(sockfd);
      exit(0);
    }
    close(sockfd);
  }
    int status1_udp_dep, status2_udp_dep, status3_udp_dep;
    wait(&status1_udp_dep);
    wait(&status2_udp_dep);
    wait(&status3_udp_dep);
  
  printf("End of Phase 2 for the admission office\n");
  
  return 0; 
}

  
