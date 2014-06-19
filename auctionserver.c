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

#define TCPPORT1 "1128"
#define TCPPORT2 "1228"
#define HOSTNAME "nunki.usc.edu"
#define S1TCPPORT "2128"
#define S2TCPPORT "2228"
#define B1TCPPORT "4128"
#define B1UDPPORT "3128"
#define B2TCPPORT "4228"
#define B2UDPPORT "3228"
#define stop      "term"

#define num_con 4
#define BACKLOG 10
#define MAXDATASIZE 256

typedef struct list_el{
    char username[30];
    char password[30];
    char account[30];
    char id[30];
    char type[30];
    char ip[50];
    char tcpport[20];
    char udpport[20];
    struct list_el *next;
}list_el,list;

typedef struct itemlist{
    char owner[20];
    char item[20];
    char price[20];
    char buyer[20];
    char bidprice[20];
    struct itemlist *next;
}itemlist,itemline;

//this is from beej's guide
void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) >0);
}

int storefile(char *input,char *name) {
    FILE *output = 0;
    char tem[MAXDATASIZE];
    int length;

    output = fopen(name,"a+");
    length = ftell(output);
    // while(fgets(tem,sizeof(tem),input)) {
    fseek(output,length,SEEK_SET);
    fprintf(output,"%s",input);
    length = ftell(output);
    //  }
    fclose(output);
    return 0;
}


