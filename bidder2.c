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
#include <malloc.h>

#define ServerPORT1 "1128"
#define bidderudp "3228"
#define biddertcp "4228"
#define MAXDATASIZE 256
#define HOSTNAME "nunki.usc.edu"
#define stop "term"
#define fileid "Bidder#2"
#define bidtxt "bidding2.txt"
#define logintxt "bidderPass2.txt"
#define BACKLOG 10

typedef struct bidding{
    char id[MAXDATASIZE];
    char item[MAXDATASIZE];
    char price[MAXDATASIZE];
    struct bidding *next;
}bidding,bid;


void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) >0);
}


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int authentication(char *read, char *filename)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in sin;
    socklen_t sin_len;
    char buffer[MAXDATASIZE];
    //char s[INET6_ADDRSTRLEN];
    int rv;
    struct sigaction sa;
    int yes = 1;
    
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    FILE *infile;
    /*
	 * L74 to L99 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/* create socket */

    memset (&hints, 0 ,sizeof hints);
    //hints.ai_family = AF_UNSPEC;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((rv = getaddrinfo(HOSTNAME, ServerPORT1, &hints, &servinfo)) != 0) {
        fprintf (stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    }
    
    char s[MAXDATASIZE];
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "server: failed to connect\n");
        return 1;
    }
    
    //  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s ,sizeof s);
    //    printf("Phase 1:Auction server has TCP port number %s and IP address %s \n", ServerPORT1, s);
    int getsock_check = getsockname(sockfd, (struct sockaddr *)&sin, &sin_len);
    if (getsock_check == -1) {
        perror("getsockname");
        exit(1);
    }
    printf("Phase 1: <%s>  has TCP port %d and IP address %s \n", filename,(int) ntohs(sin.sin_port), inet_ntoa(sin.sin_addr));
    freeaddrinfo(servinfo);
    //read file
    
    char temp[MAXDATASIZE];
    char buff[MAXDATASIZE];
    char cmd[MAXDATASIZE];
    char type[MAXDATASIZE];
    char username[MAXDATASIZE];
    char password[MAXDATASIZE];
    char account[MAXDATASIZE];
    char port[MAXDATASIZE];
    char reply[MAXDATASIZE];
    int numbytes;
    
    infile = fopen (read,"r");
    if(infile==NULL) {
        printf("%s ","file do not exit\n");
        exit(0);
    }
    
    if(fgets(temp,sizeof(temp),infile) == NULL) {
        printf("login document is invalid \n");
        exit(0);
    }
    if(sscanf(temp,"%s %s %s %s",type, username , password, account) != 4) {
        printf("missing account information \n");
        return 1;
    }
    strcpy(cmd,"<");
    strcat(cmd,filename);
    strcat(cmd,">");
    strcat(cmd, "Login#");
    strcat(cmd, temp);
   // printf("%s\n",cmd);
    int t;
    if ((t=send(sockfd, cmd,strlen(cmd),0) )== -1) {
        perror("send");
        close(sockfd);
        exit(0);
    }
    //   printf("%d\n",t);
    printf("Phase 1: Login request.User: %s password %s Bank account: %s \n", username, password, account);
    
    if ((numbytes = recv(sockfd, buff, MAXDATASIZE, 0)) == -1) {
        perror("recv");
        close(sockfd);
        exit (1);
    }
    buff[numbytes] = '\0';
    char ip[MAXDATASIZE];
    sscanf(buff,"%s %s %s",reply, port,ip);
    printf("Phase 1: Login request reply: %s. \n", reply);
    close(sockfd);
    if(strcmp("Rejected#",reply)==0) {
        return 1;
    }
    else {
        return 0;
    }
}

