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

bool checkForGet(unsigned char buf[])
{
     return (((char) buf[0]) == 'G' && ((char) buf[1]) == 'E' && ((char) buf[2]) == 'T');
}

string checkForWebpage(unsigned char buf[])
{
    char filename[200];
    int start, end, i;
    start = end = 4;
    while(((char)buf[end]) != ' '){
        printf("%i\n", end);
        end++;
    }
    printf("end is %d", end);
    for(i = start; i < end; i++){
        printf("Putting %c into string\n", buf[i]);
        filename[i - start] = (char) buf[i];
    }
    cout << filename << endl;
    printf("%s\n", filename);
    filename[end] = '\0';
    string filenamez = filename;
    cout << filenamez << endl;
    return filename;
}

//this just echoes whatever it received.  Used for testing
void myService(int in, int out)
{
    printf("myService.\n");
    unsigned char buf[1024];
    int count;
    //char hexString[1024];
    //while((count = read(in, buf, 1024)) > 0){
        //printf("testing\n");
        count = read(in, buf, 1024);
        write(out, buf, count);
    //}


    printf("You made it this far.");
    for(int i = 0; i < count; i++){
        //printf("Hexadecimal: %xhh, %d\n", buf[i], buf[i]);
        //printf("This: ");
        //sprintf(hexString, "0x%08X", buf[i]);
        //printf("\n");
        //int number = (int) strtol(reinterpret_cast<const char *>(buf[i]), NULL, 0);
        //printf("%d\n", number);
        int j = (int) buf[i];
        char cool = (char) j;
        char cool2 = (char) buf[i];
        printf("%d, %c, %c\n", j, cool, cool2);
    }

    //char a, b, c;
    //a = (char) buf[0];
    //b = (char) buf[1];
    //c = (char) buf[2];
    //if(a == 'G' && b == 'E' && c == 'T'){
        if(checkForGet(buf)){
            printf("This is a GET request\n");
            //printf("Filename is: %s\n", checkForWebpage(buf));
            cout << "Filename is: " << checkForWebpage(buf) << endl;
        }else{
            printf("Not a GET request\n");
        }
    //}
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