void *get_in_addr(struct sockaddr *sa)
{
    //   if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
    //   }
    //   return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int authentication()
{
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    char temp[100];
    FILE *infile;
    list  *curr, *header;
    header = NULL;
    int numbytes;
    
//    if(argc != 2){
//        fprintf(stderr,"usaga: missing input file \n");
//        exit(1);
//    }
    
    infile = fopen("Registration.txt","r");
    if(infile==NULL)
    {
        printf("%s %s","Registration.txt","do not exist\n");
        exit(1);
    }
    //parse register file
    while(fgets(temp,sizeof(temp),infile)) {
        curr = (list*)malloc(sizeof(list));
        char check[MAXDATASIZE];
        if (curr ==NULL) {
            printf("not enough space to creat linklist \n");
            exit(1);
        }
       
        if(sscanf(temp,"%s %s %s",curr->username,curr->password,curr->account) != 3) {
            printf("inputfile data format error \n");
            exit (1);
        }
        sscanf(curr->account,"%4s",check);
        if(strcmp(check,"4519")!=0){
            continue;
        }
        curr->next = header;
        header = curr;

    }
    fclose(infile);
    
    /*curr = header;
    while (curr) {
        printf("%s %s %s \n",curr->username,curr->password,curr->account);
        curr = curr ->next;
    }*/
    
    /*
	 * L144 to L184 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/* create socket */

    memset(&hints, 0 ,sizeof(hints));
    // hints.ai_family = AF_UNSPEC;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    if ((rv = getaddrinfo(NULL, TCPPORT1, &hints, &servinfo)) != 0) {
        fprintf (stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    //   inet_ntop(servinfo->ai_family, get_in_addr((struct sockaddr *)servinfo->ai_addr),s ,sizeof s);
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
//    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s ,sizeof s);
    
    freeaddrinfo(servinfo);
    
    if (listen(sockfd, BACKLOG) == -1) {
        perror ("listen");
        exit(1);
    }
    
    //gethostname
    struct hostent *hostptr;
    // int hn;
    if((hostptr = gethostbyname(HOSTNAME)) == NULL) {
        //      printf("cannot get hostname \n");
        perror("gethostbyname");
        return 1;
    }
    
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    strcpy(s,inet_ntoa(*(struct in_addr *)hostptr->h_addr_list[0]));
    printf("Phase 1:Auction server has TCP port number %s and IP address %s \n",TCPPORT1,s );
    sin_size = sizeof their_addr;
    
    int numcon;
    for(numcon = 0;numcon<num_con;){
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        // printf("%d\n",new_fd);
        if (new_fd == -1) {
            if(errno = EINTR) {
                continue;
            }
            perror("accept");
            continue;
        }
        
        //creat child process
        pid_t fpid = fork();
        if (fpid < 0) {
            perror("fork");
            exit(1);
        }
        
        // this is a child process
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
            char cmd[MAXDATASIZE];
            //   char token[MAXDATASIZE];
            char type[MAXDATASIZE];
            char username[MAXDATASIZE];
            char password[MAXDATASIZE];
            char account[MAXDATASIZE];
            char id[MAXDATASIZE];
            char reply1[MAXDATASIZE] = "Accepted#";
            char reply2[MAXDATASIZE] = "Accepted#";
            char reply3[MAXDATASIZE] = "Rejected#";
            strcat(reply2," ");
            strcat(reply2,TCPPORT2);
            strcat(reply2," ");
            strcat(reply2,s);
            if(sscanf(buffer,"%*[<]%[^>]",id) != 1) {
                printf("missing identify information \n");
                return 1;
            }

            if(sscanf(buffer,"%*[^>]>%[^#]",cmd) != 1) {
                printf("missing account information \n");
                return 1;
            }
            
            if(sscanf(buffer,"%*[^#]#%*[^#]#%s %s %s %s",type, username , password, account) != 4) {
                printf("missing account information \n");
                return 1;
            }
           //    printf("input test %s %s %s %s %s %s \n",id,cmd,type, username , password, account);
            
            if(strcmp("Login",cmd)!=0) {
                printf("wrong command \n");
                return 1;
            }
            
            char e[MAXDATASIZE];
            inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr),e ,sizeof(e));
            
            curr = header;
            while (curr) {
                if((strcmp(username,curr->username)!=0) || (strcmp(password,curr->password)!=0) || (strcmp(account,curr->account)!=0)) {
                    curr = curr ->next;
                    continue;
                }
                // printf ("match \n");
                //  exit(1);
                break;
            }
            if (curr == NULL){
                //      printf("miss match \n");
                if (send(new_fd,reply3,strlen(reply3),0) == -1) {
                    perror("send");
                    close(new_fd);
                    exit(0);
                }
                
                printf("Phase 1:Authentication request. %s: Username %s Password: %s Bank Account: %s UserIP Addr:%s .Authorized: Rejected#\n",id,username,password,account,e);
                close(new_fd);
            }
            
            else {
                char archive[MAXDATASIZE];
                strcpy(archive,id);
                strcat(archive," ");
                strcat(archive,type);
                strcat(archive," ");
                strcat(archive,username);
                strcat(archive," ");
                strcat(archive,e);
                strcat(archive," ");

                if(strcmp(id,"Seller#1")==0)
                {
                    strcat(archive,S1TCPPORT);
                    strcat(archive," ");
                    strcat(archive,"0");

                }

                if(strcmp(id,"Seller#2")==0) {
                    strcat(archive,S2TCPPORT);
                    strcat(archive," ");
                    strcat(archive,"0");
                }
                
                if(strcmp(id,"Bidder#1")==0) {
                    strcat(archive,B1TCPPORT);
                    strcat(archive," ");
                    strcat(archive,B1UDPPORT);
                }
                if(strcmp(id,"Bidder#2")==0) {
                    strcat(archive,B2TCPPORT);
                    strcat(archive," ");
                    strcat(archive,B2UDPPORT);
                }
                strcat(archive,"\n");
                storefile(archive,"archive.txt");

                               // printf("%s",e);
                if(strcmp(type,"2")!=0) {
                    if (send(new_fd,reply1,strlen(reply1),0) == -1) {
                        perror("send");
                        exit(1);
                    }
                    printf("Phase 1:Authentication request. %s: Username %s Password: %s Bank Account: %s UserIP Addr: %s .Authorized: Accepted#\n",id,username,password,account,e);
                }
                else {
                    if (send(new_fd,reply2,strlen(reply2),0) == -1) {
                        perror("send");
                        exit(1);
                    }
                    printf("Phase 1:Authentication request. %s: Username %s Password: %s Bank Account: %s UserIP Addr: %s .Authorized: Accepted#\n",id,username,password,account,e);
                    printf("Phase 1:Auction Server IP address: %s  PreAuction Port Number: %s sent to <%s> \n",e,TCPPORT2,id);
                }
                close(new_fd);
            }
            exit(5);
        }
        close(new_fd);
        numcon++;
    }
    
   int status1, status2, status3,status4;
	wait(&status1);
	wait(&status2);
	wait(&status3);
    wait(&status4);
    close(sockfd);
    printf("End of phase1 for Auction Server \n");
    
    return 0;
}


