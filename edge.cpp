// Kevin L
// https://github.com/kle510
// March 2016


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


#define BACKLOG 10  //typical backlog size
#define MAXDATASIZE 1024
#define MAXBUFLEN 100


 
 
 /*
 sendToBackEnd() -  opens a UDP client to send the line to the designated back-end server for computation
 
 return type is boolean
 */


bool sendToBackEnd(char*  buf, char* portnum){
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;


    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo("127.0.0.1", portnum, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    if ((numbytes = sendto(sockfd, buf, strlen(buf), 0, p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }



    freeaddrinfo(servinfo);

    close(sockfd);

    return true;

}

/*
receiveFromBackEnd() -  opens a UDP server to retrieve back the result from back end server .

return type is string
*/


string receiveFromBackEnd(){

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

    if ((rv = getaddrinfo(NULL, UDP_PORT_EDGE, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        exit(1);
    }

    freeaddrinfo(servinfo);


    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                             (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    buf[numbytes] = '\0';


    //return the received input as a string
    string receivebuf(buf);


    memset (buf,0,sizeof(buf));


    close(sockfd);

    return receivebuf;

}

/*
    main method
*/

int main(void)
{

    /*
    At startup, edge.cpp starts a server-side TCP connection and starts listening for clients. This code is a slightly modifued version of Beej's Network Tutorial. The  majority of the code was kept the same.
    */
    
    int sockfd;

    int new_fd;  // listen on sock_fd, new connection on new_fd

    char edgebufin[MAXDATASIZE];

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


    if ((rv = getaddrinfo(NULL, TCP_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // Here, servinfo now points to a linked list of 1 or more struct addrinfos.

    /*

        Here, the server loops through all the struct addrinfos, and creates a socket and binds it to the server's address for the first availiable one.
        
    Unlike client.cpp,  I  kept this for loop the same as Beej's. I would get segmentation -- core dumped when I try to remove the for loop and operate on servinfo because of memory access issues
    
    */
    
    for (p = servinfo; p != NULL; p = p->ai_next) {


        //if you cant make the current socket, print error and go onto the next one in the list
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            cout << "ERROR creating socket on server" <<endl;
            continue;
        }

        //If after making socket, you try to set socket options and it fails, print error and quit
        //The purpose of setsockopt is to control socket behavior. 
        //In this case, we set it so bind can reuse local addresses. 
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            cout << "ERROR, setting socket options failed. Exit program. " <<endl;
            exit(1);
        }


        //bind is used to bind the socket to the server's address
        //if you make a socket and you cant bind it to the client, skip it, print error, and make socket for next one
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            cout << "ERROR, binding error on current socket" <<endl;
            continue;
        }

        break;
    }

    //If a socket can't bind  to the server, print error and end
    if (p == NULL) {
        cout << "ERROR, unable to bind a socket to the server. Exit program. " <<endl;
        exit(1);
    }


    freeaddrinfo(servinfo); 


    // listen - listen for socket connections and limit the queue of incoming connections
    //if error on listen, print error and end
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }


    /*
    The while loop that keeps the edge server running on Beej's tutorial, as well as the fork() distinction, was removed as our program is a one-to-one connection with only one client. Thus, every time a TCP connection with the client is finished, we must reboot the server again from the start. 
    */
    
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        exit(1);
    }

    if (send(new_fd, "Hello, world!", 13, 0) == -1){
        perror("send");
    }

    //recv(int s, void *buf, size_t len, int flags);
    if (recv(new_fd, edgebufin, MAXDATASIZE-1, 0) == -1) {
        perror("recv");
        exit(1);
    }


    cout << "The edge server has received " << edgebufin[0] <<" lines from the client using TCP over port " TCP_PORT << endl;



    //make new char array to get rid of element number
    char edgebuf[MAXDATASIZE];

    for(int i=0; i<MAXDATASIZE; ++i){
        edgebuf[i]=edgebufin[i+2]; 
    }


    /*
    When the edge server receives and accepts  a connection from the client, it runs the UDPcircuit while loop. The UDP circuit while loop separates the lines from the character buffer received from the client into and/or based on the ASCII distinction of the character. From there, it sends the line to the sendToBackEnd() method, which opens a UDP client to send the line to the designated back-end server for computation. THe UDP circuit while loop then runs the receiveFromBackEnd() method to open a UDP server to take back the result of the client and stuff it into a vector of strings containing the results .
    */
    


    bool UDPcircuit = true;
    int i = 0; //UDP circuit counter
    int andcounter = 0;
    int orcounter = 0;


    vector<string> extractedLines;
    int lineCount;

    while (UDPcircuit){


        if (edgebuf[i] == 97) { //97 ASCII character corresponds to letter a

            char andbuf[MAXDATASIZE];
            int j = 0; // andbuf counter
            char* portnum = UDP_PORT_AND;


            while (edgebuf[i] != '\n'){  


                andbuf[j] = edgebuf[i];

                i++;
                j++;
            } 

            andbuf[j] = '\n'; //pad the end so we have a stopping point in the back-end server

            andcounter++;
            sendToBackEnd(andbuf, portnum);
            i++; //advance the counter past the space

            string receivebuf = receiveFromBackEnd();



            extractedLines.push_back(receivebuf);
            lineCount++;

        }
        else if (edgebuf[i] == 111){ // 111 ASCII character corresponds to letter o
            char orbuf[MAXDATASIZE];
            int j = 0; // andbuf counter
            char* portnum = UDP_PORT_OR;

            while (edgebuf[i] != '\n'){  

                orbuf[j] = edgebuf[i];

                i++;
                j++;
            } 

            orbuf[j] = '\n'; //pad the end so we have a stopping point in the back-end server

            orcounter++;
            sendToBackEnd(orbuf, portnum);
            i++; //advance the counter past the space


            string receivebuf = receiveFromBackEnd();



            extractedLines.push_back(receivebuf);
            lineCount++;

        }
        else{  //create a distinction to terminate the servers now that we're done

            //Send signal to terminate and server
            char andbuf[MAXDATASIZE];
            andbuf[0] = '\n'; 

            sendToBackEnd(andbuf, UDP_PORT_AND);

            //Send signal to terminate or server
            char orbuf[MAXDATASIZE];
            orbuf[0] = '\n'; 


            sendToBackEnd(orbuf, UDP_PORT_OR);


            //exit out of while loop
            UDPcircuit = false;
        }




    }


    cout << "The edge server has successfully sent " << andcounter << " lines to Backend-Server OR" << endl;
    cout << "The edge server has successfully sent " <<  orcounter << " lines to Backend-Server AND" << endl;
    cout << "The edge server start receiving the computation results from Backend-Server OR and Backend-Server AND using UDP port " <<  UDP_PORT_EDGE << endl;

    cout << "The computation results are: " << endl;
    
    //print  out results 
    for(int i=0; i<extractedLines.size(); ++i){
        cout << extractedLines[i] << endl;

    }

    cout << "The edge server has successfully finished receiving all computation results from the Backend-Server OR and Backend-Server AND." << endl;


    char returnbuf[MAXDATASIZE];

    /*
    Similarly to client.cpp, the results were printed and  the vector were then copied iteratively into a character array buffer and sent back to client.cpp. The edge server shuts down.
    */
    
    int lineNumber = 0;


    //current index of buf that we're on. want this to be less than MAXDATASIZE
    int bufferIndex = 0;


    memset(returnbuf, 0, sizeof(returnbuf));


    while (lineNumber != lineCount) {


        strncpy(returnbuf + bufferIndex, extractedLines[lineNumber].c_str(), extractedLines[lineNumber].size());


        //update location of index in the buffer
        bufferIndex += extractedLines[lineNumber].size();

        // line break \n between the lines 
        returnbuf[bufferIndex] = '\n';
        bufferIndex++;

        //go to the next line in text
        lineNumber++;

    }

    if (send(new_fd, returnbuf, strlen(returnbuf), 0) == -1) {
        perror("send tcp");
        exit(1);
    }


    cout << "The edge server has successfully finished sending all computation results to the client." << endl;


    close(new_fd);

    close(sockfd);

    return 0;

}
