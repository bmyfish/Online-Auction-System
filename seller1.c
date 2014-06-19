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
#define ServerPORT2 "1228"
#define HOSTNAME "nunki.usc.edu"
#define SELLERPORT "2128"
#define BACKLOG 10
#define MAXDATASIZE 256
#define fileid "Seller#1"
#define itemtxt "itemList1.txt"
#define logintxt "sellerPass1.txt"
#define stop "term"


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) >0);
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
	 * L67to L92 are from the given tutorial http://beej.us/guide/bgnet/.
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
    //printf("%s\n",cmd);
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
    printf("Phase 1: Login request reply: %s \n", reply);
    close(sockfd);
    if(strcmp("Accepted#",reply)==0)
    {
        printf("Phase 1: Auction Server has IP Address %s and PreAuction TCP Port Number %s \n", ip,port);
        printf("End of Phase 1 for <%s>.\n",filename);
        return 0;
    }
    else {
        printf("End of Phase 1 for <%s>.\n",filename);
        return 1;
    }
}

int PreAuction(char *sendfile,char *sendname,int flag)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in sin;
    socklen_t sin_len;
    char buffer[MAXDATASIZE];
    int rv;
    
    FILE *infile;
    
    /*
	 * L182 to L207 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/* create socket */
    memset (&hints, 0 ,sizeof hints);
    //hints.ai_family = AF_UNSPEC;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((rv = getaddrinfo(HOSTNAME, ServerPORT2, &hints, &servinfo)) != 0) {
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
    if(flag==1){
        if( send(sockfd,stop,strlen(stop),0) ==-1) {
            perror("send");
            close(sockfd);
            exit(0);
        }
        close(sockfd);
        return 1;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s ,sizeof s);
    printf("Phase 2:Auction server IP Address: %s and PreAuction Port Number %s\n", s, ServerPORT2);
    
    freeaddrinfo(servinfo);
    //read file
    
    char temp[MAXDATASIZE];
    infile = fopen (sendfile,"r");
    if(infile==NULL) {
        printf("%s\n","file do not exit\n");
        exit(0);
    }
    printf("Phase 2: <%s> send item lists.\n",sendname);
    
    char sender[MAXDATASIZE];
    strcpy(sender,sendname);
    strcat(sender," ");
    if( send(sockfd,sender,strlen(sender),0) ==-1) {
        perror("send");
        close(sockfd);
        exit(0);
    }
    while(fgets(temp,sizeof(temp),infile)) {
      //  char sendbuff[MAXDATASIZE];
     //   strcpy(sendbuff,sendname);
     //   strcat(sendbuff,"<");
      //  strcat(sendbuff,temp);
      //  strcat(sendbuff,">");
        if( send(sockfd,temp,strlen(temp),0) ==-1) {
            perror("send");
            close(sockfd);
            exit(0);
        }
        printf("%s",temp);
    }
    close(sockfd);
    printf("End of Phase 2 for <%s>\n",sendname);
    return 0;
}

int Auction(char* sendname) {
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    char temp[100];
    int numbytes;

    memset(&hints, 0 ,sizeof(hints));
    // hints.ai_family = AF_UNSPEC;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, SELLERPORT, &hints, &servinfo)) != 0) {
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
        printf("End of Phase 3 for <%s>.\n",sendname);
        exit(1);
    }
    close(new_fd);
    return 0;
}




int main(void) {
    int phase1,phase2,phase3;
    phase1 = authentication(logintxt,fileid);
    if (phase1 == 1) {
        printf("authentication fail\n");
        sleep(10);
        phase2 = PreAuction(itemtxt,fileid,phase1);
        exit(1);
    }
    sleep(10);
    phase2 = PreAuction(itemtxt,fileid,phase1);
    if (phase2 == 1) {
        printf("PreAuction fail\n");
        exit(1);
    }
    phase3 = Auction(fileid);
    if (phase3 == 1) {
        printf("Auction fail\n");
        exit(1);
    }
    exit(0);
}