int PreAuction()
{
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    char temp[100];
    //  int numbytes;
    
    /*
	 * L388 to L433 are from the given tutorial http://beej.us/guide/bgnet/.
	 */
	/* create socket */

    memset(&hints, 0 ,sizeof(hints));
    // hints.ai_family = AF_UNSPEC;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    
    if ((rv = getaddrinfo(NULL, TCPPORT2, &hints, &servinfo)) != 0) {
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
    
    struct hostent *hostptr;
    if((hostptr = gethostbyname(HOSTNAME)) == NULL) {
        perror("gethostbyname");
        return 1;
    }
    
    printf("Phase 2:Auction Server IP Address %s PreAuction TCP Port Number: %s \n", inet_ntoa(*(struct in_addr *)hostptr->h_addr_list[0]),TCPPORT2);
    
    sin_size = sizeof their_addr;
    
    int numsell;
    for(numsell = 0;numsell<2;){
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            if(errno = EINTR) {
                continue;
            }
            perror("accept");
            continue;
        }
        
        //creat child process
        pid_t fpid = fork();
        if (fpid < 0) {
            perror("fork");
            exit(1);
        }
        
        // this is a child process
        if (fpid == 0) {
            close(sockfd);
            int numbytes;
            char buffer2[MAXDATASIZE];
            char *name;
            if ((numbytes = recv(new_fd, buffer2, MAXDATASIZE-1, 0)) == -1) {
                perror("recv");
                exit (1);
            }
            buffer2[numbytes] = '\0';
            if(strcmp(buffer2,stop)==0){
                close(new_fd);
                exit(2);
            }
         //   printf("buffer2 %s \n",buffer2);
         //   if(sscanf(buffer2,"%*[^<]<%[^>]",)) {
         //       printf("missing account information \n");
         //       return 1;
         //   }
         //   storefile(buffer2,"broad.txt");
            char sendername[MAXDATASIZE];
            char sendid[MAXDATASIZE];
            name = strtok(buffer2,"\n");
            sscanf(buffer2,"%s %s",sendername,sendid);
            printf("Phase 2 :<%s> send item list\n",sendername);
            printf("Phase 2 :(Received item display here)\n");

           // name = strtok(buffer2,"\n");
            //printf("1%s\n",name);
          //  char sendername[MAXDATASIZE];
           // strcpy(sendername,name);
            //printf("%s\n",sendername);
            while(name!=NULL) {
                name = strtok(NULL,"\n");
                char outputline[MAXDATASIZE];
                char templine[MAXDATASIZE];
                strcpy(outputline,sendid);
                //strcpy(templine,name);
                strcat(outputline," ");
                strcat(outputline,name);
                strcat(outputline,"\n");
              //  strcat(outputline,templine);
               // strcat(outputline,"\n");
                printf("%s",outputline);
                storefile(outputline,"broadcastlist.txt");
            }
            close(new_fd);
            //    fclose(fp);
            
            exit(1);
        }
        close(new_fd);
        numsell++;
    }
    int status5, status6;
    wait(&status5);
    wait(&status6);
    // wait(&status3);
    // wait(&status4);
    close(sockfd);
    printf("End of Phase 2 for Auction Server \n");
    return 0;
}


