/** @AUTHOR     Kenneth Adair
*   @FILENAME   CServer.cpp
*   The goal is for this server to be able to process a GET request from CURL and return the appropriate file.
*
*   To run this use:
*   c++ CServer.cpp
*   ./a.out 1068 /Users/kennethadair/Documents/Python
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
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
//Cast size_t to string
#include <sstream>
using namespace std;

int SERVER_PORT = 1067;
const int FILENAME_MAX_SIZE = 256;
const int FILENAME_BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE = 1048576;      //1 MiB, larger sizes such as 10 MiB cause segmentation faults.
string FILEPATH_FOLDER = "/Users/kennethadair/Documents/CServer";   //The filepath on my computer to the files to return.

//Retrieving information over the commandline.
void setPort(int argc, char *argv[]);
void setFilepath(int argc, char *argv[]);

//Calls all the other methods for the server
void myService(int in, int out, map<string, string>* bufferComponents);

//These methods are used to parse the requests the server receives
void parseBuffer(unsigned char* buf, int* size, map<string, string>* bufferComponents);
unsigned char* getRequestType(unsigned char* buf, int* size, map<string, string>* bufferComponents);
unsigned char* parseURI(unsigned char* buf, int* size, map<string, string>* bufferComponents);
unsigned char* getHTTPVersion(unsigned char* buf, int* size, map<string, string>* bufferComponents);
unsigned char* parseHeaders(unsigned char* buf, int* size, map<string, string>* bufferComponents);

//This method converts strings to unsigned char*
void getFilenameFromUri(string filenameToConvert, unsigned char* filename, int size);
void castStringToChar(string stringToConvert, char* output, int outputSize);

//This method sends the file back
void returnFile(int out, unsigned char* filename);


//serverResponse will be a character array that we'll pass from method to method until it's completely built.
void returnProperlyFormattedResponse(int out, unsigned char* filename, string* serverResponse, map<string, string>* bufferComponents);

//These methods are used to append certain components of the response.
void prepareFile(map<string, string>* bufferComponents);
void appendHttpVersion(string* serverResponse, map<string, string>* bufferComponents);
void appendResponseCodeAndOK(string* serverResponse, map<string, string>* bufferComponents);
void appendDate(string* serverResponse, map<string, string>* bufferComponents);
void appendServer(string* serverResponse, map<string, string>* bufferComponents);
void appendLastModified(string* serverResponse, map<string, string>* bufferComponents);
void appendEtag(string* serverResponse, map<string, string>* bufferComponents);
void appendContentType(string* serverResponse, map<string, string>* bufferComponents);
void appendContentLength(string* serverResponse, map<string, string>* bufferComponents);
void appendAcceptRanges(string* serverResponse, map<string, string>* bufferComponents);
void appendConnectionCloseAndTwoNewLines(string* serverResponse, map<string, string>* bufferComponents);
void appendFileContents(string* serverResponse, map<string, string>* bufferComponents);
void sendProperlyFormattedResponse(int out, unsigned char* filename, string* serverResponse, map<string, string>* bufferComponents);

int main(int argc, char *argv[])
{
	setPort(argc, argv);
    setFilepath(argc, argv);
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

/** @AUTHOR Kenneth Adair
*   This method checks for commandline arguments and if the user puts a port number it changes the port.
*   Otherwise it uses the default port number.
*/
void setPort(int argc, char *argv[])
{
    //If there is a commandline argument and it's a number set it to that number, otherwise use default value
	SERVER_PORT = ((argc == 2 || argc == 3) && strtol(argv[1], NULL, 10) != 0) ? strtol(argv[1], NULL, 10) : SERVER_PORT;
	cout << "Will listen on port " << SERVER_PORT << endl;
}

void setFilepath(int argc, char *argv[])
{
  FILEPATH_FOLDER = (argc == 3) ? argv[2] : FILEPATH_FOLDER;
  cout << "Will return files in the directory \"" << FILEPATH_FOLDER << "\"" << endl;
}

void myService(int in, int out, map<string, string>* bufferComponents)
{
    unsigned char buf[FILENAME_BUFFER_SIZE];
    unsigned char filename[FILENAME_BUFFER_SIZE];
    int count;
    count = read(in, buf, FILENAME_BUFFER_SIZE);
    parseBuffer(buf, &count, bufferComponents);
    typedef map<string, string>::const_iterator MapIterator;
    for(MapIterator iter = bufferComponents->begin(); iter != bufferComponents->end(); iter++){
        cout << "Key: " << iter->first << endl << "Value: " << iter->second << endl;
    }
    if(bufferComponents->find("RequestType")->second == "GET"){
        getFilenameFromUri(bufferComponents->find("Filepath")->second, filename, count);
				string serverResponse = "";
				returnProperlyFormattedResponse(out, filename, &serverResponse, bufferComponents);
    }
}

