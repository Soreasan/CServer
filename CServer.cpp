/** @AUTHOR     Kenneth Adair
*   @FILENAME   CServer.cpp
*   The goal is for this server to be able to process a GET request from CURL and return the appropriate file.
*
*   Version 1.0
*   Completed 12/22/16
*   Successfully returns the contents of a file from a CURL GET request
*   After compiling and running this program, in a seperate window type:
*   curl localhost:1067/index.html
*   And then the server will display the contents of the requested file to the user.
*/
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <map>
//#include <string>
using namespace std;

int SERVER_PORT = 1067;
const int FILENAME_MAX_SIZE = 256;
const int FILENAME_BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE = 1048576;      //1 MiB, larger sizes such as 10 MiB cause segmentation faults.

/** @AUTHOR Kenneth Adair
*   Given a buffer this determines if it's a GET request by checking
*   the first 3 letters of the buffer.
*/
bool isGetRequest(unsigned char* buf, int size)
{
    return (strncmp(reinterpret_cast<const char*>(buf), "GET", (size < 3) ? size : 3) == 0);
}

/** @AUTHOR Kenneth Adair
*   Alternative approach to check if there's a GET request, doesn't only check first three characters
*/
bool isGetRequest2(unsigned char* buf, int size)
{
    // Cast the buf to a c++ string
    // http://stackoverflow.com/questions/1195675/convert-a-char-to-stdstring
    string buff(reinterpret_cast<const char*>(buf));
    // Check if the string contains the word GET 
    // http://stackoverflow.com/questions/2340281/check-if-a-string-contains-a-string-in-c
    if(buff.find("GET") != std::string::npos){
        return true;
    }
    return false;

}

/*
To parse a buffer there will be 4 stages:
GET /index.html HTTP/1.1
[thingy]: thingy
[thingy]: thingy
\n
So I need something to parse the request type, something to parse the URI, check if it says HTTP/1.1 or 
is empty (which means it's HTTP/1.0), then something to parse the attributes like content-type.  Once
I hit two newlines that means the request is done.  We need to consume it and move the stuff

When I send a response back the content-length will be sizeof(file) and doesn't include the size of the header
*/


/** @AUTHOR Kenneth Adair
*   Goes through the buffer and fills the hash table with all the components of our buffer
*/
void parseBuffer(unsigned char* buf, int size, map<string, string>* bufferComponents)
{
    //i will be used to track which character we're on
    int i = 0;
    //Casting the buffer to a string to make it easier to work with
    string buff(reinterpret_cast<const char*>(buf));
    //First we'll try to find the type of request
    for(; i < buff.length(); i++){
        if(buff[i] == ' '){
            bufferComponents->insert(pair<string, string>("RequestType", buff.substr(0, i)));
            cout << "The request type is: " << bufferComponents->find("RequestType")->second << endl;
            break;
        }
    }
    //At this point i should be at the square with the space so we should be able to search for the URI next
    //This moves us to the start of the URI
    for(; i < buff.length(); i++){
        if(buff[i] == '/')
            break;
    }
    //For the substring method we'll track where the URI starts.
    int startOfURI = i;
    for(; i < buff.length(); i++){
        if(buff[i] == ' ' || buff[i] == '\n'){
            bufferComponents->insert(pair<string, string>("URI", buff.substr(startOfURI, i - startOfURI + 1)));
            cout << "The URI is: " << bufferComponents->find("URI")->second << endl;
            break;
        }
    }
}

void getRequestType(unsigned char* buf, int* size, map<string, string>* bufferComponents)
{
    char* requestType = new char[*size];
    //First we'll try to find the type of request
    for(int i = 0; i < *size; i++){
        if(buf[i] == ' '){
            printf("Adding null terminator and finishing string.");
            requestType[i] = '\0';
            string request(requestType);
            bufferComponents->insert(pair<string, string>("RequestTypez", request));
            cout << "The request type is: " << bufferComponents->find("RequestTypez")->second << endl;
            buf = &buf[i];
            cout << "size was " << *size << endl;
            *size = *size - i;
            cout << "size is now " << *size << endl;
            break;
        }else{
            printf("Adding %c to the array\n", buf[i]);
            requestType[i] = buf[i];
        }
    }
}

