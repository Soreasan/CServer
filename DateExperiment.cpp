#include <iostream>
#include <string>
#include <time.h>
using namespace std;

int main(){
    //string goalstring = "Date: Thu, 19 Feb 2009 12:27:04 GMT\n";
    
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    //strftime(buffer, 80, "Now it's %I:%M%p.", timeinfo);
    //puts(buffer);
    
    strftime(buffer, 80, "Date: %c %Z", timeinfo);
    //puts(buffer);

    string test = "";
    test += buffer;
    cout << test << endl;
    return 0;
}
