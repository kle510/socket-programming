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
#define MAXDATASIZE 1024 // max number of bytes we can get at once (for a vector)

int main(int argc, char *argv[])
{
    // Client creates a socket, boots up the TCP socket, and attempts to connect to the server sharing the same port number.
    int sockfd, numbytes, rv;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo;

    // There must be two arguments (./client and file name)
    if (argc != 2)
    {
        cout << "ERROR Passing in Arguments" << endl;
        exit(1);
    }

    // Implement TCP connection part withgetaddrinfo(): http://beej.us/guide/bgnet/output/html/multipage/getaddrinfoman.html

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Create and connect the socket for the TCP connection
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    // Error in sockfd
    if (sockfd == -1)
    {
        cout << "ERROR creating socket on client" << endl;
        exit(1);
    }
    cout << "The client is up and running." << endl;
    // Error in server
    if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        close(sockfd);
        cout << "Error on client connect" << endl;
        exit(1);
    }
    // Client unable to connect
    if (servinfo == NULL)
    {
        cout << "ERROR, client was unable to connect" << endl;
        exit(1);
    }

    freeaddrinfo(servinfo);

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
    {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';

    // Use an input file stream to open and extract the lines of the file into a vector of strings
    vector<string> extractedLines;
    string line;
    ifstream myfile;

    myfile.open(argv[1]);

    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            extractedLines.push_back(line);
        }
        myfile.close();
    }
    else
    {
        cout << "Error: File could not be opened";
        exit(1);
    }

    // If there are more characters than what the buffer can fit, exit program.
    int safetyCheck = 0;

    for (int i = 0; i < extractedLines.size(); ++i)
    {
        safetyCheck += extractedLines[i].size();
    }

    if (safetyCheck >= MAXDATASIZE)
    {
        cout << "Error: File is too big, wont fit in the buffer. Exit program" << endl;
        exit(1);
    }

    // Start process of copying text from extractedLines into buffer
    int currLineNumber = 0;
    int currBufIndex = 0;

    // Convert number of elements to string to pass it into the buffer
    stringstream ss;
    ss << extractedLines.size();
    string myString = ss.str();

    strncpy(buf + currBufIndex, myString.c_str(), myString.size());

    currBufIndex++;
    buf[currBufIndex] = '\n'; // add a line break between lines
    currBufIndex++;

    // Copy text from extractedLines into buffer
    while (currLineNumber <= extractedLines.size())
    {
        strncpy(buf + currBufIndex, extractedLines[currLineNumber].c_str(), extractedLines[currLineNumber].size());

        currBufIndex += extractedLines[currLineNumber].size();
        buf[currBufIndex] = '\n';
        currBufIndex++;

        currLineNumber++;
    }

    // Send the contents of the buffer via the socket
    if (send(sockfd, buf, strlen(buf), 0) == -1)
    {
        cout << "Error: Unable to send";
        exit(1);
    }

    cout << "The client has sucessfully finished sending " << extractedLines.size() << " lines to the edge server." << endl;

    // Print out the resulting computation received by the buffer from edge.cpp and shut down.

    memset(buf, 0, sizeof(buf)); // Reset the buffer

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1)
    {
        perror("recv");
        exit(1);
    }

    cout << "The client has successfully finished receiving all computation results from the edge server." << endl;
    cout << "The final computation results are: " << endl;
    cout << buf << endl;

    close(sockfd);

    return 0;
}