void parseURI(unsigned char* buf, int size, map<string, string>* bufferComponents)
{

}

void getHTTPVersion(unsigned char* buf, int size, map<string, string>* bufferComponents)
{

}

void parseHeaders(unsigned char* buf, int size, map<string, string>* bufferComponents)
{

}

/** @AUTHOR Kenneth Adair
*   Goes through the buffer and fills the hash table with all the components of our buffer
*/
//first we need to identify what the request type is:
void parseBuffer2(unsigned char* buf, int size, map<string, string>* bufferComponents)
{
    getRequestType(buf, &size, bufferComponents);
    parseURI(buf, size, bufferComponents);
    getHTTPVersion(buf, size, bufferComponents);
    parseHeaders(buf, size, bufferComponents);
}

/** @AUTHOR Kenneth Adair
*   Given a buffer of a GET request from CURL this retrieves the name of the file.  
*/
void getFilenameFromUri(unsigned char* buf, unsigned char* filename, int size)
{
    //startingIndex is the 4th index, assuming that the first 4 symbols are "GET "
    int startingIndex = 4;
    for(int i = startingIndex; i < size; i++){
        if((char)buf[i + 1] == ' '){
            filename[i - startingIndex] = '\0';
            break;
        }
        filename[i - startingIndex] = buf[i + 1];
    }
}

/** @AUTHOR Kenneth Adair
*   This method writes the contents of a file to an open file descriptor.
*/
void returnFile(int out, unsigned char* filename)
{
    unsigned char buff[MAX_FILE_SIZE]={0};
    int count;
    FILE *fp = fopen(reinterpret_cast<const char*>(filename), "rb");      
    if(fp==NULL)
    {
        printf("File open error");
        return;   
    }else{
        printf("There is a filename and it's %s\n", filename);
    }
    int nread = fread(buff,1,MAX_FILE_SIZE,fp);
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
    if (nread < MAX_FILE_SIZE)
    {
        if (feof(fp))
            printf("End of file\n");
        if (ferror(fp))
            printf("Error reading\n");
    }
}

void myService(int in, int out, map<string, string>* bufferComponents)
{
    unsigned char buf[FILENAME_BUFFER_SIZE];
    unsigned char filename[FILENAME_BUFFER_SIZE];
    int count;
    count = read(in, buf, FILENAME_BUFFER_SIZE);
    /********************************/
    parseBuffer(buf, count, bufferComponents);
    /********************************/
    if(isGetRequest2(buf, count)){
        printf("This is a GET request\n");
        getFilenameFromUri(buf, filename, count);
        cout << "Filename is: " << filename << endl;
        //NO
        getRequestType(buf, &count, bufferComponents);
        cout << "MOMENT OF TRUTH: SIZE IS: " << count << endl;
        //NO

        returnFile(out, filename);
    }
}

/** @AUTHOR Kenneth Adair
*   This method checks for commandline arguments and if the user puts a port number it changes the port.
*   Otherwise it uses the default port number.
*/
void setPort(int argc, char *argv[])
{
    //If there is a commandline argument and it's a number set it to that number, otherwise use default value
	SERVER_PORT = (argc == 2 && strtol(argv[1], NULL, 10) != 0) ? strtol(argv[1], NULL, 10) : SERVER_PORT;
	cout << "Will listen on port " << SERVER_PORT << endl;
}

int main(int argc, char *argv[])
{
	setPort(argc, argv);
    //Boilerplate networking code
    int sock, fd, client_len;
    struct sockaddr_in server, client;
    struct servent *service;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(SERVER_PORT);
    bind(sock, (struct sockaddr *)&server, sizeof(server));
    if(listen(sock, 5) == -1)
    {
        printf("Failed to listen\n");
        return -1;
    }
    printf("listening...\n");

    //Wait for users to connect, then send them the contents of files if they send a GET request
    while(1){
        try
        {
            client_len = sizeof(client);
            fd = accept(sock, (struct sockaddr *)&client, (socklen_t*)  &client_len);
            printf("got connection\n");
            map<string, string> myMap;
            myService(fd, fd, &myMap);        
            close(fd);
        }
        catch(exception& e)
        {
            cout << e.what() << endl;
        }
    }

    return 0;
}