int Auction(char *inputfilename,char *biddername){
    int sockfd,new_fd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    struct sockaddr_in client;
    char buf3[MAXDATASIZE];
    char temp[MAXDATASIZE];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    bid *header, *bidelement;
    bid *header1,*incomeinfo;
    header1 =NULL;
    header = NULL;
    socklen_t sin_size;
    
    
    /*
	 * L193 to L243 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/* create socket */

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    FILE *bidinfo;
    bidinfo = fopen(inputfilename,"r");
    if(bidinfo== NULL) {
        printf("file do not exist\n");
        exit(1);
    }
    
    while (fgets(temp,sizeof(temp),bidinfo)) {
        bidelement = (bid*)malloc(sizeof(bid));
        if (bidelement == NULL) {
            printf("can not create linklist \n");
            exit(1);
        }
        bidelement->next = header;
        header = bidelement;
        if(sscanf(temp,"%s %s %s",bidelement->id,bidelement->item,bidelement->price) !=3 ) {
            printf("inputfile data format error\n");
            exit(1);
        }
    }
    fclose(bidinfo);
    
    
    if((rv = getaddrinfo(NULL,bidderudp,&hints,&servinfo)) != 0) {
        fprintf (stderr,"getaddrinfo: %s\n",gai_strerror(rv));
        return 1;
    }
    
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
            perror("socket");
            continue;
        }
        if (bind(sockfd,p->ai_addr,p->ai_addrlen) == -1) {
            close (sockfd);
            perror("bind");
            continue;
        }
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr,"failed to bind socket\n");
        return 1;
    }
    freeaddrinfo(servinfo);
    
    struct hostent *hostptr;
    // int hn;
    if((hostptr = gethostbyname(HOSTNAME)) == NULL) {
        perror("gethostbyname");
        return 1;
    }
    strcpy(s,inet_ntoa(*(struct in_addr *)hostptr->h_addr_list[0]));
    printf("Phase 3:<%s> has UDP port %s and IP address: %s\n",biddername,bidderudp,s);
    
    addr_len = sizeof their_addr;
    printf("Phase 3:Item list displayed here \n");

    while(1){
     //   numbytes = recvfrom(sockfd, buf3,MAXDATASIZE-1,0 ,(struct sockaddr *)&their_addr,&addr_len);

        numbytes = recvfrom(sockfd, buf3,MAXDATASIZE-1,0 ,(struct sockaddr *)&client,&addr_len);
     //   numbytes = recv(sockfd, buf3,MAXDATASIZE-1,0);
        if(numbytes  == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf3[numbytes] = '\0';
        if(strcmp(buf3,stop)==0) {
            break;
        }
        printf("%s",buf3);
        incomeinfo =(bid*)malloc(sizeof(bid));
        if(incomeinfo ==NULL){
            printf("out fo memory\n");
            exit(1);
        }
        incomeinfo->next = header1;
        header1 = incomeinfo;
        if(sscanf(buf3,"%s %s %s",incomeinfo->id,incomeinfo->item,incomeinfo->price) !=3) {
            printf("transferred file data format wrong\n");
            exit(1);
        }
    }
  
    int theirport;
    theirport = (int) ntohs(client.sin_port);
   // printf("ip  %s  port:%d\n",inet_ntoa(client.sin_addr),theirport);
    
    char test1[MAXDATASIZE];
    sprintf(test1,"Phase 3: Auction Server IP Address: %s UDP Port Number: %d",inet_ntoa(client.sin_addr),theirport);
    //if((numbytes=sendto(sockfd,test1,strlen(test1),0,their_addr,sizeof(struct sockaddr_storage)))==-1) {
        // if((numbytes=send(sockfd,stop,strlen(stop),0))==-1) {
    if( (numbytes = sendto(sockfd,test1,strlen(test1),0,(struct sockaddr*)&client,addr_len)) == -1 ) {
        perror("sendto");
        close(sockfd);
        exit(0);
    }
   
    /*bidelement = header;
    while(bidelement) {
        printf("%s %s %s \n",bidelement->id,bidelement->item,bidelement->price);
        bidelement = bidelement->next;
    }*/

    printf("Phase 3: <%s>(Bidding information displayed here)\n",fileid);
    incomeinfo = header1;
    char buf4[MAXDATASIZE];
    while(incomeinfo){
        bidelement = header;
        while(bidelement){
            if( (strcmp(bidelement->id,incomeinfo->id) !=0) || (strcmp(bidelement->item,incomeinfo->item)!=0) ){
                bidelement = bidelement->next;
                continue;
            }
            sprintf(buf4,"%s %s %s %s",fileid,bidelement->id,bidelement->item,bidelement->price);
            printf("%s %s %s\n",bidelement->id,bidelement->item,bidelement->price);

            if((numbytes = sendto(sockfd,buf4,strlen(buf4),0,(struct sockaddr*)&client,addr_len)) == -1) {
                perror("sendto");
                close(sockfd);
                exit(0);
            }
            bidelement = bidelement->next;
            break;
        }
       // printf("b%s %s %s %s\n",name,incomeinfo->id,incomeinfo->item,incomeinfo->price);

        incomeinfo = incomeinfo->next;
    }
    if((numbytes = sendto(sockfd,stop,strlen(stop),0,(struct sockaddr*)&client,addr_len)) == -1) {
        perror("sendto");
        close(sockfd);
        exit(0);
    }
    close(sockfd);
    
    
    
    struct sigaction sa;
    int yes = 1;
    
    memset(&hints, 0 ,sizeof(hints));
    // hints.ai_family = AF_UNSPEC;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, biddertcp, &hints, &servinfo)) != 0) {
        fprintf (stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        return 1;
    }
    freeaddrinfo(servinfo);
    
    if (listen(sockfd, BACKLOG) == -1) {
        perror ("listen");
        exit(1);
    }
    
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        if(errno = EINTR) {
       //     continue;
        }
        perror("accept");
     //   continue;
    }
    pid_t fpid = fork();
    if (fpid < 0) {
        perror("fork");
        exit(1);
    }
    if (fpid == 0) {
        //   printf("This is child process pid %d",getpid());
        close(sockfd);
        int numbytes;
        char buffer[MAXDATASIZE];
        if ((numbytes = recv(new_fd, buffer, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit (1);
        }
        buffer[numbytes] = '\0';
        char *resultbuff;
        resultbuff = strtok(buffer,"\n");
        while(resultbuff!=NULL) {
            printf("%s\n",resultbuff);
            resultbuff = strtok(NULL,"\n");
        }
        close(new_fd);
        printf("End of Phase 3 for %s.\n",fileid);
        exit(0);
    }
    close(new_fd);
    return 0;
}

int main(void) {
    int phase1,phase3;
    phase1 = authentication(logintxt,fileid);
    if (phase1 == 1) {
        printf("authentication fail\n");
        exit(1);
    }
    phase3 = Auction(bidtxt,fileid);
    if (phase3 == 1) {
        printf("Auction fail\n");
        exit(1);
    }
    exit(0);}
