#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <ctype.h>

#define MQCONVSRV_QUEUE_KEY	10020
#define SERVER_ID 		1

//Entrada
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

//Saida
#define OK_RESP			0
#define ERROR_RESP		1
#define ALREADY_LOG_RESP	2
#define CANNOT_LOG_RESP		3
#define INVALID_REQ_RESP	4

//Tamanho
#define MAXSIZE_MSGDATA 	200
#define MAXSIZE_USERNAME 	20
#define MAX_USERS 		100

/**********************************************************************
 *
 * Structs
 *
 * *******************************************************************/

//Entrada de clientes
struct request_msg {
 	long	server_id;
 	long	client_id;
   	int	request;
	char	requestdata[MAXSIZE_MSGDATA+1];
	char  client_name[MAXSIZE_USERNAME+1];
	long index;
};

//respostas curtas
struct response_msg {
	long	client_id;
	int	response;
	char	responsedata[MAXSIZE_MSGDATA+1];
};

//respostas longas
struct special_response_msg {
	long	client_id;
	int response;
	char	responsedata[2048];
};

//Usuarios
struct user_record 
{
	long id;
	char name[MAXSIZE_USERNAME+1];
};

//banco de dados de usuarios
struct database_users
{
	long id;
	char responsedata[MAXSIZE_MSGDATA+1];
};

//forum
struct forum_msg
{
	long code;
	char responsedata[MAXSIZE_MSGDATA+1];
};

/******************************************************************
*
* Funcoes
*
*******************************************************************/

struct special_response_msg spc;
struct database_users database[100];
struct user_record user_table[MAX_USERS];
struct request_msg req_msg;
struct response_msg resp_msg;
struct forum_msg forum[100];

//naturais
char *safegets(char *buffer, int buffersize);
void login();
void logout();
void post(int mqid, int code);
void list_forum(int mqid);
void del_post(int mqid);
void delete_forum(int mqid);
void users(int mqid);
void myid(int mqid);
void stopall();

//usuarios
int initialize_user_table();
int add_user_to_table(char *name, long id);
int del_user_from_table(char *name);

//Banco de dados usuarios
int initialize_database();
int add_mensage_to_database(char *responsedata, long id);
void del_data();

//Forum
int initialize_forum();
int add_mensage_to_forum(char *responsedata, long code);
int del_msg_from_forum(long code);
void del_forum();

//Procura usuarios
long get_user_id(char *name);
long get_user_name(char *name, long id);
long get_list_name();

/******************************************************************
*
* Rotinas Principal do Servidor
*
*******************************************************************/

void main()
{
	int mqid;
	int stop_server;
	int number = 0;
	int code = 0;


	// Faz as inicializacoes
	mqid = msgget(MQCONVSRV_QUEUE_KEY, IPC_CREAT | 0777);
	if (mqid == -1) {
   		printf("msgget() falhou\n"); exit(1); 
	} 
	printf("servidor: iniciou execucao\n");
	stop_server = 0;
	initialize_user_table();
	initialize_forum();
	
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
				login();
			break;
				
			case LOGOUT_REQ:
				logout();
			break;

				//Complete
			case SEND_MENSAGE_REQ:

				// Prepara a mensagem de resposta
				resp_msg.response = OK_RESP;
				resp_msg.client_id = req_msg.client_id;

				long tmpsend = get_user_id(req_msg.client_name);

				if (tmpsend == -1){
					strcpy(resp_msg.responsedata, "Usuario não encontrado");
					msgsnd(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),0);
				}
				else{
				strcpy(resp_msg.responsedata, "mensagem enviado com sucesso");
				msgsnd(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),0);
		
				strcpy(database[number].id, tmpsend);
				strcpy(database[number].responsedata, req_msg.requestdata);

				number++;
				}

				break;
			




			case LIST_MENSAGE_REQ:

				// Prepara a mensagem de resposta
				resp_msg.response = OK_RESP;
				resp_msg.client_id = req_msg.client_id;

				list_mensage_to_database(req_msg.client_id);
				msgsnd(mqid,&spc,sizeof(struct special_response_msg)-sizeof(long),0);

				break;
				




			case DEL_POST_REQ:
				del_post(mqid);
			break;

			case DEL_POSTS_REQ:
				delete_forum(mqid);
			break;

			case POST_MENSAGE_REQ:
				post(mqid, code);
			break;

			case SHOW_POST_REQ:
				list_forum(mqid);
			break;

			case SHOW_USERS_REQ:
				users(mqid);
			break;


			case SHOW_ID_REQ:
				myid(mqid);
			break;

			case STOP_SERVER_REQ:
				stop_server = 1;
				stopall();
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



/******************************************************************
*
* Funções dos casos
*
*******************************************************************/
void login(){

	if (get_user_id(req_msg.requestdata)>0) {
		printf("servidor: usuario %s ja' esta' logado\n",req_msg.requestdata);
		resp_msg.response = ALREADY_LOG_RESP;
		strcpy(resp_msg.responsedata,"ERROR: user already logged\n");
	} 

	else if (add_user_to_table(req_msg.requestdata,req_msg.client_id)==0) {
		printf("servidor: usuario %s aceito e logado\n",req_msg.requestdata);
		resp_msg.response = OK_RESP;
		strcpy(resp_msg.responsedata,"OK: user logged in\n");
	} 

	else {
		printf("servidor: usuario %s aceito mas nao foi possivel logar\n",req_msg.requestdata);
		resp_msg.response = CANNOT_LOG_RESP;
		strcpy(resp_msg.responsedata,"ERROR: cannot log user\n");
	}	

	resp_msg.client_id = req_msg.client_id;
}

