/*
 * socket_client.h
 *
 *  Created on: 1 abr. 2019
 *      Author: steve-urbit
 * 
 * This header file provides high level abstractions for socket operations.
 * wW sugest mock socket server interactions using netcat
 * $ netcat -l -p 12121
 * For the socket client
 * $ netcat localhost 12121
 */

#ifndef SRC_SOCKET_CLIENT_H_
#define SRC_SOCKET_CLIENT_H_

#include<stdio.h>
#define RPC_JSON_FMT "{command_id:%d,satellite_id:%d,station_id:%d,payload:%Q,error:%Q}"

#define RPC_MSG_BUF_SIZE 504
#define FILE_CHUNK_BUF_SIZE 32

typedef enum{
	socket_success,
	socket_failure,
	socket_unknown
}operation_result;

typedef struct rpc
{
	unsigned char command_id;
	int satellite_id;
	int station_id;
	char* payload;//results
	char* error;
} rpc;

operation_result tcp_init_client();
operation_result tcp_init_server();
operation_result tcp_timeouts(int seconds);
operation_result tcp_connect_to_server(char*  server_ip);
operation_result tcp_send_data(char* data_buffer);
operation_result tcp_send_data_bytes(char* data_buffer, size_t byte_count);
operation_result tcp_recv_data(char* data_buffer);
operation_result tcp_recv_data_bytes(char* data_buffer, size_t byte_count);
operation_result tcp_send_rpc(rpc* rpc_message);
operation_result tcp_recv_rpc(rpc* rpc_message);
operation_result tcp_send_file(char* file_name);
operation_result tcp_recv_file(FILE* input_file);
operation_result tcp_recv_file_known_size(FILE* input_file, long byte_count);
operation_result tcp_close_connection();

//TODO endianness independendant implementations
int get_payload_size(char* whole_buffer);
void set_payload_size(int payload_size,char* whole_buffer);

operation_result udp_connect();

operation_result udp_init_client();
operation_result udp_init_server();
operation_result udp_timeouts(int seconds);
operation_result udp_connect_to_server(char*  server_ip);
operation_result udp_send_data();
operation_result udp_recv_data();
operation_result udp_send_rpc(rpc* rpc_message);
operation_result udp_recv_rpc(rpc* rpc_message);


#endif /* SRC_SOCKET_CLIENT_H_ */