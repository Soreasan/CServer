/** @AUTHOR     Kenneth Adair
*   @FILENAME   CServer.cpp
*   The goal is for this server to be able to process a GET request from CURL and return the appropriate file.
*
*   Version 1.0
*   Completed 12/22/16
*   Successfully returns the contents of a file from a CURL GET request
*   After compiling and running this program, in a seperate window type:
*   curl localhost:1067/filname
*   And then the server will display the contents of the [filename] to the user.
*
*   Version 1.1
*   Updated syntax without altering any functionality and kept line numbers mostly same.
*/
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>


#include <cstring>
#include <unistd.h>
#include <string>
#include <iostream>
using namespace std;

const int FILE_CONTENTS_SIZE = 104857600;  //100 MiB, a MiB is 2^20 bytes
const int FILENAME_SIZE = 255;
const int SERVER_PORT = 1067;    //Can't use port 80 right now b/c my machine is running LAMP

/** @AUTHOR Kenneth Adair
*   Given a buffer this determines if it's a GET request by checking
*   the first 3 letters of the buffer.
*/
bool isGetRequest(unsigned char *  buf)
{
     return (((char) buf[0]) == 'G' && ((char) buf[1]) == 'E' && ((char) buf[2]) == 'T');
}

/** @AUTHOR Kenneth Adair
*   Given a buffer of a GET request from CURL this retrieves the name of the file.  
*/
string getFilenameFromUri(unsigned char * buf)
{

    char filename[FILENAME_SIZE];
    int start, end, i;
    //Picked 4 beccause if it's a GET request then GET and a space eat up first 4 spots, index 4 should be a /
    start = end = 4;
    //Loop to the end of the filename by checking for spaces.
    while(((char)buf[end]) != ' ')
        end++;

    //Moves all the characters in the filename into a new array from the buffer
    for(i = start; i < end; i++)
        filename[i - start] = (char) buf[i];

    /** FIX LATER
    *   Awkwardly solves the problem that preceding code has a "\" which prevents my filename from being right.
    *   This loop literally just removes the "\" from the filename.
    */
    for(i = start; i < end - 1; i++)
        filename[i - start] = filename[i - start + 1];

    //Put a null terminator at the end of the string and then return it
    filename[end - start - 1] = '\0';
    return filename;
}
/** @AUTHOR Kenneth Adair
*   This method displays the contents of a file out to a user 
*   up to a MiB (2^20 bytes) big by writing to an open file descriptor.  
*/
void returnFile(int out, string filename)
{
    unsigned char buff[FILE_CONTENTS_SIZE]={0};
    int count;
    FILE *fp = fopen(filename.c_str(), "rb");      
    if(fp==NULL)
    {
        printf("File open error");
        return;   
    }
    int nread = fread(buff, 1, FILE_CONTENTS_SIZE, fp);
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
    if (nread < FILE_CONTENTS_SIZE)
    {
        if (feof(fp))
            printf("End of file\n");
        if (ferror(fp))
            printf("Error reading\n");
    }
}
/*  This method reads the buffer, determines if it's a GET request,
*   then if it is calls returnFile to satisfy the request.  Otherwise do nothing.
*/
void myService(int in, int out){
    unsigned char buf[1024] = "0x00";
    int count;
    count = read(in, buf, 1024);
    //write(out, buf, count);   //This is how we echo back the request that was sent

    if(isGetRequest(buf)){
        printf("This is a GET request\n");
        cout << "Filename is: " << getFilenameFromUri(buf) << endl;
        returnFile(out, getFilenameFromUri(buf));
     }else{
        printf("Not a GET request\n");
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
