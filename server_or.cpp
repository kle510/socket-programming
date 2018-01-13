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
#include <arpa/inet.h>
#include <netdb.h>

#include <vector>
#include <iostream>
#include <sstream>
#include <string.h>
#include <signal.h>

#define UDP_PORT_OR "21971"
#define UDP_PORT_EDGE "24971"



#define MAXBUFLEN 100

using namespace std;



int main(void)
{
    bool serverloop = true;
    int andcounter = 0;
    cout << "The Server AND is up and running using UDP on port " << UDP_PORT_OR << endl;
    cout << "The Server AND start receiving lines from the edge server for AND computation." << endl;
    cout << "The computation results are:" <<endl ;

    /*
    The UDP server boots at startup and enters a while loop. At this point, the server is ready to receive the character array buffer from the edge server for computation. 
    */

    while (serverloop){


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

        if ((rv = getaddrinfo(NULL, UDP_PORT_OR, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
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
            return 2;
        }

        freeaddrinfo(servinfo);

        ////////////////////////////////////////////////////////////////

        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        /*
        THIS IS IMPORTANT.  buf[0] == '\n'

        When done sending, edge.cpp sends an unfilled array with only '\n' to the back end server. This signals the exit of the while loop and the shutdown of the server.

        */


        if (buf[0] == '\n'){
            serverloop=false;
            break;
        }

        buf[numbytes] = '\0';




        /*
        When the back-end finally receives the buffer, it makes an initial scan to determine the character count of the two binary numbers from the line.  The back-end server then creates two temporary character arrays to extract the two binary numbers from the line, basing the size of the array on the larger of the two binary numbers. The temporary character array pads the array with 0's for the binary number with the smaller character count.
        */


        int i = 0; //full array counter
        int numonesize = 0;
        int numtwosize = 0;


        //run through the array once and set the counters
        while (buf[i] != '\n'){ //while bit is not a comma

            while (buf[i] != 44){
                i++;

            } 

            //here, bit is comma. skip over it
            i++;


            while (buf[i] != 44){ //while bit is not a comma
                i++;
                numonesize++;       
            } 

            //here, bit is the second comma. skip over it
            i++;

            while (buf[i] != '\n'){ //while bit is the end
                i++;
                numtwosize++;

            } 


        } 


        //compare the sizes to make sure there is room for both arrays

        int arraysize;

        if (numonesize>numtwosize){
            arraysize = numonesize;
        }
        else if (numtwosize>numonesize){
            arraysize = numtwosize;
        }
        else{ //both numbers are equal
            arraysize = numonesize;
        }


        /*
        Computation is performed by comparing each individual character of the two temporary character arrays and outputting a resulting value into a paddedResult character array. The computation is different for the and and or servers. 

        */


        //initialize computational arrays for making comparisons.  Prepare them for the computational part. 
        char numonearray[arraysize];
        char numtwoarray[arraysize];





        //fill computational arrays and pad them with 0's. 
        //traverse the buffer indices and copy them over to the two arrays
        if (numonesize>numtwosize){

            int bufindex = 4; //start at 4, since we dont count the indexes "and,"


            //copy numonearray
            for (int j =0; j < arraysize; j++){
                numonearray[j] = buf[bufindex];
                bufindex++;
            }

            bufindex++; //skip one for the comma
            int diff = numonesize - numtwosize;

            //copy numtwoarray
            for (int j = 0; j < diff; j++){
                numtwoarray[j] = '0'; //or 48, which is ascii code for 0
            }

            for (int j = diff; j < arraysize; j++){
                numtwoarray[j] = buf[bufindex];
                bufindex++;
            }

        }
        else if (numtwosize>numonesize){

            int bufindex = 3; //start at 3, since we dont count the indexes "or,"
            int diff = numtwosize - numonesize;


            //copy numonearray
            for (int j = 0; j < diff; j++){
                numonearray[j] = '0'; //or 48, which is ascii code for 0
            }

            for (int j = diff; j < arraysize; j++){
                numonearray[j] = buf[bufindex];
                bufindex++;
            }

            bufindex++; //skip one for the comma

            //copy numtwoarray
            for (int j =0; j < arraysize; j++){
                numtwoarray[j] = buf[bufindex];
                bufindex++;
            }

        }
        else{ //both numbers are equal

            int bufindex = 3; //start at 3, since we dont count the indexes "or"

            for (int j =0; j < arraysize; j++){
                numonearray[j] = buf[bufindex];
                bufindex++;
            }

            bufindex++; //skip one for the comma

            for (int j =0; j < arraysize; j++){
                numtwoarray[j] = buf[bufindex];
                bufindex++;
            }

        }




        //make padded result array
        //0 is 48, and 1 is 49 in ascii
        char paddedarray[arraysize];

        for (int i=0; i<arraysize; i++){


            if(numonearray[i] == 48 && numtwoarray[i] == 48){ //both results are 0
                paddedarray[i] = '0';
            }
            else if(numonearray[i] == 48 && numtwoarray[i] == 49){ //array 1 gives 0, array 2 gives 1
                paddedarray[i] = '1';
            }
            else if(numonearray[i] == 49 && numtwoarray[i] == 48){ //array 1 gives 1, array 2 gives 0
                paddedarray[i] = '1';
            }
            else if(numonearray[i] == 49 && numtwoarray[i] == 49){ // both arrays give 1
                paddedarray[i] = '1';
            }


        }


        /*
        The paddedResult character array is reduced to a finalarray character array by extracting the leading 0's.
        */


        //check for front zeros
        int frontzeros = 0;
        int counter = 0 ;
        while (paddedarray[counter] == 48){
            frontzeros++;
            counter++;
        }

        //finalize array (+1 to pad the array at the end)
        int finalarraysize = arraysize - frontzeros + 1;


        char finalarray[finalarraysize];

        for(int i=0; i<finalarraysize-1; i++){

            finalarray[i] = paddedarray[i+frontzeros] ;

        }

        finalarray[finalarraysize]='\n';


        //test to see if arrays arrived okay
        for(int i=0; i<finalarraysize; i++){
            cout << finalarray[i] ;
        }
        cout << endl;


        //create socket to send back to front end server

        /*
         This character array is finally sent back to the edge server's receiveFromBackEnd() method when the back-end server creates a UDP client and sends it over.
        */


        int sockfd_out;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

        if ((rv = getaddrinfo("localhost", UDP_PORT_EDGE, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        // loop through all the results and make a socket
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd_out = socket(p->ai_family, p->ai_socktype,
                                     p->ai_protocol)) == -1) {
                perror("talker: socket");
                continue;
            }

            break;
        }

        if (p == NULL) {
            fprintf(stderr, "talker: failed to create socket\n");
            return 2;
        }

        // Sends the result to the AWS server and checks if an error occurred

        if ((numbytes = sendto(sockfd_out, finalarray, sizeof(finalarray), 0,
                               p->ai_addr, p->ai_addrlen)) == -1) {
            perror("talker: sendto");
            exit(1);
        }


        /*
        The while loop then repeats -- the back-end closes the current UDP server, creates a new UDP server, and waits for the next reception.
        */

        andcounter++;

        close(sockfd);
        close(sockfd_out);

    }

    cout << "The Server AND has successfully received " << andcounter << " lines from the edge server and finished all AND computations." <<endl ;
    cout << "The Server AND has successfully finished sending all computation results to the edge server" <<endl ;



    return 0;
}