#include <stdio.h>
#include <stdlib.h>	
#include <string.h>	
#include <netdb.h>     
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>	
#include <sys/wait.h>    
#include <fcntl.h> 
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SIZE 1000

typedef struct file_info
{
	char *name;
	char host[SIZE];
	int port_number;
}file_info;

file_info *file;

int def_port = 23456;
void *file_handler(void *s1);
int consult_peer(int input_port, char host[], char f_args[]);
int Get_upload_port();
int Get_input_port();
int consult_server(int input_port, char host[],int upload_port);
char my_host[SIZE];
int upload_port;
int state_type;

int main()
{
	
	int input_port;
	char host[SIZE];
	int curr_socket_desc, request;
	socklen_t len_str;
	struct sockaddr_in * curr_socket_addr;
	struct sockaddr_in client_addr;
	char *quit_singal;
	file = malloc(sizeof(file_info));

    
    upload_port = Get_upload_port();
    printf("Enter the host of this machine.\n");
    scanf("%s",my_host);
	printf("Connect the server first.\n");
	printf("Server information:\n");
	printf("Enter host IP(127.0.0.1 for local host)\n");
	scanf("%s",host);
	input_port = Get_input_port();

	int link = consult_server(input_port,host,upload_port);
	if (link == 1)
		printf("Connected to the server!\n");
	else
	{
		printf("Connection broke with the Server\n");
		return 0;
	}



	switch(state_type)
	{
		case 0://In case 0, the peer is uploading data.
		{
			curr_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
			if (curr_socket_desc < 0) {
				printf("Socket Creation failed\n");
				return 0;
			}
			curr_socket_addr = (struct sockaddr_in *) calloc (1,sizeof(struct sockaddr_in));
			curr_socket_addr->sin_family = AF_INET;   /* address family */
			curr_socket_addr->sin_port = htons(upload_port);
			curr_socket_addr->sin_addr.s_addr = htonl(INADDR_ANY);

			if (bind(curr_socket_desc, (struct sockaddr *)curr_socket_addr, sizeof(*curr_socket_addr)) < 0) 
			{
				printf("could not bind the socket");
				return (0);
			}
			if (listen(curr_socket_desc, 5) < 0) {
				printf("could not listen to connections");
				return (0);
			}
			printf("The peer is running on output_port %d\n",upload_port);
			len_str = sizeof(client_addr);                   /* length of address */
			while( (request = accept(curr_socket_desc,(struct sockaddr *)&client_addr, &len_str)) )
			{
				printf("Connection accepted\n");
				pthread_t new_thread;
				int * args = calloc(1,sizeof(int));
				*args = request;
				if( pthread_create( &new_thread, NULL ,file_handler,(void*) args) < 0)
				{
					printf("error on thread creation");
					return 0;
				}
				printf("Thread handler attached\n");
			}
			if (request < 0) 
			{
				printf("error on accept");
				return 0;
			}
			
		}

		case 1://In case 1, the peer is downloading data.
		{
			char file_name[SIZE];
			char file_host[SIZE];
			int file_port;
			printf("Upload Peer information:\n");
			printf("Please enter the file name:\n");
			scanf("%s",file_name);
			printf("Please enter the file host:\n");
			scanf("%s",file_host);
			printf("Please enter the file port number:\n");
			scanf("%d",&file_port);

			int res = consult_peer(file_port,file_host,file_name);
			if (res == 1)
			{
				printf("File transfer successful from the peer\n");
			}
			else if (res == -1 || res == 0 )
			{

				if (res == 0)
				{
					printf("Connection broke with the peer\n");
				}
				if (res == -1)
				{
					printf("File not found in server 1\n");
				}
			}
		} 
		default:
		{
			printf("The client stops working.\n");
			return 0;
		}
	}

	return 0;
}

int Get_upload_port()
{
	int upload_port;
	printf("Enter the port number between 1024 and 65535:(Enter -1 to use default port) used to upload.\n");
    scanf("%d",&upload_port);
	if (upload_port == -1)
		{
			upload_port = def_port;
		}
		else if (upload_port < 1024 || upload_port > 65535)
		{
			printf("invalid port number for the service\n");
			return Get_upload_port();
		}

	return upload_port;	
}

int Get_input_port()
{
	int input_port;
	printf("Enter the port number between 1024 and 65535:(Enter -1 to use default port).\n");
    scanf("%d",&input_port);
	if (input_port == -1)
	{
		input_port = def_port;
		return input_port;
	}
	else if (input_port < 1024 || input_port > 65535)
	{
		printf("invalid port number for the service\n");
		return Get_input_port();
	}
	else
		return input_port;	
}


