#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <ctype.h>

#define MQCONVSRV_QUEUE_KEY	10020

#define LOGIN_REQ		0
#define LOGOUT_REQ		1
#define SEND_MENSAGE_REQ 2 
#define LIST_MENSAGE_REQ 3
#define POST_MENSAGE_REQ 4
#define SHOW_POST_REQ 5
#define DEL_MENSAGE_REQ 6
#define DEL_POST_REQ 7
#define DEL_POSTS_REQ 8
#define SHOW_USERS_REQ 9
#define SHOW_ID_REQ 10
#define STOP_SERVER_REQ		99

#define OK_RESP			0
#define ERROR_RESP		1
#define ALREADY_LOG_RESP	2
#define CANNOT_LOG_RESP		3
#define INVALID_REQ_RESP	4

#define MAXSIZE_MSGDATA 	200

// As mensagens destinadas ao servidor tem que ter um identificador e esse 
// identif. nao pode ser zero, tem que ser um numero positivo maior que zero
// O numero 1 pode ser usado porque nao entra em conflito com os PIDs usados 
// como identificacao dos clientes.
#define SERVER_ID 		1

#define MAXSIZE_USERNAME 	20
#define MAX_USERS 		100

struct request_msg {
 	long	server_id;
 	long	client_id;
   	int	request;
	char	requestdata[MAXSIZE_MSGDATA+1];
	char  client_name[MAXSIZE_USERNAME+1];
	long index;
};

struct response_msg {
	long	client_id;
	int	response;
	char	responsedata[MAXSIZE_MSGDATA+1];
};


struct user_record 
{
	long id;
	char name[MAXSIZE_USERNAME+1];
};

struct user_record user_table[MAX_USERS];

/******************************************************************
*
* Rotinas Utilitarias
*
*******************************************************************/

char *safegets(char *buffer, int buffersize)
{
	char *str;
	int slen;
	
	// First we use fgets(), which has buffer 
	// protection, to read the string from 
	// standard input
	str = fgets(buffer,buffersize,stdin);
	// Different thant gets(), fgets() keeps 
	// the newline char read from the input 
	// as the last char of the string, so we 
	// eliminate this newline char here
	if (str!=NULL) {
		slen = strlen(str);
		if (slen>0)
			str[slen-1] = '\0';
	}
	return str;
}

void strtoupper(char * pstr) 
{
  	// Convert string to upper case
  	while (*pstr) {
    		*pstr = toupper((unsigned char) *pstr);
    		pstr++;
  	}

}

void strtolower(char * pstr) 
{
  	// Convert string to upper case
  	while (*pstr) {
    		*pstr = tolower((unsigned char) *pstr);
    		pstr++;
  	}

}


/******************************************************************
*
* Rotinas de Gerenciamento da Tabela de Usuarios
*
*******************************************************************/

int initialize_user_table()
{
	int i;
	for (i=0; i<MAX_USERS; i++) {
		user_table[i].id = -1;
		strcpy(user_table[i].name,"");
	}
	return -1;
}

int add_user_to_table(char *name, long id)
{
	int i;
	for (i=0; i<MAX_USERS; i++) {
		if (user_table[i].id <= 0) {
			strncpy(user_table[i].name, name, MAXSIZE_USERNAME);
			user_table[i].id = id;
			return 0;
		}
	}
	return -1;
}

long get_user_id(char *name)
{
	int i;
	for (i=0; i<MAX_USERS; i++) {
		if (user_table[i].id > 0) {
			if (strncmp(user_table[i].name, name, MAXSIZE_USERNAME)==0) {
				return user_table[i].id;
			}
		}
	}
	return -1;
}

int del_user_from_table(char *name)
{
	int i;
	for (i=0; i<MAX_USERS; i++) {
		if (user_table[i].id > 0) {
			if (strncmp(user_table[i].name, name, MAXSIZE_USERNAME)==0) {
				user_table[i].id = -1;
				strcpy(user_table[i].name,"");
				return 0;
			}
		}
	}
	return -1;
}

/******************************************************************
*
* Rotinas Principal do Servidor
*
*******************************************************************/