void logout(){

	if (del_user_from_table(req_msg.requestdata)==0) {
		printf("servidor: usuario %s deslogado\n",req_msg.requestdata);
		resp_msg.response = OK_RESP;
		strcpy(resp_msg.responsedata,"OK: user logged out\n");
	} 

	else {
		printf("servidor: nao foi possivel deslogar usuario %s\n",req_msg.requestdata);
		resp_msg.response = ERROR_RESP;
		strcpy(resp_msg.responsedata,"ERROR: cannot log user out\n");
	}	
	
	resp_msg.client_id = req_msg.client_id;
}

void post(int mqid, int code){

	// Prepara a mensagem de resposta
	resp_msg.response = OK_RESP;
	resp_msg.client_id = req_msg.client_id;
	
	add_mensage_to_forum(req_msg.requestdata, code);
	
	strcpy(resp_msg.responsedata, "mensagem enviado com sucesso");
	msgsnd(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),0);
	code = code+1;

}

void list_forum(int mqid){

	printf("Listagem de usuarios ativada para %s. \n", req_msg.requestdata); 

	// Prepara a mensagem de resposta
	spc.response = OK_RESP;
	spc.client_id = req_msg.client_id;

	int j;
	for(j=0; j<20; j++){

		strncat(spc.responsedata, forum[j].responsedata, MAXSIZE_MSGDATA);
		strcat(spc.responsedata, "\n");
		
	}	
	
	msgsnd(mqid,&spc,sizeof(struct special_response_msg)-sizeof(long),0);
	strcpy(spc.responsedata, " ");

}

void del_post(int mqid){

	// Prepara a mensagem de resposta
	resp_msg.response = OK_RESP;
	resp_msg.client_id = req_msg.client_id;
	
	strcpy(resp_msg.responsedata, "deletado com sucesso");
	msgsnd(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),0);
	
	del_msg_from_forum(req_msg.requestdata);

}

void delete_forum(int mqid){

	// Prepara a mensagem de resposta
	resp_msg.response = OK_RESP;
	resp_msg.client_id = req_msg.client_id;

	strcpy(resp_msg.responsedata, "forum limpo");
	msgsnd(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),0);

	del_forum();

}

void users(int mqid){

	printf("Listagem de usuarios ativada para %s. \n", req_msg.requestdata); 

	// Prepara a mensagem de resposta
	spc.response = OK_RESP;
	spc.client_id = req_msg.client_id;

	int j;
	for(j=0; j<20; j++){

		strncat(spc.responsedata, user_table[j].name, MAXSIZE_USERNAME);
		strcat(spc.responsedata, "\n");
		
	}	
	
	msgsnd(mqid,&spc,sizeof(struct special_response_msg)-sizeof(long),0);
	strcpy(spc.responsedata, " ");
}

void myid(int mqid){

	// Prepara a mensagem de resposta
	resp_msg.response = OK_RESP;
	resp_msg.client_id = req_msg.client_id;

	strncpy(resp_msg.responsedata,get_user_name(req_msg.requestdata, req_msg.client_id),MAXSIZE_USERNAME);
	msgsnd(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),0);

}

void stopall(){

	// Prepara a mensagem de resposta
	resp_msg.response = OK_RESP;
	resp_msg.client_id = req_msg.client_id;
	strcpy(resp_msg.responsedata,"Stopping server\n");
	
}


/******************************************************************
*
* Rotinas de Gerenciamento do Banco de dados
*
*******************************************************************/
int initialize_database()
{
	int i;
	for (i=0; i<100; i++) {
		database[i].id = -1;
		strcpy(database[i].responsedata,"");
	}
	return -1;
}

int add_mensage_to_database(char *responsedata, long id)
{
	int i;
	for (i=0; i<100; i++) {
		if (database[i].id <= 0) {
			strncpy(database[i].responsedata, responsedata, MAXSIZE_MSGDATA);
			return 0;
		}
	}
	return -1;
}

void del_data()
{
	int i;
	for (i=0; i<100; i++) {
				database[i].id = -1;
				strcpy(database[i].responsedata,"");
	}
}

/******************************************************************
*
* Rotinas de Gerenciamento do Forum
*
*******************************************************************/
int initialize_forum()
{
	int i;
	for (i=0; i<100; i++) {
		forum[i].code = -1;
		strcpy(forum[i].responsedata,"");
	}
	return -1;
}

int add_mensage_to_forum(char *responsedata, long code)
{
	int i;
	for (i=0; i<100; i++) {
		if (forum[i].code <= 0) {
			strncpy(forum[i].responsedata, responsedata, MAXSIZE_MSGDATA);
			return 0;
		}
	}
	return -1;
}

int del_msg_from_forum(long code)
{
	int i;
	for (i=0; i<100; i++) {
		if (forum[i].code > 0) {
			if (strncmp(forum[i].responsedata, forum[code].responsedata, MAXSIZE_MSGDATA)==0) {
				forum[i].code = -1;
				strcpy(forum[i].responsedata,"");
				return 0;
			}
		}
	}
	return -1;
}

void del_forum()
{
	int i;
	for (i=0; i<100; i++) {
				forum[i].code = -1;
				strcpy(forum[i].responsedata,"");
	}
}
/******************************************************************
*
* Rotinas de Pesquisa da Tabela de Usuarios
*
*******************************************************************/
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

long get_user_name(char *name, long id)
{
	int i;
	for (i=0; i<MAX_USERS; i++) {
		if (user_table[i].id > 0) {
			if (strncmp(user_table[i].name, name, MAXSIZE_USERNAME)==0) {
				return user_table[i].name;
			}
		}
	}
	return -1;
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

