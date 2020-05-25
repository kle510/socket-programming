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

#define UDP_PORT_AND "22971"
#define UDP_PORT_EDGE "24971"
#define MAXBUFLEN 100

using namespace std;

int main(void)
{
    cout << "The Server AND is up and running using UDP on port " << UDP_PORT_AND << endl;
    cout << "The Server AND start receiving lines from the edge server for AND computation." << endl;
    cout << "The computation results are:" << endl;

    // The UDP server boots at startup and enters a while loop.
    // At this point, the server is ready to receive the character array buffer from the edge server for computation.

    bool serverLoop = true;
    int andCounter = 0;
    while (serverLoop)
    {
        // Establish UDP client socket
        int sockfd;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        int numbytes;
        struct sockaddr_storage their_addr;
        char buf[MAXBUFLEN];
        socklen_t addr_len;
        char s[INET6_ADDRSTRLEN];

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;

        if ((rv = getaddrinfo(NULL, UDP_PORT_AND, &hints, &servinfo)) != 0)
        {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        // Loop through all the results and bind to the first we can
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
            return 2;
        }

        freeaddrinfo(servinfo);

        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        // buf[0] == '\n'
        // When done sending, edge.cpp sends an unfilled array slot with only '\n' to the back end server.
        // This signals the exit of the while loop and the shutdown of the server.
        if (buf[0] == '\n')
        {
            serverLoop = false;
            break;
        }

        buf[numbytes] = '\0';

        // When the back-end finally receives the buffer,
        // it makes an initial scan to determine the character count of the two binary numbers from the line.
        // The back-end server then creates two temporary character arrays to extract the two binary numbers from the line,
        // basing the size of the array on the larger of the two binary numbers.
        // The temporary character array pads the array with 0's for the binary number with the smaller character count.

        int curIndex = 0;
        int numOneSize = 0;
        int numTwoSize = 0;

        // Run through the array once and set the counters.
        while (buf[curIndex] != '\n')
        { 
            // While bit is not a comma, increment full array index past the word "and/or"
            while (buf[curIndex] != 44)
            {
                curIndex++;
            }

            // Here, bit is comma. skip over it.
            curIndex++;

            // While bit is not a comma, track the size of the first number.
            while (buf[curIndex] != 44)
            {
                curIndex++;
                numOneSize++;
            }

            // Here, bit is the second comma. skip over it.
            curIndex++;

            // While bit is not the end, track the size of the second number.
            while (buf[curIndex] != '\n')
            { 
                curIndex++;
                numTwoSize++;
            }
        }

        // Compare the sizes to make sure there is room for both numbers in the computational array.
        int arraySize;
        if (numOneSize >= numTwoSize)
        {
            arraySize = numOneSize;
        }
        else
        {
            arraySize = numTwoSize;
        }

        // Computation is performed by comparing each individual character of the two temporary character arrays
        // and outputting a resulting value into a paddedResult character array.
        // The computation is different for the and and or servers.

        // Initialize computational arrays for making comparisons.  Prepare them for the computational part.
        char numOneArray[arraySize];
        char numTwoArray[arraySize];

        // Fill the computational arrays and pad them with 0's.
        // Traverse the buffer indices and copy them over to the two arrays
        if (numOneSize > numTwoSize)
        {
            int bufIndex = 4; //start at 4, since we dont count the indexes in the string "and,"

            // Copy numOneArray
            for (int j = 0; j < arraySize; j++)
            {
                numOneArray[j] = buf[bufIndex];
                bufIndex++;
            }

            bufIndex++; // Skip one for the comma

            int diff = numOneSize - numTwoSize;

            // Copy numTwoArray
            for (int j = 0; j < diff; j++)
            {
                numTwoArray[j] = '0';
            }

            for (int j = diff; j < arraySize; j++)
            {
                numTwoArray[j] = buf[bufIndex];
                bufIndex++;
            }
        }
        else if (numTwoSize > numOneSize)
        {
            int bufIndex = 4; //start at 4, since we dont count the indexes in the string "and,"

            int diff = numTwoSize - numOneSize;

            // Copy numOneArray
            for (int j = 0; j < diff; j++)
            {
                numOneArray[j] = '0';
            }

            for (int j = diff; j < arraySize; j++)
            {
                numOneArray[j] = buf[bufIndex];
                bufIndex++;
            }

            bufIndex++; // skip one for the comma

            // Copy numTwoArray
            for (int j = 0; j < arraySize; j++)
            {
                numTwoArray[j] = buf[bufIndex];
                bufIndex++;
            }
        }
        else
        {
            int bufIndex = 4; //start at 4, since we dont count the indexes in the string "and,"

            for (int j = 0; j < arraySize; j++)
            {
                numOneArray[j] = buf[bufIndex];
                bufIndex++;
            }

            bufIndex++; // skip one for the comma

            for (int j = 0; j < arraySize; j++)
            {
                numTwoArray[j] = buf[bufIndex];
                bufIndex++;
            }
        }

        // Use the two computational arrays to compute the result and make padded result array
        // 0 is 48, and 1 is 49 in ascii
        char paddedArray[arraySize];

        for (int i = 0; i < arraySize; i++)
        {
            // Both arrays give 0
            if (numOneArray[i] == 48 && numTwoArray[i] == 48)
            { 
                paddedArray[i] = '0';
            }
            // First array gives 0, second array gives 1
            else if (numOneArray[i] == 48 && numTwoArray[i] == 49)
            {
                paddedArray[i] = '0';
            }
            // First array gives 1, second array gives 0
            else if (numOneArray[i] == 49 && numTwoArray[i] == 48)
            {
                paddedArray[i] = '0';
            }
            // Both arrays give 1
            else if (numOneArray[i] == 49 && numTwoArray[i] == 49)
            {
                paddedArray[i] = '1';
            }
        }

        // The paddedResult character array is reduced to a finalArray character array by extracting the leading 0's.

        // Check for leading zeros
        int leadingZeros = 0;
        curIndex = 0;
        while (paddedArray[curIndex] == 48)
        {
            leadingZeros++;
            curIndex++;
        }

        // Construct final array (+1 to pad the array at the end)
        int finalArraySize = arraySize - leadingZeros + 1;

        char finalArray[finalArraySize];

        for (int i = 0; i < finalArraySize - 1; i++)
        {
            finalArray[i] = paddedArray[i + leadingZeros];
        }

        finalArray[finalArraySize] = '\n';

        // Print out computational results
        for (int i = 0; i < finalArraySize; i++)
        {
            cout << finalArray[i];
        }
        cout << endl;

        // This character array is finally sent back to the edge server's receiveFromBackEnd() method 
        // when the back-end server creates a UDP client and sends it over. 

        int sockfd_out;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

        if ((rv = getaddrinfo("localhost", UDP_PORT_EDGE, &hints, &servinfo)) != 0)
        {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        // Loop through all the results and make a socket
        for (p = servinfo; p != NULL; p = p->ai_next)
        {
            if ((sockfd_out = socket(p->ai_family, p->ai_socktype,
                                     p->ai_protocol)) == -1)
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

        // Send the result to the edge server
        if ((numbytes = sendto(sockfd_out, finalArray, sizeof(finalArray), 0,
                               p->ai_addr, p->ai_addrlen)) == -1)
        {
            perror("talker: sendto");
            exit(1);
        }

        // The while loop then repeats --
        // the back-end closes the current UDP server, creates a new UDP server, and waits for the next reception.
        andCounter++;
        close(sockfd);
        close(sockfd_out);
    }

    cout << "The Server AND has successfully received " << andCounter << " lines from the edge server and finished all AND computations." << endl;
    cout << "The Server AND has successfully finished sending all computation results to the edge server" << endl;

    return 0;
}