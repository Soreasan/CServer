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
const string FILEPATH_FOLDER = "/Users/kennethadair/Documents/CServer";   //The filepath on my computer to the files to return.
const string FILEPATH2 = "~\\Documents\\CServer\\index.html";
const string FILEPATH3 = "/Users/kennethadair/Documents/CServer/index.html";
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
            bufferComponents->insert(pair<string, string>("URI", buff.substr(startOfURI, i - startOfURI)));
            break;
        }
    }
}

unsigned char* getRequestType(unsigned char* buf, int* size, map<string, string>* bufferComponents)
{
    char* requestType = new char[*size];
    int i = 0;
    //First we'll try to find the type of request
    for(; i < *size; i++){
        if(buf[i] == ' '){
            requestType[i] = '\0';
            string request(requestType);
            bufferComponents->insert(pair<string, string>("RequestType", request));
            *size = *size - i;
            break;
        }else{
            requestType[i] = buf[i];
        }
    }
    delete[] requestType;
    return &buf[i];
}

unsigned char* parseURI(unsigned char* buf, int* size, map<string, string>* bufferComponents)
{
    //Loop to the URI
    int i = 0;
    for(; i < *size; i++){
        //Search for a forward slash which indicates the start of a URI
        if(buf[i] == '/')
            break;
    }

    char* uri = new char[*size];


    //Now that we're at the start of a URI we loop through it to retrieve the URI
    for(int j = 0; i < *size; i++, j++){
        if(buf[i] == ' ' || buf[i] == '\n'){
            uri[j] = '\0';
            string myUri(uri);
            bufferComponents->insert(pair<string, string>("URI", myUri));
            /*************************************/
            //Creating the filename we'll return
            string filepath;
            filepath.append(FILEPATH_FOLDER);
            filepath.append(myUri);
            cout << "\"" << filepath << "\"" << endl;
            bufferComponents->insert(pair<string, string>("Filepath", filepath));
            /*************************************/
            *size = *size - i;
            break;
        }else{
            uri[j] = buf[i];
        }
    }
    //prevent memory leaks
    delete[] uri;
    return &buf[i];
}

unsigned char* getHTTPVersion(unsigned char* buf, int* size, map<string, string>* bufferComponents)
{
    int i = 0;
    //iterate through whitespace
    for(; i < *size; i++){
        //If there's nothing then it's a HTTP 1.0 request
        if(buf[i] == '\n'){
            bufferComponents->insert(pair<string, string>("HTTPVersion", "HTTP/1.0"));
            *size = *size - i - 1;
            return &buf[i + 1];
        }
        //Once we've gone through the whitespace we break the loop
        if(buf[i] != ' ')
            break;
    }

    //This will be used to store the new string we're building
    char* http = new char[*size];

    for(int j = 0; i < *size; i++, j++){
        if(buf[i] == ' ' || buf[i] == '\n'){
            http[j] = '\0';
            string myHttp(http);
            bufferComponents->insert(pair<string, string>("HTTPVersion", myHttp));
            *size = *size - i - 1;
            break;
        }else{
            http[j] = buf[i];
        }
    }
    delete[] http;
    return &buf[i + 1];
}

//We'll recursively call this method until we find two newlines.
unsigned char* parseHeaders(unsigned char* buf, int* size, map<string, string>* bufferComponents)
{
    //We assume we're at the start of a new line.
    int i = 0;
    char* key = new char[*size];
    //iterate through until ":"
    for(; i < *size; i++){
        if(buf[i] == ':'){
            key[i] = '\0';
            break;
        }else{
            key[i] = buf[i];
        }
    }
    //Iterate through whitespace
    for(; i < *size; i++){
        if(buf[i] != ' ' && buf[i] != ':')
            break;
    }

    char* value = new char[*size];

    for(int j = 0; i < *size; i++, j++){
        if(buf[i] == '\n'){
            value[j] = '\0';
            string myKey(key);
            string myValue(value);
            bufferComponents->insert(pair<string, string>(myKey, myValue));
            *size = *size - i - 1;
            if(buf[i + 1] == '\r' && buf[i + 2] == '\n'){
                delete[] key;
                delete[] value;
                return &buf[i];
            }
            break;
        }else{
            value[j] = buf[i];
        }
    }
    delete[] key;
    delete[] value;
    return parseHeaders(&buf[i + 1], size, bufferComponents);
}

/** @AUTHOR Kenneth Adair
*   Goes through the buffer and fills the hash table with all the components of our buffer
*/
//first we need to identify what the request type is:
void parseBuffer2(unsigned char* buf, int* size, map<string, string>* bufferComponents)
{
    buf = getRequestType(buf, size, bufferComponents);
    buf = parseURI(buf, size, bufferComponents);
    buf = getHTTPVersion(buf, size, bufferComponents);
    buf = parseHeaders(buf, size, bufferComponents);
    cout << "We should theoretically successfully have parsed the thing if we make it " << endl;
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

void getFilenameFromUri2(string filenameToConvert, unsigned char* filename, int size)
{
    int i = 0;
    for(; i < filenameToConvert.length(); i++){
        filename[i] = filenameToConvert.at(i);
    }
    filename[i] = '\0';
    cout << "Filename is: " << filename << endl;
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
    //parseBuffer(buf, count, bufferComponents);
    /********************************/
    if(isGetRequest2(buf, count)){
        //getFilenameFromUri(buf, filename, count);
        parseBuffer2(buf, &count, bufferComponents);
        typedef map<string, string>::const_iterator MapIterator;
        for(MapIterator iter = bufferComponents->begin(); iter != bufferComponents->end(); iter++){
            cout << "Key: " << iter->first << endl << "Value: " << iter->second << endl;
        }
        //bufferComponents->find("Filepath")->second;
        //FILEPATH2
        getFilenameFromUri2(bufferComponents->find("Filepath")->second, filename, count);
        //getFilenameFromUri2(FILEPATH3, filename, count);
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