int consult_peer(int input_port,char host[],char f_args[])//To connect with the peer
{
	
	struct sockaddr_in * server_addr;
	char msg_buffer1[1000]="",msg_buffer2[1000]=""; 
	struct hostent *server_info;	/* host information */
	struct sockaddr_in * client_addr;
	int len_addr,curr_socket_desc;
	curr_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (curr_socket_desc < 0) {
		printf("Socket Creation failed\n");
		return 0;
	}

	client_addr = (struct sockaddr_in *) calloc (1,sizeof(struct sockaddr_in));
	client_addr->sin_family = AF_INET;
	client_addr->sin_addr.s_addr = htonl(INADDR_ANY);
	client_addr->sin_port = htons(0);
	
	if (bind(curr_socket_desc, (struct sockaddr *)client_addr, sizeof(*client_addr)) < 0) 
	{
		printf("could not bind the socket");
		return 0;
	}

	server_addr = (struct sockaddr_in *) calloc (1,sizeof(struct sockaddr_in));
	server_addr->sin_family = AF_INET;
	server_addr->sin_port = htons(input_port);
	server_addr->sin_addr.s_addr = inet_addr(host);

	if (connect(curr_socket_desc, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) 
	{
		printf("connection failed");
		return 0;
	}
    int  n;
    char filename[1000];
    strcpy (filename,"client_files/");//Make the client_files/ at the beginning of the file:filename to indicate the file address
    strcat (filename,f_args);//strcat: add f_args to the end of filename;
    n = write(curr_socket_desc,f_args,strlen(f_args));
    if (n < 0) 
    {
         return 0;
    }
    
    n = read(curr_socket_desc,msg_buffer2,255);
    if (n < 0) 
    {
         return 0;
    }
    int i=atoi(msg_buffer2);
    if (i==1)
    {
    	printf("File found!\n");
    	char file_buffer[100000];
    	FILE *fp;
    	int fd = open(filename, O_WRONLY|O_CREAT,S_IRWXU);//open the file
    	n = read(curr_socket_desc,msg_buffer2,255);
    	if (n < 0) 
    	{
        	return 0;
        }
	   int sd;
	   for (sd=0;sd<100000;sd++)
	          file_buffer[sd]='\0';
	printf("File Size: %d\n",atoi(msg_buffer2));
	long long int len=atoi(msg_buffer2);

	printf("File Transfer Started!\n");

	int chunk_size=256;
	long long int x=len/chunk_size;
	int y=len%chunk_size;
	long long int i,prev=0;
	for (i=0;i<x;i++)
	{
		n=read(curr_socket_desc,file_buffer,chunk_size);
		 if (n < 0) 
		    {
		         return 0;
		    }

	   n=write(fd,file_buffer,chunk_size);//send the file chunk
	   prev=prev+n;
	   printf("Transferred bytes %lld\n",prev );

		n=write(curr_socket_desc,"1",1);
		 if (n < 0) 
		    {
		         return 0;
		    }	

	}
	n=read(curr_socket_desc,file_buffer,y);
		 if (n < 0) 
		    {
		         return 0;
		    }

	n=write(fd,file_buffer,y);
	prev=prev+n;
    printf("Transferred bytes %lld\n",prev );


				if (prev!=len)
					{
						printf("Incomplete File Transfer,Problem with the Network:\n");

					}
					else
					{
						printf("File Transfer Successful!\n");
					}

	return 1;
}
else
 {
    	return -1;
  }
    return 0;

}

void *file_handler(void *s1)//Deal with file request from other peer
{

   int request = *(int*)s1;
   char buf[1000]="";
   int n;
   n = read(  request ,buf,255 );//read the request file name.
  if (n < 0)
  {
        printf("error on read");
        return 0;
  }
  printf("Requested file name: %s\n",buf);
	FILE *f1;
	char filename[1000];
	strcpy (filename,"client_files/");
	strcat (filename,buf);
	if (n < 0)
	 {
	       printf("error on write");
           return 0;
	 }
	char file_buffer[1000];
  char msg_buffer2[1024];
  int fd,sd,addrlen;                    
  struct stat stat_buf;     
  off_t offset = 0;          
    fd = open(filename, O_RDONLY);
    if (fd == -1) 
    {
      
      printf("unable to open '%s': %s\n", filename, strerror(errno));
      n = write( request ,"0",1);
      if (n < 0)
      {
           printf("error on write");
             return 0;
      }
      return 0;
    }

    printf("File found!\n");
    n = write( request ,"1",1);
    fstat(fd, &stat_buf);
    printf(" %9jd", (intmax_t)stat_buf.st_size);
    char buffer[100000];
    for (sd=0;sd<100000;sd++)
          buffer[sd]='\0';
    sprintf(buffer, " %9jd", (intmax_t)stat_buf.st_size );
     n = write( request ,buffer,strlen(buffer));    // write the length of the file
     printf("File transfer starts \n");
     FILE * fpIn = fopen(filename, "r");
    if (fpIn)
    {
            char buf[256];
	   char buf1[2];
            while(1)
            {
               ssize_t bytesRead = fread(buf, 1, sizeof(buf), fpIn);
               if (bytesRead <= 0) break;  // EOF

               if (send(request, buf, bytesRead, 0) != bytesRead)
               {
                  printf("Error,sending ");
                  break;
               }
		n = read( request,buf1,1);
              if (n < 0)
              {
                     printf("error on write");
                     return 0;
              }
			

            }
         }
         else {
          printf("Error, couldn't open file [%s] to send!\n", filename);
         }
          
      close(fd);
      printf("the file transfer was successful\n");
      free(s1);

}


int consult_server(int input_port, char *host, int upload_port)
{

	char sendbuf[SIZE];
	char recvbuf[SIZE];
    int n;
int type;
	struct sockaddr_in * server_addr;
	struct hostent *server_info;	/* host information */
	struct sockaddr_in * client_addr;
	int len_addr,curr_socket_desc;
	curr_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (curr_socket_desc < 0) {
		printf("Socket Creation failed\n");
		return 0;
	}

	client_addr = (struct sockaddr_in *) calloc (1,sizeof(struct sockaddr_in));
	client_addr->sin_family = AF_INET;
	client_addr->sin_addr.s_addr = htonl(INADDR_ANY);
	client_addr->sin_port = htons(0);
	
	if (bind(curr_socket_desc, (struct sockaddr *)client_addr, sizeof(*client_addr)) < 0) 
	{
		printf("could not bind the socket");
		return 0;
	}

	server_addr = (struct sockaddr_in *) calloc (1,sizeof(struct sockaddr_in));
	server_addr->sin_family = AF_INET;
	server_addr->sin_port = htons(input_port);
	server_addr->sin_addr.s_addr = inet_addr(host);


if(connect(curr_socket_desc, (struct sockaddr *)server_addr, sizeof(*server_addr))<0)
{
printf("Connection failed\n");
return 0;
}

printf("Connection success!\n");
while(1)
{
	
	printf("Input: \n");//set up connection
	scanf("%s", sendbuf);
	if(n = (write(curr_socket_desc,sendbuf, SIZE)))
	{
		printf("Send Success;\n");
	}


if(!(strcmp(sendbuf,"Quit")))//Quit instruction
{
read(curr_socket_desc,recvbuf,SIZE);
if(!(strcmp(recvbuf,"Quit")))
{
	state_type = 2;
	break;
}
}



if(!(strcmp(sendbuf,"Port")))//To send the port number
{
read(curr_socket_desc, recvbuf,SIZE);
if(!(strcmp(recvbuf,"OK")))
{
sprintf(sendbuf,"%d",upload_port);
write(curr_socket_desc,sendbuf,SIZE);
continue;
}
}



if(!(strcmp(sendbuf,"Host")))//To send the host address
{
read(curr_socket_desc, recvbuf,SIZE);
if(!(strcmp(recvbuf,"OK")))
{
write(curr_socket_desc,my_host,SIZE);
continue;
}
}


	if(!(strcmp(sendbuf,"Add")))//To add file to the server list
	{
		printf("Input(filename):\n");
		scanf("%s",sendbuf);
write(curr_socket_desc, sendbuf, 1000);
continue;
}


	if(!(strcmp(sendbuf,"List")))//To ask the server to list all the file names
	{
		read(curr_socket_desc,recvbuf,SIZE);
		printf("Receive List: \n%s",recvbuf);
	}



	if(!(strcmp(sendbuf,"Download")))//To get the download information from the server
			{
				file_info *file = malloc(sizeof(file_info));
				read(curr_socket_desc,recvbuf,SIZE);
				if(!(strcmp(recvbuf,"OK")))
				{
					printf("The server is permitted.\n");
					printf("Input:Filename to download.\n");
					scanf("%s", sendbuf);
					n = write(curr_socket_desc,sendbuf, SIZE);
					if(n<0)
						{
							printf("Fail to receive the respond!\n");
							return 0;
							break;
						}
						file->name = sendbuf;
						n = read(curr_socket_desc, recvbuf, SIZE);
						if(n<0)
							{
								printf("Fail to receive the respond!\n");
								return 0;
								break;
							}
							if(!(strcmp(recvbuf, "No such file.")))
								{
									printf("There is no such file.\n");
									continue;
								}
								strcpy(file->host,recvbuf);
								n = write(curr_socket_desc,"Host OK",SIZE);
								if(n<0)
									{
										printf("Fail to receive the respond!\n");
										return 0;
										break;
									}
									read(curr_socket_desc,recvbuf,SIZE);
									file->port_number = atoi(recvbuf);
									printf("%s\t%s\t%d\n",file->name,file->host,file->port_number);
									state_type = 1;
									break;
				}
			}
			
	if(!(strcmp(sendbuf,"Upload")))//To show that this peer is going to upload;
	{
		read(curr_socket_desc,recvbuf,SIZE);
		if(!(strcmp(recvbuf,"OK")))
		{
			printf("It's uploading.\n");
			state_type = 0;
			break;
		}

	}
}

    

	return 1;
}