void main()
{
	int mqid;
	struct request_msg req_msg;
	struct response_msg resp_msg;
	int stop_server;
	
	// Faz as inicializacoes
	mqid = msgget(MQCONVSRV_QUEUE_KEY, IPC_CREAT | 0777);
	if (mqid == -1) {
   		printf("msgget() falhou\n"); exit(1); 
	} 
	printf("servidor: iniciou execucao\n");
	stop_server = 0;
	initialize_user_table();
	
	// Executa laco de atendimento de requisicoes dos clientes
	while (!stop_server) {
		// Espera pelas requisicoes dos cliente (message type=SERVER ID)
		if (msgrcv(mqid,&req_msg,sizeof(struct request_msg)-sizeof(long),SERVER_ID,0)<0) {
			printf("msgrcv() falhou\n"); 
			exit(1);
		}
		printf("servidor: recebeu requisicao %d do cliente %ld\n",req_msg.request,req_msg.client_id);
		// Atende a requisicao
		switch(req_msg.request) {
			case LOGIN_REQ:
				if (get_user_id(req_msg.requestdata)>0) {
					printf("servidor: usuario %s ja' esta' logado\n",req_msg.requestdata);
					resp_msg.response = ALREADY_LOG_RESP;
					strcpy(resp_msg.responsedata,"ERROR: user already logged\n");
				} else if (add_user_to_table(req_msg.requestdata,req_msg.client_id)==0) {
					printf("servidor: usuario %s aceito e logado\n",req_msg.requestdata);
					resp_msg.response = OK_RESP;
					strcpy(resp_msg.responsedata,"OK: user logged in\n");
				} else {
					printf("servidor: usuario %s aceito mas nao foi possivel logar\n",req_msg.requestdata);
					resp_msg.response = CANNOT_LOG_RESP;
					strcpy(resp_msg.responsedata,"ERROR: cannot log user\n");
				}	
				resp_msg.client_id = req_msg.client_id;
				break;
				
			case LOGOUT_REQ:
				if (del_user_from_table(req_msg.requestdata)==0) {
					printf("servidor: usuario %s deslogado\n",req_msg.requestdata);
					resp_msg.response = OK_RESP;
					strcpy(resp_msg.responsedata,"OK: user logged out\n");
				} else {
					printf("servidor: nao foi possivel deslogar usuario %s\n",req_msg.requestdata);
					resp_msg.response = ERROR_RESP;
					strcpy(resp_msg.responsedata,"ERROR: cannot log user out\n");
				}	
				resp_msg.client_id = req_msg.client_id;
				break;
					
				












			case SHOW_USERS_REQ:

				prinftf("Listagem de usuarios ativada para %s. \n", req_msg.requestdata); 



				// Prepara a mensagem de resposta
				resp_msg.response = OK_RESP;
				resp_msg.client_id = req_msg.client_id;
				strncpy(resp_msg.responsedata,req_msg.requestdata,MAXSIZE_MSGDATA);

				break;



			case SHOW_ID_REQ:

				// Prepara a mensagem de resposta
				resp_msg.response = OK_RESP;
				resp_msg.client_id = req_msg.client_id;
				strncpy(resp_msg.responsedata,req_msg.requestdata,MAXSIZE_MSGDATA);

				break;






















			case STOP_SERVER_REQ:
				// Para o servidor
				stop_server = 1;
				// Prepara a mensagem de resposta
				resp_msg.response = OK_RESP;
				resp_msg.client_id = req_msg.client_id;
				strcpy(resp_msg.responsedata,"Stopping server\n");
				break;
				
			default:
				// Requisicao invalida, apenas prepara a mensagem de resposta
				resp_msg.response = INVALID_REQ_RESP;
				resp_msg.client_id = req_msg.client_id;
				strcpy(resp_msg.responsedata,"Invalid request\n");
				break;										
		}
		// Envia a resposta ao cliente
		msgsnd(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),0);
		printf("servidor: enviou resposta %d ao cliente %ld\n",resp_msg.response,resp_msg.client_id);
	}

	// Deleta a fila de mensagens, para nao deixar requisicoes pendentes
	// que podem causar erros quando executar o servidor novamente
	msgctl(mqid,IPC_RMID,NULL);
	
	exit(0);	
}
