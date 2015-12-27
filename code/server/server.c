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

typedef struct file_info//Store the file information
{
	char *name;//store the file's name;
	char *host;//store the file's host address
	int port_number;//store the file's host port number;
}file_info;

typedef struct file_node
{
    file_info *file;
    struct file_node *next;
}file_node;

typedef struct file_queue//Store the file's list;
{
    file_node *head;
    file_node *tail;
    int size;
}file_queue;


int def_port = 23456;
file_queue *file_list;
file_info *file;
char file_name_list[SIZE];

int Get_input_port();
void file_enqueue(file_info *f, file_queue *q);//enqueue file to the back of the q;
file_queue *Delete(file_queue *q, char *host);//delete the file information while the peer quit.


int main()
{
	file_list = malloc(sizeof(file_queue));
    file = malloc(sizeof(file_info));   
	int input_port;
	int curr_socket_desc, request;
	socklen_t len_str;
	struct sockaddr_in * curr_socket_addr;
	struct sockaddr_in client_addr;
	

	input_port = Get_input_port();

	curr_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (curr_socket_desc < 0) 
	{
		printf("Socket Creation failed\n");
		return 0;
	}

	curr_socket_addr = (struct sockaddr_in *) calloc (1,sizeof(struct sockaddr_in));
	curr_socket_addr->sin_family = AF_INET;   /* address family */
	curr_socket_addr->sin_port = htons(input_port);
	curr_socket_addr->sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(curr_socket_desc, (struct sockaddr *)curr_socket_addr, sizeof(*curr_socket_addr)) < 0) 
	{
		printf("could not bind the socket");
		return (0);
	}
	if (listen(curr_socket_desc, 10) < 0) //The Server can listen to 10 peer at the same time
	{
		printf("could not listen to connections");
		return (0);
	}

	printf("The peer is running on output_port %d\n",input_port);
	len_str = sizeof(client_addr);  /* length of address */




    int peer_upload_port;//To store the upload port number of the peer.
    char peer_host[SIZE];//To store the IP address of the peer.
    char sendbuf[SIZE], recvbuf[SIZE];//The receive and send buffer, no longer than 1000bits.
    int n;
    int state; //Use to find the file in the file_list;
    int quit = 0;

    while( (request = accept(curr_socket_desc,(struct sockaddr *)&client_addr, &len_str)))
    {
    	printf("Connection success!\n");


    	while(request)
    		{
    			read(request,recvbuf,SIZE);
    			printf("Receive: %s\n",recvbuf);



    			if(!(strcmp(recvbuf,"Quit")))//The peer prepare to quit from the link.
    				{
    					printf("Ask for quit!\n");
    					//file_list = Delete(file_list, peer_host);//This cannot be realized.
    					printf("Delete the file information from the list.\n");
    					write(request,"Quit",SIZE);
    					break;
    				}


    			if(!(strcmp(recvbuf,"Port")))//To receive the port number of the peer
    				{
    					write(request, "OK",SIZE);
    					n = read(request,recvbuf,SIZE);
    					if(n<0)
    						{
    							printf("Fail to read the port number!\n");
    							return 0;
    							break;
    						}
    					peer_upload_port = atoi(recvbuf);
    					printf("Receive Port!:%s\n",recvbuf);
    					continue;
    				}



    			if(!(strcmp(recvbuf,"Host")))//To receive the host address of the peer.
    				{
    					write(request, "OK",SIZE);
    					n = read(request,recvbuf,SIZE);
    					if(n<0)
    						{
    							printf("Fail to read the host address!.\n");
    							return 0;
    							break;
    						}
    					strcpy(peer_host,recvbuf);
    					printf("Receive Host!:%s\n",peer_host);
    					continue;
    				}




    			if(!(strcmp(recvbuf,"List")))//To send the list of the file;
    			{
    				write(request, file_name_list,SIZE);
    				printf("Sending List!\n");
    			}




    			if(!(strcmp(recvbuf,"Add")))//Add new file information to the server.
    				{
    					printf("Add new file to the list.\n");
    					read(request,recvbuf,SIZE);
    					printf("file name: %s\n",recvbuf);
    					file_info *tempfile = malloc(sizeof(file_info));
    					tempfile->name = recvbuf;
    					printf("name: %s\n",tempfile->name);
    					tempfile->port_number = peer_upload_port;
    					printf("portnumber: %d\n",tempfile->port_number);
    					tempfile->host = peer_host;
    					printf("host: %s\n",tempfile->host);
    					file_enqueue(tempfile,file_list);
    					strcat(file_name_list,tempfile->name);
    					strcat(file_name_list,"\n");
    				}


    			if(!(strcmp(recvbuf,"Download")))//To send the file information of requested file.
    				{
    					write(request,"OK",SIZE);//To inform the peer that the service has agreed the file download request.
    					n = read(request,recvbuf,SIZE);
    					if(n<0)
    						{
    							printf("Fail to receive the file name!\n");
    							return 0;
    							break;
    						}
    					printf("You asked for file: %s\n",recvbuf);
    					file_info *tempfile = malloc(sizeof(file_info));
    					file_node *node = malloc(sizeof(file_node));
    					tempfile->name = recvbuf;
    					node = file_list->head;
    					state = 0;
    					while(node != NULL)
    						{
    							if(tempfile->name == node->file->name)
    								{
    									state = 1;//Find the file!
    									break;
    								}
    							else
    								{
    									node = node->next;
    									continue;//Cannot find the file!
    								}
    						}
    					if(state == 0)
    						{
    							write(request, "No such file.",SIZE);
    							continue;
    						}
    					if(state == 1)
    						{
    							printf("%s\n",node->file->host);
    					        write(request, node->file->host,SIZE);//send the host address of the peer that has the file
    					        n = read(request,recvbuf,SIZE);
    					        if(n<0)
    					        {
    							    printf("Fail to get the respond!\n");
    							    return 0;
    							    break;
    						    }
    					        if(!(strcmp(recvbuf,"Host OK")))
    						    {
    							    sprintf(sendbuf,"%d",node->file->port_number);//send the port number of the peer that has the file.
    							    write(request,sendbuf ,SIZE);
    							    printf("This peer is downloading.\n");
                                    break;
    						    }
    						}
    				}


    				if(!(strcmp(recvbuf,"Upload")))//The peer is goint to upload.
    					{
    						write(request, "OK", SIZE);
    						printf("This peer is uploading.\n");
                            break;
    					}


    		}
    	}
			if (request < 0) 
			{
				printf("error on accept");
				return 0;
			}
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

