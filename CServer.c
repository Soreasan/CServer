#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVER_PORT 1067

char *choppy(char *s)
{
	char *n = malloc(strlen(s ? s : "\n"));
	if(s)
		strcpy(n, s);
	n[strlen(n) - 1] = '\0';
	return n;
}

void myService(int in, int out)
{
    unsigned char buf[1024];
    int count;
    while((count = read(in, buf, 1024)) > 0)
        write(out, buf, count);
}

void returnFile(int in, int out)
{
		unsigned char buff[256]={0};
		int count;
	//while((count = read(in, buff, 65535)) > 0){
		//write(out, buff, count);
		//printf("User wrote this:");		
		printf(buff);
		//char *test = choppy(buff);
		//printf("Test succeeded");		
		//printf(test);
		FILE *fp = fopen("sample_file.txt", "rb");		
		//FILE *fp = fopen("sample_file.txt", "rb");
        	if(fp==NULL)
        	{
            		printf("File open error");
            		return;   
        	}
		/* First read file in chunks of 256 bytes */
        	//unsigned char buff[256]={0};
        	int nread = fread(buff,1,256,fp);
        	printf("Bytes read %d \n", nread);


        	/* If read was success, send data. */
        	if(nread > 0)
        	{
            		printf("Sending \n");
            		write(out, buff, nread);
        	}

        	/*
        	 * There is something tricky going on with read .. 
        	 * Either there was error, or we reached end of file.
        	 */
	
        	if (nread < 256)
        	{
            	    if (feof(fp))
                	printf("End of file\n");
        	    if (ferror(fp))
        	        printf("Error reading\n");
        	}
	//}
	
}

void main()
{
    int sock, fd, client_len;
    struct sockaddr_in server, client;
    struct servent *service;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(SERVER_PORT);
    bind(sock, (struct sockaddr *)&server, sizeof(server));
    listen(sock, 5);
    printf("listening...\n");

    while(1){
        client_len = sizeof(client);
        fd = accept(sock, (struct sockaddr *)&client, &client_len);
        printf("got connection\n");
        //myService(fd, fd);
	returnFile(fd, fd);
	//myService(fd, fd);        
	close(fd);
    }
}
