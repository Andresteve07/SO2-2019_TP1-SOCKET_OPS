/*
 * udp_socket_client.c
 *
 *  Created on: 1 abr. 2019
 *      Author: steve-urbit
 */

#include "socket_operation.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include "frozen.h"
#include "log.h"
#include <sys/time.h>
#include <errno.h>
  
#define PORT	12121 
#define MAXLINE 1024 

int udp_sockfd;
struct sockaddr_in udp_my_address, udp_target_address;

operation_result udp_init_client(){
	// Creating socket file descriptor 
	if ( (udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		log_error("socket creation failed"); 
		exit(EXIT_FAILURE); 
	} 

	memset(&udp_target_address, 0, sizeof(udp_target_address)); 
	
	// Filling server information 
	udp_target_address.sin_family = AF_INET; 
	udp_target_address.sin_port = htons(PORT); 
	udp_target_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	log_debug("UDP socket creation succeed!");
	return socket_success;
}
operation_result udp_init_server(){
	// Creating socket file descriptor 
    if ( (udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&udp_my_address, 0, sizeof(udp_my_address)); 
    memset(&udp_target_address, 0, sizeof(udp_target_address)); 
      
    // Filling server information 
    udp_my_address.sin_family    = AF_INET; // IPv4 
    udp_my_address.sin_addr.s_addr = INADDR_ANY; 
    udp_my_address.sin_port = htons(PORT); 
      
    // Bind the socket with the server address 
    if ( bind(udp_sockfd, (const struct sockaddr *)&udp_my_address,  
            sizeof(udp_my_address)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }
	return udp_timeouts(2);
}
operation_result udp_timeouts(int seconds){
	struct timeval timeout;      
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    if (setsockopt (udp_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
		log_error("setsockopt failed\n");
		return socket_failure;
	}
        
    if (setsockopt (udp_sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
		log_error("setsockopt failed\n");
		return socket_failure;
	}
    return socket_success;
}
operation_result udp_connect_to_server(char*  server_ip){
	return socket_success;
}

operation_result udp_connect(){
	operation_result op = socket_success;
		return op;
}

operation_result udp_send_data(){
	operation_result op = socket_success;
	return op;
}
operation_result udp_recv_data(){
	operation_result op = socket_success;
	return op;
}

operation_result udp_send_rpc(rpc* rpc_message){
	char total_buf[RPC_MSG_BUF_SIZE];
	bzero(total_buf,sizeof(total_buf));
	
	char* rpc_buf = &total_buf[4];
	struct json_out output = JSON_OUT_BUF(rpc_buf, RPC_MSG_BUF_SIZE-4);
	
	json_printf(&output, RPC_JSON_FMT,
	rpc_message->command_id,
	rpc_message->satellite_id,
	rpc_message->station_id,
	rpc_message->payload,
	rpc_message->error);
	log_trace("RPC req: %s\n",rpc_buf);

	set_payload_size(strlen(rpc_buf),total_buf);
	
	log_trace("TOTAL req: %c%c%c%c%s\n",total_buf[0],total_buf[1],total_buf[2],total_buf[3],&total_buf[4]);

	if(sendto(udp_sockfd, total_buf, strlen(rpc_buf)+4, 
	MSG_CONFIRM, (const struct sockaddr *) & udp_target_address, 
	sizeof(udp_target_address)) > 0){
		return socket_success;
	} else {
		return socket_failure;
	}
}

operation_result udp_recv_rpc(rpc* rpc_message){
	size_t payload_size;
	char input_buf[RPC_MSG_BUF_SIZE];//sizeof only works ok for static arrays i.e. results on 500
	bzero(input_buf, sizeof(input_buf));
	socklen_t address_size; 
	
	if(recvfrom(udp_sockfd, input_buf, sizeof(input_buf),
	MSG_WAITALL, (struct sockaddr *) &udp_target_address, 
	&address_size) > 0){
		int size_int = get_payload_size(input_buf);
		char* recv_data = &input_buf[4];
		recv_data[size_int]='\0';
		log_trace("size_cadena:%lu,cadena: %s\n",strlen(recv_data),recv_data);
		log_trace("HOLOOO");
		
		json_scanf(recv_data,strlen(recv_data),RPC_JSON_FMT,
		& rpc_message->command_id,
		& rpc_message->satellite_id,
		& rpc_message->station_id,
		& rpc_message->payload,
		& rpc_message->error);

		if (rpc_message->payload!=NULL){
			log_trace("payload_size: %lu, payload:%s", strlen(rpc_message->payload),rpc_message->payload);
		}

		if(rpc_message->error!=NULL){
			log_trace("error_size: %lu, error:%s", strlen(rpc_message->error),rpc_message->error);
		}

		log_trace("cid:%i,\nsatid:%i,\nstid:%i,\npay:%s,\nerror:%s",
		rpc_message->command_id,
		rpc_message->satellite_id,
		rpc_message->station_id,
		rpc_message->payload,
		rpc_message->error);
		
		return socket_success;

	} else {
		log_error("FAILED TO RECEIVE UDP RPC MSG! Errno: %s", strerror(errno));
		return socket_failure;
	}
}