void file_enqueue(file_info *f, file_queue *q)//enqueue file to the back of the q;
{
    file_node *tempnode = malloc(sizeof(file_node));//arrange space for tempnode;
    tempnode->file = f;
    tempnode->next = NULL;//initial tempnode;

    if(q->size == 0) 
    {
        q->size++;
        q->head = tempnode;
        q->tail = tempnode;
       
    }
    else
    {
        q->size++;
        q->tail->next = tempnode;
        q->tail = tempnode;
    }
}

file_queue *Delete(file_queue *q, char *host)//delete the file information while the peer quit.
{
	file_node *tempnode1 = malloc(sizeof(file_node));
	file_node *tempnode2 = malloc(sizeof(file_node));
	tempnode1 = q->head;
	tempnode2 = NULL;
	if(q->head == NULL)
		return NULL;
	else
	{
		while(1)
		{
			if(tempnode1->file->host == host)
			{
				q->head = q->head->next;
				free(tempnode1);
				continue;
			}
			else
				break;
		}
		while(tempnode1 != NULL)
			{
				tempnode2 = tempnode1;
				tempnode1 = tempnode1->next;
				if(tempnode1->file->host == host)
				{
					tempnode1->next = tempnode2->next;
					free(tempnode2);
					continue;
				}
				tempnode1 = tempnode1->next;
			}
		}

		return q;
}
			