int Auction() {
    int sockfd,sockfd2;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf3[MAXDATASIZE];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    struct sockaddr_in sin;
    socklen_t sin_len;
    list *data,*ptr;
    ptr = NULL;
    char feedback[MAXDATASIZE];
    
    itemline *element,*header;
    header = NULL;
    FILE *readfile;
    char temp3[MAXDATASIZE];
    char temp4[MAXDATASIZE];
    struct sigaction sa;
    int yes = 1;
    int flag=0;
    
    FILE *checklist;
    
    readfile = fopen("broadcastlist.txt","r");
    
    if(readfile==NULL)
    {
        printf("%s %s","broadcastlist.txt","do not exist\n");
        exit(1);
    }
  
    //parse broadcast file
    while(fgets(temp3,sizeof(temp3),readfile)) {
        element = (itemline*)malloc(sizeof(itemline));
        if (element ==NULL) {
            printf("not enough space to creat linklist \n");
            exit(1);
        }
        element->next = header;
        header = element;
        if(sscanf(temp3,"%s %s %s",element->owner,element->item,element->price) != 3) {
            printf("inputfile data format error \n");
            exit (1);
        }
        strcpy(element->buyer,"0");
        strcpy(element->bidprice,element->price);

    }
    fclose(readfile);
    
    checklist = fopen("archive.txt","r");
    if(checklist==NULL)
    {
        printf("%s %s","archive.txt","do not exist\n");
        exit(1);
    }
    //parse archive file
    while(fgets(temp4,sizeof(temp4),checklist)) {
        data = (list*)malloc(sizeof(list));
        if (data ==NULL) {
            printf("not enough space to creat linklist \n");
            exit(1);
        }
        data->next = ptr;
        ptr = data;
        if(sscanf(temp4,"%s %s %s %s %s %s",data->id,data->type,data->username,data->ip,data->tcpport,data->udpport) != 6) {
            printf("inputfile data format error \n");
            exit (1);
        }
    }
    fclose(checklist);
    
  /*  data = ptr;
    while (data) {
        printf("%s %s %s %s %s %s \n",data->id,data->type,data->username,data->ip,data->tcpport,data->udpport);
        data = data->next;
    }*/

    
  //  hints.ai_flags = AI_PASSIVE;
    data =ptr;
    while (data) {
        itemline *incomebid,*header1;
        header1 = NULL;
        if (strcmp(data->type,"1")!=0) {
            data = data->next;
            continue;
        }
      //  printf("a%s\n",data->udpport);
        char sendbuf[MAXDATASIZE];
        memset(&hints,0,sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        /*
         * L631 to L657 are from the given tutorial http://beej.us/guide/bgnet/.
         */
        /* create socket */


        if((rv = getaddrinfo(data->ip,data->udpport,&hints,&servinfo)) !=0) {
            fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(rv));
            return 1;
        }
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1) {
                perror("socket");
                continue;
            }
         /* if (bind(sockfd,p->ai_addr,p->ai_addrlen) == -1) {
                close (sockfd);
                perror("bind");
                continue;
            }
            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                perror("client: connect");
                continue;
            } */
            break;
        }
        if(p ==NULL) {
            fprintf(stderr,"failer to  socket\n");
            return 1;
        }
        
        freeaddrinfo(servinfo);
        
        element = header;
        while (element) {
          //  printf("%s %s %s \n",element->owner,element->item,element->price);
            strcpy(sendbuf,element->owner);
            strcat(sendbuf," ");
            strcat(sendbuf,element->item);
            strcat(sendbuf," ");
            strcat(sendbuf,element->price);
            strcat(sendbuf,"\n");
           // printf("%s \n",sendbuf);
            if((numbytes=sendto(sockfd,sendbuf,strlen(sendbuf),0,p->ai_addr,p->ai_addrlen))==-1) {
         //   if((numbytes=send(sockfd,sendbuf,strlen(sendbuf),0))==-1){
                perror("send");
                close(sockfd);
                exit(0);
            }
            
            element = element->next;
        }
        
        if((numbytes=sendto(sockfd,stop,strlen(stop),0,p->ai_addr,p->ai_addrlen))==-1) {
       // if((numbytes=send(sockfd,stop,strlen(stop),0))==-1) {
            perror("sendto");
            close(sockfd);
            exit(0);
        }
        
        char buf3[MAXDATASIZE];
        numbytes = recvfrom(sockfd, buf3,MAXDATASIZE-1,0 ,NULL,NULL);
        //   numbytes = recv(sockfd, buf3,MAXDATASIZE-1,0);
        if(numbytes  == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf3[numbytes] = '\0';
        printf("%s \n",buf3);
        printf("Phase 3: Item list displayed here\n");
        element = header;
        while (element) {
            printf("%s %s %s \n",element->owner,element->item,element->price);
            element = element->next;
        }
        
        while(1){
            //   numbytes = recvfrom(sockfd, buf3,MAXDATASIZE-1,0 ,(struct sockaddr *)&their_addr,&addr_len);
            
            numbytes = recvfrom(sockfd, buf3,MAXDATASIZE-1,0 ,NULL,NULL);
            //   numbytes = recv(sockfd, buf3,MAXDATASIZE-1,0);
            if(numbytes  == -1) {
                perror("recvfrom");
                exit(1);
            }
            buf3[numbytes] = '\0';
            if(strcmp(buf3,stop)==0) {
                break;
            }
           
            //printf("%s\n",buf3);
            incomebid = (itemline*)malloc(sizeof(itemline));
            if (incomebid == NULL) {
                printf("can not create linklist \n");
                exit (1);
            }
            incomebid->next = header1;
            header1 = incomebid;

            if(sscanf(buf3,"%s %s %s %s",incomebid->buyer,incomebid->owner,incomebid->item,incomebid->price) !=4) {
                printf("transferred file data format wrong\n");
                exit(1);
            }
            
            element = header;
            while(element) {
                if( strcmp(incomebid->owner,element->owner) || strcmp(incomebid->item,element->item) ){
                    element = element->next;
                    continue;
                }
                if( atoi(incomebid->price) > atoi(element->bidprice) ) {
                    strcpy(element->bidprice,incomebid->price);
                    strcpy(element->buyer,incomebid->buyer);
                }
                element = element->next;
            }
            
        }
        
       /* int getsock_check = getsockname(sockfd, (struct sockaddr *)&sin, &sin_len);
        if (getsock_check == -1) {
            perror("getsockname");
            exit(1);
        }
        printf("Phase 3: Auction Server IP Address: %s UDP Port Number: %d\n",inet_ntoa(sin.sin_addr),(int) ntohs(sin.sin_port));*/
        printf("Phase 3: Auction Server received a bidding from <%s>\n",header1->buyer);
        printf("Phase 3: Bidding information displayed here\n");
        incomebid = header1;
        while(incomebid) {
            printf("%s %s %s %s\n",incomebid->buyer,incomebid->owner,incomebid->item,incomebid->price);
            incomebid = incomebid->next;
        }

        close(sockfd);
        data = data->next;
    }
    
    
    data = ptr;
    while(data){
        char feedback[MAXDATASIZE];
        int count = 0;
        memset (&hints, 0 ,sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        sa.sa_handler = sigchld_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror("sigaction");
            exit(1);
        }
        if ((rv = getaddrinfo(data->ip,data->tcpport , &hints, &servinfo)) != 0) {
            fprintf (stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        }
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                perror("client: socket");
                continue;
            }
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                           sizeof(int)) == -1) {
                perror("setsockopt");
                exit(1);
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
        freeaddrinfo(servinfo);
        element = header;
        while(element) {
            if(strcmp(element->buyer,"0")!=0){
                if((strcmp(data->username,element->owner)==0)||(strcmp(data->id,element->buyer)==0)) {
                    count++;
                    sprintf(feedback,"Phase 3: Item %s %s was sold at price %s\n",element->owner,element->item,element->bidprice);
                    if ((numbytes=send(sockfd, feedback,strlen(feedback),0) )== -1) {
                        perror("send");
                        close(sockfd);
                        exit(0);
                    }
                }
            }
            element = element->next;
        }
        if(count==0) {
            char bidfail[MAXDATASIZE]="nothing have being sold\n";
            if ((numbytes=send(sockfd, bidfail,strlen(bidfail),0) )== -1) {
                perror("send");
                close(sockfd);
                exit(0);
            }
        }
        close(sockfd);
        data = data->next;
    }

    element = header;
    while(element){
        //printf("%s %s %s %s \n",element->owner, element->item,element->price,element->buyer);
        if(strcmp(element->buyer,"0")!=0) {
            printf("Phase 3: Item %s %s was sold at price %s\n",element->owner,element->item,element->bidprice);
        }
        element = element->next;
    }

    printf("End of Phase 3 for Auction Server.\n");
    return 0;
}



int main(void) {
    int phase1,phase2,phase3;
    phase1 = authentication();
    if (phase1 == 1) {
        printf("authentication fail\n");
        exit(1);
    }
    phase2 = PreAuction();
    if (phase2 == 1) {
        printf("PreAuction fail\n");
        exit(1);
    }
    sleep(10);
    phase3 = Auction();
    if (phase3 == 1) {
        printf("Auction fail\n");
        exit(1);
    }

    exit(1);
}








