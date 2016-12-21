#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>

//Stuff needed for c++
#include <cstring>
#include <unistd.h>
#include <string>

//C++ Default stuff
#include <iostream>
using namespace std;

#define SERVER_PORT 1067

//this just echoes whatever it received.  Used for testing
void myService(int in, int out)
{
    printf("myService.\n");
    unsigned char buf[1024];
    int count;
    char hexString[1024];
    //while((count = read(in, buf, 1024)) > 0){
        //printf("testing\n");
        count = read(in, buf, 1024);
        write(out, buf, count);
    //}
    printf("You made it this far.");
    for(int i = 0; i < count; i++){
        printf("Hexadecimal: %xhh, %d\n", buf[i], buf[i]);
        //printf("This: ");
        //sprintf(hexString, "0x%08X", buf[i]);
        //printf("\n");
    }
}

void returnFile(int in, int out)
{
        unsigned char buff[256]={0};
        int count;
        FILE *fp = fopen("sample_file.txt", "rb");      
            if(fp==NULL)
            {
                    printf("File open error");
                    return;   
            }
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
}

int main()
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
        fd = accept(sock, (struct sockaddr *)&client, (socklen_t*)  &client_len);
        printf("got connection\n");
        //returnFile(fd, fd);
        myService(fd, fd);        
        //close(fd);
    }
    //Now i don't get that weird error
    close(fd);
    return 0;
}
