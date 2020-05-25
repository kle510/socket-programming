# Socket Programming


## Foreward
The program follows the format as specified by the project instruction. The servers boot up preceding the client. The client is run by the command ./client <filename>, where file name is an input text file (command line argument) containing a list of commands separated by commas in the format of "decision,number1,number2", where decision is "and" or "or", the first binary number, and the second binary number. The client sends the lines of the text file to the edge server. The edge server makes a determination on each line iteratively on whether or not it is an "and" or  "or" command, and sends it to the server_and and server_or back-end servers. The backend servers perform the computations, forwards the computational result back to the edge server, and shuts down. The edge server prints the collective results, aggregates them, and sends them back to the client, and shuts down. The client, prints out the results, and shuts down. 

NOTE: modified versions of Beej's Networking Guide were used to start and close the TCP and UDP sockets. All processing code was self-developed, with the help of Beej's C  Guide and cplusplus.com

## client.cpp
The client runs ./client and the <filename>, where the file name is a txt file following the same format as specified in the project guidelines. The program has NOT been tested with different file formats.  

In accordance with Beej's Networking Guide, the client boots creates a socket, boots up the TCP socket, and attempts to connect to the server sharing the same port number. 

Two techniques were considered when implementing the TCP connection part: getaddrinfo() from Beej's code, and gethostbyname() from the TCP socket tutorial on linuxhowto.com. Both methods were tested but ultimately, the Beej's code was chosen to be implemented since there is much more functionality options with getaddrinfo (see p.16 of Beej's Network Tutorial)

This code was slightly modified from the Beej's version of the TCP client. Because our code is a one-to-one, one time connection, there is no need to cycle through all avaialble clients -- so the for loop to create and connect the socket was taken out. 

A series of errors are checked in TCP connections. These were kept mostly similar to Beej's, except changed so that the program automatically terminated should an error occur. 

If the connection is successful, we can proceed. 

The client then uses an ifstream to open and extract the lines of the file into a vector of strings. The lineCount was incremented for each line added to the vector.  The strings in the vector were then copied iteratively into a character array buffer and sent to edge.cpp.

The client then waits for a response from edge.cpp, prints out the resulting computation from the buffer received from edge.cpp, and shuts down.

## edge.cpp
At startup, edge.cpp starts a server-side TCP connection and starts listening for clients. This code is a slightly modifued version of Beej's Network Tutorial. The  majority of the code was kept the same.

The while loop that keeps the edge server running on Beej's tutorial, as well as the fork() distinction, was removed as our program is a one-to-one connection with only one client. Thus, every time a TCP connection with the client is finished, we must reboot the server again from the start. 

(Intuitively, this defeats the purpose of having an always-on server, but for the case of this project and a one-to-one client-server connection this is the way it works. )

When the edge server receives and accepts  a connection from the client, it runs the UDPcircuit while loop. The UDP circuit while loop separates the lines from the character buffer received from the client into and/or based on the ASCII distinction of the character. From there, it sends the line to the sendToBackEnd() method, which opens a UDP client to send the line to the designated back-end server for computation. THe UDP circuit while loop then runs the receiveFromBackEnd() method to open a UDP server to take back the result of the client and stuff it into a vector of strings containing the results .

Similarly to client.cpp, the results were printed and  the vector were then copied iteratively into a character array buffer and sent back to client.cpp. The edge server shuts down.

## server_and.cpp and server_or.cpp

Both of the back-end servers work in a similar fashion, with minor differences in the computational step.

The UDP server boots at startup and enters a while loop. At this point, the server is ready to receive the character array buffer from the edge server for computation. 

When the back-end finally receives the buffer, it makes an initial scan to determine the character count of the two binary numbers from the line.  The back-end server then creates two temporary character arrays to extract the two binary numbers from the line, basing the size of the array on the larger of the two binary numbers. The temporary character array pads the array with 0's for the binary number with the smaller character count.

Computation is performed by comparing each individual character of the two temporary character arrays and outputting a resulting value into a paddedResult character array. The computation is different for the and and or servers. 

The paddedResult character array is reduced to a finalarray character array by extracting the leading 0's. This character array is finally sent back to the edge server's receiveFromBackEnd() method when the back-end server creates a UDP client and sends it over. The while loop then repeats -- the back-end closes the current UDP server, creates a new UDP server, and waits for the next reception.

When the back-end server receives an indication that there are no more lines to be sent, the server shuts down.

All codes for establishing the UDP client and server were taken from Beej's Networking Tutorial, and kept as is.


## Notes about with the project

*All 3 servers (edge.cpp, server_and.cpp, and sever_or.cpp) must be rebooted every time we want to run the client program. 
*The servers must be booted first before running the client.
*The client is run via ./client <filename>, where file name is an input text file (command line argument) . 
*The program has not yet been tested with files of different file extensions besides .txt. It also has not been tested with files that do not follow the format of --   "decision,number1,number2", where decision is "and" or "or", the first binary number, and the second binary number, then a line break
*The code has not yet been tested with files of extremely large numbers past the limit allocated by the buffer arrays.
*For the computed result, only the result is printed (on all files )


## Reused Code

Beej's Networking Tutorial
Modified version of Beej's code were used for the TCP client on client.cpp and TCP server on edge.cpp. For the TCP client --  because our code is a one-to-one, one time connection, there is no need to cycle through all avaialble clients -- so the for loop to create and connect the socket was taken out.  For the TCP server -- the while loop that keeps the edge server running on Beej's tutorial, as well as the fork() distinction, was removed as our program is a one-to-one connection with only one client. Thus, every time a TCP connection with the client is finished, we must reboot the server again from the start. 

The UDP server and client codes on edge.cpp, server_or.cpp, and server_and.cpp were kept as is.

http://www.linuxhowtos.org/C_C++/socket.htm
The idea of implementing the TCP and UDP connection process from this tutorial was considered. However, the gethostbyname() function used by this website had less functionality then the getaddressinfo() method used by Beej's so Beej's implementation was ultimately chosen. However, the tutorial provided by the Linux How To website was tested, and provided further insight onto the way TCP and UDP connections worked, which helped my understanding a lot :)


Other code commands, functions, and libraries were used with the guidance of cplusplus.com and Beej's C Programming Tutorial.





