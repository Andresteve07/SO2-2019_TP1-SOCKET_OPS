/*
 * tcp_socket_client.c
 *
 *  Created on: 1 abr. 2019
 *      Author: steve-urbit
 */
#include "socket_operation.h"
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <strings.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "frozen.h"
#include "log.h"
#include <sys/time.h>
#include <errno.h>
 
#define PORT 12121 
#define SA struct sockaddr

int tcp_sockfd, connfd, client_address_len;
struct sockaddr_in tcp_servaddr, tcp_client_address;


operation_result tcp_init_client(){
    // socket create and varification 
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (tcp_sockfd == -1) { 
        log_error("socket creation failed...\n"); 
        return socket_failure; 
    } 
    else {
		log_debug("Socket successfully created..\n");
		return tcp_timeouts(10);
	}
}

operation_result tcp_init_server(){
	// socket create and verification 
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (tcp_sockfd == -1) { 
        printf("socket creation failed...\n");
        return socket_failure;
    } else {
		printf("Socket successfully created..\n"); 
	}
        
    bzero(&tcp_servaddr, sizeof(tcp_servaddr)); 
  
    // assign IP, PORT 
    tcp_servaddr.sin_family = AF_INET; 
    tcp_servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    tcp_servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(tcp_sockfd, (SA*)&tcp_servaddr, sizeof(tcp_servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        return socket_failure;
    } else {
		printf("Socket successfully binded..\n"); 
	}
    // Now server is ready to listen and verification 
    if ((listen(tcp_sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        return socket_failure;
    } else {
		printf("Server listening..\n"); 
	}
    client_address_len = sizeof(tcp_client_address); 
    // Accept the data packet from client and verification 
    connfd = accept(tcp_sockfd, (SA*)&tcp_client_address, &client_address_len);
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        return socket_failure;
    } else {
		printf("server acccept the client...\n");
	}
	return tcp_timeouts(2);
}

operation_result tcp_timeouts(int seconds){
	struct timeval timeout;      
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    if (setsockopt (connfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
		log_error("setsockopt failed\n");
		return socket_failure;
	}
        
    if (setsockopt (connfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
		log_error("setsockopt failed\n");
		return socket_failure;
	}
    return socket_success;
}

operation_result tcp_connect_to_server(char*  server_ip){
	bzero(&tcp_servaddr, sizeof(tcp_servaddr)); 
	// assign IP, PORT 
    tcp_servaddr.sin_family = AF_INET; 
    tcp_servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    tcp_servaddr.sin_port = htons(PORT); 
  
    // connect the client socket to server socket 
    if (connect(tcp_sockfd, (SA*)&tcp_servaddr, sizeof(tcp_servaddr)) != 0) { 
        log_error("connection with the server failed...\n"); 
        return socket_failure; 
    } else {
		log_debug("connected to the server..\n");
		connfd = tcp_sockfd;
		return socket_success;
	}
}

operation_result tcp_send_data(char* data_buffer){
	if(write(connfd, data_buffer, sizeof(data_buffer)) > 0){
		return socket_success;
	} else {
		return socket_failure;
	}
}

operation_result tcp_send_data_bytes(char* data_buffer, size_t byte_count){
	if(write(connfd, data_buffer, byte_count) > 0){
		return socket_success;
	} else {
		return socket_failure;
	}
}

operation_result tcp_recv_data_bytes(char* data_buffer, size_t byte_count){
	if(read(connfd, data_buffer, byte_count) > 0){
	return socket_success;
	} else {
		return socket_failure;
	}
}

operation_result tcp_recv_data(char* data_buffer){
	if(read(connfd, data_buffer, sizeof(data_buffer)) > 0){
	return socket_success;
	} else {
		return socket_failure;
	}
}

/**
 * @brief Sets the payload size in the first 4 bytes of the whole buffer
 * LSB go first
 * 
 * @param payload_size 
 * @param whole_buffer 
 */
void set_payload_size(int payload_size,char* whole_buffer){
	//LSB first
	log_trace("payload_size: %i ", payload_size);
	whole_buffer[0] = (unsigned char) (payload_size & 0xFF);
	whole_buffer[1] = (unsigned char) ((payload_size>>8) & 0xFF);
	whole_buffer[2] = (unsigned char) ((payload_size>>16) & 0xFF);
	whole_buffer[3] = (unsigned char) ((payload_size>>24) & 0xFF);
	log_trace("bytes:%i,%i,%i,%i\n",whole_buffer[0],whole_buffer[1],whole_buffer[2],whole_buffer[3]);
}

operation_result tcp_send_rpc(rpc* rpc_message){
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
	
	log_trace("TOTAL req: 0x%X,0x%X,0x%X,0x%X::%s\n",total_buf[0],total_buf[1],total_buf[2],total_buf[3],&total_buf[4]);

	if(write(connfd, total_buf, strlen(rpc_buf)+4) > 0){
		return socket_success;
	} else {
		return socket_failure;
	}
	
}
/**
 * @brief Gets the payload size from the first 4 bytes of the input buffer.
 * 
 * @param whole_buffer 
 * @return int 
 */
int get_payload_size(char* whole_buffer){
	int payload_size = 0;
	payload_size = (int)((unsigned char)whole_buffer[3]);
	payload_size = (payload_size<<8) + (unsigned char) whole_buffer[2];
	payload_size = (payload_size<<8) + (unsigned char) whole_buffer[1];
	payload_size = (payload_size<<8) + (unsigned char) whole_buffer[0];
	log_trace("payload_size:%i",payload_size);
	return payload_size;

}

operation_result tcp_recv_rpc(rpc* rpc_message){
	size_t payload_size;
	char input_buf[RPC_MSG_BUF_SIZE];//sizeof only works ok for static arrays i.e. results on 500
	bzero(input_buf, sizeof(input_buf));
	int bytes_recv = 0;
	bytes_recv = read(connfd, input_buf, sizeof(input_buf));
	log_trace("BYTES READ: %i", bytes_recv);
	log_trace("INPUT BUF: 0x%X,0x%X,0x%X,0x%X. EXTRA 0x%X.",input_buf[0],input_buf[1],input_buf[2],input_buf[3],input_buf[4]);
	if(bytes_recv > 0){
		int size_int = get_payload_size(input_buf);
		char* recv_data = &input_buf[4];
		recv_data[size_int]='\0';
		log_trace("size_cadena:%lu,cadena: %s\n",strlen(recv_data),recv_data);
		
		int scan_fields = json_scanf(recv_data,strlen(recv_data),RPC_JSON_FMT,
		& rpc_message->command_id,
		& rpc_message->satellite_id,
		& rpc_message->station_id,
		& rpc_message->payload,
		& rpc_message->error);
		if(scan_fields<=0){
			log_error("Could not decode any field of payload json. FIELD_COUNT:%i",scan_fields);
			return socket_failure;
		}
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
		log_error("FAILED TO RECEIVE RPC MSG! Errno: %s", strerror(errno));
		return socket_failure;
	}
}
#define nofile "File Not Found!"
// funtion sending file
int load_file_buffer(FILE* fp, char* buf, int s) 
{ 
    int i, len; 
    if (fp == NULL) { 
        strcpy(buf, nofile); 
        len = strlen(nofile); 
        buf[len] = EOF;
        return 1; 
    } 
  
    unsigned char ch; 
    for (i = 0; i < s; i++) {
		if (feof(fp)) 
            return 1;
        ch = fgetc(fp);//TODO replace with 'fread'
		//fread(&ch,1,1,fp);
		/*
		if (ch == EOF) {
			return 1;
		}
		*/
        buf[i] = ch;        
    } 
    return 0;
}

operation_result tcp_send_file(char* file_name){
	FILE *file_ptr;
	file_ptr=fopen(file_name,"rb");
	char file_buffer[FILE_CHUNK_BUF_SIZE]; 
	long sended_bytes = 0;
	int write_bytes;
	while (1) {
		// process 
		write_bytes = 0;
		if (load_file_buffer(file_ptr, file_buffer, FILE_CHUNK_BUF_SIZE)) {
			write_bytes = write(connfd, file_buffer, strlen(file_buffer));
			if( write_bytes > 0){
				break;
			} else {
				log_error("Failure to write %i bytes on file transfer.",strlen(file_buffer));
				if (file_ptr != NULL){
					fclose(file_ptr);
				}
				return socket_failure;
			}
		}
		// send
		write_bytes = write(connfd, file_buffer, FILE_CHUNK_BUF_SIZE);
		if( write_bytes <= 0){
			log_error("Failure to write %i bytes on file transfer.",strlen(file_buffer));
			if (file_ptr != NULL){
				fclose(file_ptr);
			}
			return socket_failure;
		}
		sended_bytes += write_bytes;
		log_trace("FILE_SEND - %lu",sended_bytes);
		bzero(file_buffer,FILE_CHUNK_BUF_SIZE);
	}
	if (file_ptr != NULL){
		fclose(file_ptr);
	}
	return socket_success;
}

int scan_input_buf_for_EOF(char* buf, int s) { 
    int i;
    char ch;
    for (i = 0; i < s; i++) {
        ch = buf[i];
        if (ch == EOF){
			if (i+1 < strlen(buf)) {
				buf[i+1] = '\0';
			}
			return 1;
		}
    }
    return 0;
}
operation_result tcp_recv_file(FILE* file_ptr){
	log_trace("RECV FILE");
	char input_buffer[FILE_CHUNK_BUF_SIZE];
	while(1){
		bzero(input_buffer,FILE_CHUNK_BUF_SIZE);
		if(read(connfd, input_buffer, FILE_CHUNK_BUF_SIZE) <= 0){
			log_error("Failure to read %i bytes on file transfer.",FILE_CHUNK_BUF_SIZE);
			fclose(file_ptr);
			return socket_failure;
		}
		if(fwrite(input_buffer,1,sizeof(input_buffer),file_ptr) <= 0){
			log_error("Failure to write %i bytes into input file.",sizeof(input_buffer));
			fclose(file_ptr);
			return socket_failure;
		}
		if(scan_input_buf_for_EOF(input_buffer, FILE_CHUNK_BUF_SIZE)){
			break;
		}	
	}
	fclose(file_ptr);
	log_debug("Successful file transfer.");
	return socket_success;
}

operation_result tcp_recv_file_known_size(FILE* input_file, long byte_count){
	log_trace("HOLO FILE");
	log_trace("RECV FILE WITH SIZE: %lu", byte_count);
	char input_buffer[FILE_CHUNK_BUF_SIZE];
	long read_bytes = 0;
	int current_read = 0;
	while(read_bytes < byte_count){
		bzero(input_buffer,FILE_CHUNK_BUF_SIZE);
		current_read = read(connfd, input_buffer, FILE_CHUNK_BUF_SIZE);
		if( current_read <= 0){
			log_error("Failure to read %i bytes on file transfer.",FILE_CHUNK_BUF_SIZE);
			fclose(input_file);
			return socket_failure;
		}
		read_bytes += current_read;
		if(fwrite(input_buffer,1,sizeof(input_buffer),input_file) <= 0){
			log_error("Failure to write %i bytes into input file.",sizeof(input_buffer));
			fclose(input_file);
			return socket_failure;
		}
		current_read = 0;
	}
	fclose(input_file);
	log_debug("Successful file transfer.");
	return socket_success;
}

operation_result tcp_close_connection(){
	// close the socket 
    close(tcp_sockfd);
	return socket_success;
}
