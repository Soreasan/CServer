/** @AUTHOR     Kenneth Adair
*   @FILENAME   CServer.cpp
*   The goal is for this server to be able to process a GET request from CURL and return the appropriate file.
*
*   Version 1.0
*   Completed 12/22/16
*   Successfully returns the contents of a file from a CURL GET request
*   After compiling and running this program, in a seperate window type:
*   curl localhost:1067/index.html
*   And then the server will display the contents of the index.html file to the user.
*/
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <string.h>
#include <iostream>
using namespace std;

const int SERVER_PORT = 1067;
const int FILENAME_MAX_SIZE = 255;

/** @AUTHOR Kenneth Adair
*   Given a buffer this determines if it's a GET request by checking
*   the first 3 letters of the buffer.
*/
bool isGetRequest(unsigned char* buf, int size)
{
    return (strncmp(reinterpret_cast<const char*>(buf), "GET", (size < 3) ? size : 3) == 0);
}

/** @AUTHOR Kenneth Adair
*   Given a buffer of a GET request from CURL this retrieves the name of the file.  
*/
void getFilenameFromUri(unsigned char* buf, unsigned char* filename, int size)
{
    for(int i = 4; i < size; i++){
        if((char)buf[i + 1] == ' '){
            filename[i - 4] = '\0';
            break;
        }
        filename[i - 4] = buf[i + 1];
    }
}

/** @AUTHOR Kenneth Adair
*   This method writes the contents of a file out
*   up to 65535 bytes big.  
*/
void returnFile(int in, int out, unsigned char* filename)
{
    unsigned char buff[65535]={0};
    int count;
    FILE *fp = fopen(reinterpret_cast<const char*>(filename), "rb");      
    if(fp==NULL)
    {
        printf("File open error");
        return;   
    }else{
        printf("There is a filename and it's %s\n", filename);
    }
    int nread = fread(buff,1,65535,fp);
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
    if (nread < 65535)
    {
        if (feof(fp))
            printf("End of file\n");
        if (ferror(fp))
            printf("Error reading\n");
    }
}

void myService(int in, int out)
{
    unsigned char buf[1024];
    unsigned char filename[1024];
    int count;
    count = read(in, buf, 1024);
    if(isGetRequest(buf, count)){
        printf("This is a GET request\n");
        getFilenameFromUri(buf, filename, count);
        cout << "Filename is: " << filename << endl;
        returnFile(in, out, filename);
    }
}

int main()
{
    //Boilerplate networking code
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

    //Wait for users to connect, then send them the contents of files if they send a GET request
    while(1){
        client_len = sizeof(client);
        fd = accept(sock, (struct sockaddr *)&client, (socklen_t*)  &client_len);
        printf("got connection\n");
        myService(fd, fd);        
        close(fd);
    }

    return 0;
}
