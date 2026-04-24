#pragma once

#include <stdio.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32")

#include <time.h>
#include <string.h>
#include <iostream>


void ntpdate() {
	//char *hostname=(char *)"163.117.202.33";
	//char *hostname=(char *)"pool.ntp.br";
	char    *hostname = (char *)"203.254.163.75";
	int portno = 123;     //NTP is port 123
	int maxlen = 1024;        //check our buffers
	int i;          // misc var i
	char msg[48] = { 010,0,0,0,0,0,0,0,0 };    // the packet we send
	char  buf[1024]; // the buffer we get back
	//struct in_addr ipaddr;        //  
	struct protoent *proto;     //
	struct sockaddr_in server_addr;
	int s;  // socket
	time_t tmit;   // the time -- This is a time_t sort of

	//use Socket;
	//
	//#we use the system call to open a UDP socket
	//socket(SOCKET, PF_INET, SOCK_DGRAM, getprotobyname("udp")) or die "socket: $!";
	//proto = getprotobyname("udp");
	
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);// proto->p_proto);
	//perror("socket");
	//
	//#convert hostname to ipaddress if needed
	//$ipaddr   = inet_aton($HOSTNAME);
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(hostname);
	//argv[1] );
	//i   = inet_aton(hostname,&server_addr.sin_addr);
	server_addr.sin_port = htons(portno);
	//printf("ipaddr (in hex): %x\n",server_addr.sin_addr);

	/*
	 * build a message.  Our message is all zeros except for a one in the
	 * protocol version field
	 * msg[] in binary is 00 001 000 00000000
	 * it should be a total of 48 bytes long
	*/

	// send the data
	printf("sending data..\n");
	i = sendto(s, msg, sizeof(msg), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
	//perror("sendto");
	// get the data back
	struct sockaddr saddr;
	int saddr_l = sizeof(saddr);
	i = recvfrom(s, buf, 48, 0, &saddr, &saddr_l);
	//perror("recvfr:");

	//We get 12 long words back in Network order
	/*
	for(i=0;i<12;i++) {
		//printf("%d\t%-8x\n",i,ntohl(buf[i]));
		long tmit2=ntohl((time_t)buf[i]);
		std::cout << "Round number " << i << " time is " << ctime(&tmit2)  << std::endl;
	}
	*/
	/*
	 * The high word of transmit time is the 10th word we get back
	 * tmit is the time in seconds not accounting for network delays which
	 * should be way less than a second if this is a local NTP server
	 */

	 tmit=ntohl((time_t)buf[10]);    //# get transmit time
	//tmit = ntohl((time_t)buf[4]);    //# get transmit time
	//printf("tmit=%d\n",tmit);

	/*
	 * Convert time to unix standard time NTP is number of seconds since 0000
	 * UT on 1 January 1900 unix time is seconds since 0000 UT on 1 January
	 * 1970 There has been a trend to add a 2 leap seconds every 3 years.
	 * Leap seconds are only an issue the last second of the month in June and
	 * December if you don't try to set the clock then it can be ignored but
	 * this is importaint to people who coordinate times with GPS clock sources.
	 */

	tmit -= 2208988800U;
	//printf("tmit=%d\n",tmit);
	/* use unix library function to show me the local time (it takes care
	 * of timezone issues for both north and south of the equator and places
	 * that do Summer time/ Daylight savings time.
	 */


	 //#compare to system time
	 //printf("Time: %s",ctime(&tmit));

	std::cout << "time is " << ctime(&tmit) << std::endl;
	i = time(0);
	//printf("%d-%d=%d\n",i,tmit,i-tmit);
	//printf("System time is %d seconds off\n",(i-tmit));
	std::cout << "System time is " << (i - tmit) << " seconds off" << std::endl;

	iResult = closesocket(s);
	WSACleanup();
}