/** @AUTHOR Kenneth Adair
*   Goes through the buffer and fills the hash table with all the components of our buffer
*/
//first we need to identify what the request type is:
void parseBuffer(unsigned char* buf, int* size, map<string, string>* bufferComponents)
{
    buf = getRequestType(buf, size, bufferComponents);
    buf = parseURI(buf, size, bufferComponents);
    buf = getHTTPVersion(buf, size, bufferComponents);
    buf = parseHeaders(buf, size, bufferComponents);
    cout << "We should theoretically successfully have parsed the thing if we make it " << endl;
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
        if(buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\r'){
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
    if(*size < 0)
      return &buf[0];
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

void getFilenameFromUri(string filenameToConvert, unsigned char* filename, int size)
{
    int i = 0;
    for(; i < filenameToConvert.length(); i++){
        filename[i] = filenameToConvert.at(i);
    }
    filename[i] = '\0';
    cout << "Filename is: " << filename << endl;
}

void castStringToChar(string stringToConvert, char* output, int outputSize)
{
  int i = 0;
  int size = (stringToConvert.length() > outputSize) ? stringToConvert.length() : outputSize;
  for(; i < size; i++){
      output[i] = stringToConvert.at(i);
  }
  output[i] = '\0';
}


//serverResponse will be a character array that we'll pass from method to method until it's completely built.
void returnProperlyFormattedResponse(int out, unsigned char* filename, string* serverResponse, map<string, string>* bufferComponents){
	serverResponse = new string;
	*serverResponse = "";
  prepareFile(bufferComponents);
	appendHttpVersion(serverResponse, bufferComponents);
	appendResponseCodeAndOK(serverResponse, bufferComponents);
	appendDate(serverResponse, bufferComponents);
	appendServer(serverResponse, bufferComponents);
	appendLastModified(serverResponse, bufferComponents);
	appendEtag(serverResponse, bufferComponents);
	appendContentType(serverResponse, bufferComponents);
	appendContentLength(serverResponse, bufferComponents);
	appendAcceptRanges(serverResponse, bufferComponents);
	appendConnectionCloseAndTwoNewLines(serverResponse, bufferComponents);
	appendFileContents(serverResponse, bufferComponents);
    sendProperlyFormattedResponse(out, filename, serverResponse, bufferComponents);
    cout << "serverResponse is: \n" << *serverResponse << endl;
	delete serverResponse;
}
/**
  * First retrieve the filepath from our bufferComponents map.
  * Then check if the file is there.
  * If the file is there move its contents into the bufferComponents map
  * If the file isn't there then move the contents of a 404 error message into bufferComponents
  */
void prepareFile(map<string, string>* bufferComponents){
		stringstream ostr;

    unsigned char buff[MAX_FILE_SIZE]={0};
    int count;
    //FILE *fp = fopen(reinterpret_cast<const char*>(filename), "rb");
    //bufferComponents->insert(pair<string, string>("Filepath", filepath));
    string filename = bufferComponents->find("Filepath")->second;
    FILE *fp = fopen(reinterpret_cast<const char*>(filename.c_str()), "rb");

		string error404 = "<html><head><title>404 File not found</title></head><body><h1>404 File Not Found</h1></body></html>";

/*
    if(fp==NULL)
    {
        bufferComponents->insert(pair<string, string>("Filecontents", error404));
				//http://stackoverflow.com/questions/10510077/size-t-convert-cast-to-string-in-c
				//http://stackoverflow.com/questions/5290089/how-to-convert-a-number-to-string-and-vice-versa-in-c
				ostr << error404.length();
				string theNumberString = ostr.str();
				bufferComponents->insert(pair<string, string>("Contentlength", theNumberString));
        return;
    }
*/
		if(fp){
	    int nread = fread(buff,1,MAX_FILE_SIZE,fp);
	    printf("Bytes read %d \n", nread);

	    if(nread > 0)
	    {
				//PLACEHOLDER
				string filecontents(reinterpret_cast<char*>(buff));
	      bufferComponents->insert(pair<string, string>("Filecontents", filecontents));
				//http://stackoverflow.com/questions/10510077/size-t-convert-cast-to-string-in-c
				//http://stackoverflow.com/questions/5290089/how-to-convert-a-number-to-string-and-vice-versa-in-c
				ostr << filecontents.length();
				string theNumberString = ostr.str();
				bufferComponents->insert(pair<string, string>("Contentlength", theNumberString));
	    }else{
	        bufferComponents->insert(pair<string, string>("Filecontents", error404));
					//http://stackoverflow.com/questions/10510077/size-t-convert-cast-to-string-in-c
					//http://stackoverflow.com/questions/5290089/how-to-convert-a-number-to-string-and-vice-versa-in-c
					ostr << error404.length();
					string theNumberString = ostr.str();
					bufferComponents->insert(pair<string, string>("Contentlength", theNumberString));
	    }
		}else{
			bufferComponents->insert(pair<string, string>("Filecontents", error404));
			//http://stackoverflow.com/questions/10510077/size-t-convert-cast-to-string-in-c
			//http://stackoverflow.com/questions/5290089/how-to-convert-a-number-to-string-and-vice-versa-in-c
			ostr << error404.length();
			string theNumberString = ostr.str();
			bufferComponents->insert(pair<string, string>("Contentlength", theNumberString));
			return;
		}
}
/*
//http://stackoverflow.com/questions/174531/easiest-way-to-get-files-contents-in-c
void test(map<string, string>* bufferComponents){
	char * buffer = 0;
	long length;
	FILE * f = fopen (filename, "rb");

	if (f)
	{
	  fseek (f, 0, SEEK_END);
	  length = ftell (f);
	  fseek (f, 0, SEEK_SET);
	  buffer = malloc (length);
	  if (buffer)
	  {
	    fread (buffer, 1, length, f);
	  }
	  fclose (f);
	}

	if (buffer)
	{
	  // start to process your data / extract strings here...
	}
}
*/

//Append the HTTP version and a space.
void appendHttpVersion(string* serverResponse, map<string, string>* bufferComponents)
{
	*serverResponse += bufferComponents->at("HTTPVersion");
	*serverResponse += " ";
}

void appendResponseCodeAndOK(string* serverResponse, map<string, string>* bufferComponents)
{
	string response = "200 OK\n";
	serverResponse->append(response);
}

void appendDate(string* serverResponse, map<string, string>* bufferComponents)
{
	time_t rawtime;
	struct tm *timeinfo;
	char buffer[80];
	time(&rawtime);
	timeinfo = localtime(&rawtime);
    strftime(buffer, 80, "Date: %a, %d %b %G %T %Z\n", timeinfo);
	*serverResponse += buffer;
}

void appendServer(string* serverResponse, map<string, string>* bufferComponents)
{
    serverResponse->append("Server: CServer/1.0.0\n");
}

void appendLastModified(string* serverResponse, map<string, string>* bufferComponents)
{
    struct stat result;
    if(stat(bufferComponents->find("Filepath")->second.c_str(), &result) == 0){
        time_t rawtime = result.st_mtime;
        struct tm *timeinfo;
        char buffer[80];
        //time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, 80, "Last-Modified: %a, %d %b %G %T %Z\n", timeinfo);
        *serverResponse += buffer;
    }else{
	   serverResponse->append("Last-Modified: Wed, 18 Jun 2003 16:05:58 GMT\n");
    }
}

void appendEtag(string* serverResponse, map<string, string>* bufferComponents)
{
	serverResponse->append("ETag: \"56d-9989200-1132c580\"\n");
}

void appendContentType(string* serverResponse, map<string, string>* bufferComponents)
{
	serverResponse->append("Content-Type: text/html\n");
}

void appendContentLength(string* serverResponse, map<string, string>* bufferComponents)
{
	//serverResponse->append("Content-Length: 86\n");
	serverResponse->append("Content-Length: ");
	serverResponse->append(bufferComponents->at("Contentlength"));
	serverResponse->append("\n");
}

void appendAcceptRanges(string* serverResponse, map<string, string>* bufferComponents)
{
	serverResponse->append("Accept-Ranges: bytes\n");
}

void appendConnectionCloseAndTwoNewLines(string* serverResponse, map<string, string>* bufferComponents)
{
	serverResponse->append("Connection: close\n\n");
}

void appendFileContents(string* serverResponse, map<string, string>* bufferComponents)
{
	//serverResponse->append("<html><head><title>Hello World</title></head><body><h1>Hello World!</h1></body></html>");
	serverResponse->append(bufferComponents->at("Filecontents"));
}

void sendProperlyFormattedResponse(int out, unsigned char* filename, string* serverResponse, map<string, string>* bufferComponents)
{
	//const unsigned char * HARDCODED_REPLY2 = reinterpret_cast<const unsigned char *>((*serverResponse).c_str());
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
	    cout << "Sending" << endl;
			cout << endl << *serverResponse << endl;
			char output[MAX_FILE_SIZE];
			castStringToChar(*serverResponse, output, serverResponse->length());
			send(out, reinterpret_cast<const unsigned char *>(output), serverResponse->length(), 0);
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
