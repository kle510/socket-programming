#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <string.h>

using namespace std;

#define UDP_PORT_OR "21971"
#define UDP_PORT_AND "22971"
#define TCP_PORT "23971"
#define UDP_PORT_EDGE "24971"
#define BACKLOG 10 //typical backlog size
#define MAXDATASIZE 1024
#define MAXBUFLEN 100

int main(void)
{
    // At startup, edge.cpp starts a server-side TCP connection and starts listening for clients

    int sockfd;
    int new_fd; // listen on sock_fd, new connection on new_fd
    char edgeBufIn[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, TCP_PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // Now, servinfo now points to a linked list of 1 or more struct addrinfos.
    // The server loops through all the struct addrinfos, creates a socket, and binds the first availalble addrinfo to the server's address.
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        // Try creating a socket with the current addrinfo
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            cout << "ERROR creating socket on server" << endl;
            continue;
        }

        // Try setting socket options for our socket
        // The purpose of setsockopt is to control socket behavior.
        // In this case, we set it so bind can reuse local addresses.
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            cout << "ERROR, setting socket options failed. Exit program. " << endl;
            exit(1);
        }

        // Try binding the socket to the server's address
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            cout << "ERROR, binding error on current socket" << endl;
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        cout << "ERROR, unable to bind a socket to the server. Exit program. " << endl;
        exit(1);
    }

    freeaddrinfo(servinfo);

    // Listen for socket connections and limit the queue of incoming connections
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1)
    {
        perror("accept");
        exit(1);
    }

    if (send(new_fd, "Hello, world!", 13, 0) == -1)
    {
        perror("send");
    }

    if (recv(new_fd, edgeBufIn, MAXDATASIZE - 1, 0) == -1)
    {
        perror("recv");
        exit(1);
    }

    cout << "The edge server has received " << edgeBufIn[0] << " lines from the client using TCP over port " << TCP_PORT << endl;

    //make new char array to get rid of element number
    char edgeBuf[MAXDATASIZE];

    for (int i = 0; i < MAXDATASIZE; ++i)
    {
        edgeBuf[i] = edgeBufIn[i + 2];
    }

    // When the edge server receives and accepts a connection from the client, it runs the UDPCircuit while loop.
    // The UDP circuit while loop separates the lines from the character buffer received from the client into and/or based on the ASCII distinction of the character.
    // From there, it sends the line to the sendToBackEnd() method, which opens a UDP client to send the line to the designated back-end server for computation.
    // THe UDP circuit while loop then runs the receiveFromBackEnd() method to open a UDP server to take back the result of the client and stuff it into a vector of strings containing the results .

    bool UDPCircuit = true;
    int edgeIndex = 0; //UDP circuit counter
    int andCount = 0;
    int orCount = 0;

    vector<string> extractedLines;

    while (UDPCircuit)
    {
        //97 ASCII character corresponds to letter a
        if (edgeBuf[edgeIndex] == 97)
        {
            char andBuf[MAXDATASIZE];
            int andBufIndex = 0;
            char *portnum = UDP_PORT_AND;

            while (edgeBuf[edgeIndex] != '\n')
            {
                andBuf[andBufIndex] = edgeBuf[edgeIndex];
                edgeIndex++;
                andBufIndex++;
            }

            andBuf[andBufIndex] = '\n'; //pad the end so we have a stopping point in the back-end server
            edgeIndex++;                //advance the counter past the space

            sendToBackEnd(andBuf, portnum);
            string receiveBuf = receiveFromBackEnd();
            extractedLines.push_back(receiveBuf);

            andCount++;
        }
        // 111 ASCII character corresponds to letter o
        else if (edgeBuf[edgeIndex] == 111)
        {
            char orBuf[MAXDATASIZE];
            int orBufIndex = 0;
            char *portnum = UDP_PORT_OR;

            while (edgeBuf[edgeIndex] != '\n')
            {
                orBuf[orBufIndex] = edgeBuf[edgeIndex];
                edgeIndex++;
                orBufIndex++;
            }

            orBuf[orBufIndex] = '\n'; //pad the end so we have a stopping point in the back-end server
            edgeIndex++;              //advance the counter past the space

            sendToBackEnd(orBuf, portnum);
            string receiveBuf = receiveFromBackEnd();
            extractedLines.push_back(receiveBuf);

            orCount++;
        }
        // Terminate both and + or servers, exit loop
        else
        {
            char andBuf[MAXDATASIZE];
            andBuf[0] = '\n';
            sendToBackEnd(andBuf, UDP_PORT_AND);

            char orBuf[MAXDATASIZE];
            orBuf[0] = '\n';
            sendToBackEnd(orBuf, UDP_PORT_OR);

            UDPCircuit = false;
        }
    }

    cout << "The edge server has successfully sent " << andCount << " lines to Backend-Server OR" << endl;
    cout << "The edge server has successfully sent " << orCount << " lines to Backend-Server AND" << endl;
    cout << "The edge server start receiving the computation results from Backend-Server OR and Backend-Server AND using UDP port " << UDP_PORT_EDGE << endl;

    cout << "The computation results are: " << endl;

    // Print out results
    for (int i = 0; i < extractedLines.size(); ++i)
    {
        cout << extractedLines[i] << endl;
    }

    cout << "The edge server has successfully finished receiving all computation results from the Backend-Server OR and Backend-Server AND." << endl;

    char returnBuf[MAXDATASIZE];

    // Print results, copy into a character array buffer and send back to client.cpp. Shut down edge server
    int currLineNumber = 0;
    int currBufIndex = 0;

    while (currLineNumber <= extractedLines.size())
    {
        strncpy(returnBuf + currBufIndex, extractedLines[currLineNumber].c_str(), extractedLines[currLineNumber].size());

        //update location of index in the buffer
        currBufIndex += extractedLines[currLineNumber].size();
        returnBuf[currBufIndex] = '\n';
        currBufIndex++;

        currLineNumber++;
    }

    memset(returnBuf, 0, sizeof(returnBuf));

    if (send(new_fd, returnBuf, strlen(returnBuf), 0) == -1)
    {
        perror("send tcp");
        exit(1);
    }

    cout << "The edge server has successfully finished sending all computation results to the client." << endl;
    close(new_fd);
    close(sockfd);

    return 0;
}

//  Open a UDP client to send the lines to the designated back-end server for computation
bool sendToBackEnd(char *buf, char *portnum)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo("127.0.0.1", portnum, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("talker: socket");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    if ((numbytes = sendto(sockfd, buf, strlen(buf), 0, p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }

    freeaddrinfo(servinfo);
    close(sockfd);

    return true;
}

// Open a UDP client to retrieve the computed results from the back-end server
string receiveFromBackEnd()
{

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, UDP_PORT_EDGE, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // Loop through all the results and bind to the first possible result
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("listener: socket");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "listener: failed to bind socket\n");
        exit(1);
    }

    freeaddrinfo(servinfo);

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                             (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }

    buf[numbytes] = '\0';

    string receiveBuf(buf);

    memset(buf, 0, sizeof(buf));
    close(sockfd);

    // Return the received input as a string
    return receiveBuf;
}