// Kevin L
// https://github.com/kle510
// March 2016

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <iostream>
#include <vector>
#include <iterator> 
#include <fstream>
#include <sstream> 
#include <string>


using namespace std;

#define PORT "23971" 

//max number of bytes we can get at once (for a vector)
#define MAXDATASIZE 1024






int main(int argc, char *argv[])
{

    /*
    In accordance with Beej's Networking Guide, the client boots creates a socket, boots up the TCP socket, and attempts to connect to the server sharing the same port number. 
    */


    int sockfd, numbytes, rv;


    //buffer array
    char buf[MAXDATASIZE];


    struct addrinfo hints, *servinfo;


    //there must be two arguments (./client and file name)
    if (argc != 2) {
        cout << "ERROR Passing in Arguments" <<endl;
        exit(1);
    }



    /*
    Two techniques were considered when implementing the TCP connection part: getaddrinfo() from Beej's code, and gethostbyname() from the TCP socket tutorial on linuxhowto.com. Both methods were tested but ultimately, the Beej's code was chosen to be implemented since there is much more functionality options with getaddrinfo (see p.16 of Beej's Network Tutorial)
    
        http://beej.us/guide/bgnet/output/html/multipage/getaddrinfoman.html
    */


    memset(&hints, 0, sizeof hints); 
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM; 

    if ((rv = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }



    /*
    This code was slightly modified from the Beej's version of the TCP client. Because our code is a one-to-one, one time connection, there is no need to cycle through all avaialble clients -- so the for loop to create and connect the socket was taken out. 

    A series of errors are checked in TCP connections. These were kept mostly similar to Beej's, except changed so that the program automatically terminated should an error occur. 

    */




    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    //error in sockfd, print error and exit out
    if (sockfd == -1) {
        cout << "ERROR creating socket on client" <<endl;
        exit(1);
    }

    cout << "The client is up and running." <<endl;

    //if you make a socket and you cant connect it to server, skip it, print error, and exit out
    if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sockfd);
        cout << "Error on client connect"<<endl;
        exit(1);
    }

    //if nothing to connect, print error and end
    if (servinfo == NULL) {
        cout << "ERROR, client was unable to connect"<<endl;
        exit(1);
    }


    freeaddrinfo(servinfo); 

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';


    /*
    If the connection is successful, we can proceed. 

    The client then uses an ifstream to open and extract the lines of the file into a vector of strings. The lineCount was incremented for each line added to the vector.  The strings in the vector were then copied iteratively into a character array buffer and sent to edge.cpp.
    
    */


    //initialize string vector of extracted lines from file
    vector<string> extractedLines;


    //read input file -- http://www.cplusplus.com/doc/tutorial/files/

    //extracted string from a line of a filestream
    string line;

    //input file stream
    //DONT HARDCODE THE FILE NAME
    ifstream myfile;

    myfile.open(argv[1]);

    //number of lines from text file
    int lineCount = 0;


    if (myfile.is_open())
    {
        //while there is a file stream
        while (getline(myfile, line)){

            //add line to vector
            extractedLines.push_back(line);

            //increment the line count
            lineCount++;
        }
        myfile.close();
    }

    else {
        cout << "Error: File could not be opened";
        exit(1);

    }

    //Check to see how many characters are in the buffer. If there are more than what the buffer can fit, exit program.
    //safety check to make sure buf array doesnt exceed MAXDATASIZE
    int safetycheck = 0;

    for(int i=0; i<extractedLines.size(); ++i){
        safetycheck +=extractedLines[i].size();
    }


    if (safetycheck >= MAXDATASIZE) {
        cout << "Error: File is too big, wont fit in the buffer. Exit program" <<endl;
        exit(1);
    }


    //copy lines from text into buffer -- http://www.cplusplus.com/reference/cstring/strncpy/, beej C page 117


    //line in text file we're currently on. this is an index in the extracedLines array
    int lineNumber = 0;

    //current index of buf that we're on. want this to be less than MAXDATASIZE
    int bufferIndex = 0;


    //convert number of elements to string to pass it into the buffer
    stringstream ss;
    ss << extractedLines.size();
    string myString = ss.str();
    
    strncpy(buf+bufferIndex, myString.c_str(), myString.size());
    
    //update location of index in the buffer
    bufferIndex++;

    // line break \n between the lines 
    buf[bufferIndex] = '\n';
    bufferIndex++;


    //line count is same as extractedLines.size()
    while (lineNumber != lineCount) {


        //	}


        //safe copy
        strncpy(buf + bufferIndex, extractedLines[lineNumber].c_str(), extractedLines[lineNumber].size());

        //update location of index in the buffer
        bufferIndex += extractedLines[lineNumber].size();

        // line break \n between the lines 
        buf[bufferIndex] = '\n';
        bufferIndex++;


        //go to the next line in text
        lineNumber++;

    }

    if (send(sockfd, buf, strlen(buf), 0) == -1) {
        cout << "Error: Unable to send";
        exit(1);
    }

    cout << "The client has sucessfully finished sending " << lineCount << " lines to the edge server." << endl;

    
    /*
    The client then waits for a response from edge.cpp, prints out the resulting computation from the buffer received from edge.cpp, and shuts down.
    */
    

    //reset the buffer
    memset (buf,0,sizeof(buf));

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {
        perror("recv");
        exit(1);
    }


    cout << "The client has successfully finished receiving all computation results from the edge server." << endl;
    cout << "The final computation results are: " <<endl;
    cout << buf << endl;





    close(sockfd);

    return 0;
}
