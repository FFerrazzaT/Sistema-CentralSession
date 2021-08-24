#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <unistd.h>

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

/******************************************************************
*
* Declarações de funções
*
*******************************************************************/

void main_screen(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid);
void saida_cliente(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid);
void send_mensage(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid);
void list_mensage(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid);
void post_mensage(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid);
void list_posts(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid);
void delete_mensage(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid);
void delete_post(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid);
void delete_all_posts(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid);
void list_users(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid);
void show_id(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid);
void help_menu();
void stop_all(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid);
char *safegets(char *buffer, int buffersize);

/******************************************************************
*
* Rotinas Principal do Cliente
*
*******************************************************************/

void main()
{
	int mqid;
	struct request_msg req_msg;
	struct response_msg resp_msg;
	struct user_record user_info;
	char tmp;
	
	mqid = msgget(MQCONVSRV_QUEUE_KEY, 0777);
	if (mqid == -1) {
   		printf("msgget() falhou, fila nao existe, servidor nao inicializado\n"); 
   		exit(1); 
	} 
	// Faz o login no servidor
	printf("CentralSeccion\n");
	printf("Entre com o nome de usuario:");
	safegets(user_info.name,MAXSIZE_USERNAME);
	user_info.id = getpid();	

	// Prepara requisicao de login
	req_msg.request=LOGIN_REQ;
	req_msg.server_id = SERVER_ID;
	req_msg.client_id = user_info.id;
	strncpy(req_msg.requestdata,user_info.name,MAXSIZE_USERNAME);	
	// Envia requisicao ao servidor
	msgsnd(mqid,&req_msg,sizeof(struct request_msg)-sizeof(long),0);
	// Espera pela mensagem de resposta especifica para este cliente
	// usando o PID do processo cliente como tipo de mensagem
	if (msgrcv(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),user_info.id,0) < 0) {
		printf("msgrcv() falhou\n"); 
		exit(1);
	}
	// Apresenta o texto da resposta
	printf("%s\n",resp_msg.responsedata);
	// Se nao aceitou login, sai do cliente
	if (resp_msg.response!=OK_RESP)
		exit(1);
		
	void main_screen(req_msg, resp_msg, user_info, mqid);

	exit(0);	
}


/******************************************************************
*
* Tela Principal
*
*******************************************************************/

void main_screen(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid){
	int opt;
	int stop_client;
	char tmp;

	// Laco de interface com o usuario
	stop_client=0;
	while(!stop_client) {
	  system("cls");

		printf("Qual opcao?\n");
		printf("   0: Sai do cliente;\n");
		printf("   1: Send <Usuario> <Texto>;\n");
		printf("   2: Msgs;\n");
		printf("   3: Post <mensagem>;\n");
		printf("   4: Show;\n");
		printf("   5: Del Msgs;\n");
		printf("   6: Del Post <Numero>;\n");
		printf("   7: Del Posts;\n");
		printf("   8: Users;\n");
		printf("   9: MyId;\n");
		printf("   10: Help;\n");
		printf("   11: Para o servidor e sai do cliente;\n");
		scanf("%d",&opt);
		scanf("%c",&tmp);  // Necessario por problema na scanf() que retorna newline extra
		
		switch(opt){
			
		  //Client exit
			case 0:
				  stop_client = 1;				
		      saida_cliente(req_msg, resp_msg, user_info, mqid);
			break;
			
			//send mensage
			case 1:
		      send_mensage(req_msg, resp_msg, user_info, mqid);
			break;
	
			//List mensages
			case 2:
		      list_mensage(req_msg, resp_msg, user_info, mqid);
			break;
			
		//Post a mensage
			case 3:
		      post_mensage(req_msg, resp_msg, user_info, mqid);
			break;
	
			//List all posts
			case 4:
		      list_posts(req_msg, resp_msg, user_info, mqid);
			break;
	
			//delete all mensages
			case 5:
		      delete_mensage(req_msg, resp_msg, user_info, mqid);
			break;

			//Delete 1 post
			case 6:
		      delete_post(req_msg, resp_msg, user_info, mqid);
			break;

			//Delete all posts
			case 7:
		      delete_all_posts(req_msg, resp_msg, user_info, mqid);
			break;

			//Users
			case 8:
		      list_users(req_msg, resp_msg, user_info, mqid);
			break;

			//ShowID
			case 9:
		     	show_id(req_msg, resp_msg, user_info, mqid);
			break;
		
			//Menu
			case 10: 
		    	help_menu();
			break;

			//Stop server and client 
			case 11:
			   	stop_client = 1;
		      stop_all(req_msg, resp_msg, user_info, mqid);		
			break;
					
			default:
				  printf("Opcao invalida\n");
			break;
		}
	}
	
}

/******************************************************************
*
* Funções de envio para servidor
*
*******************************************************************/

void saida_cliente(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid){

	// Prepara requisicao de logout
	req_msg.request=LOGOUT_REQ;
	req_msg.server_id = SERVER_ID;
	req_msg.client_id = user_info.id;
	
	strncpy(req_msg.requestdata,user_info.name,MAXSIZE_USERNAME);
	
	// Envia requisicao ao servidor
	msgsnd(mqid,&req_msg,sizeof(struct request_msg)-sizeof(long),0);
	
	// usando o PID do processo cliente como tipo de mensagem
	if (msgrcv(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),user_info.id,0) < 0) {
		printf("msgrcv() falhou\n"); 
		exit(1);
	}

	//Apresenta o texto de resposta
	printf("%s\n", resp_msg.responsedata);

}

void send_mensage(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid){

	//Seleciona Usuario a ser enviado
	printf("Qual usuario será enviado a mensagem:");
	safegets(req_msg.client_name,MAXSIZE_USERNAME);
	
	// Prepara envio
	printf("Entre com o texto a ser enviado:\n");
	safegets(req_msg.requestdata,MAXSIZE_MSGDATA);

	req_msg.request=SEND_MENSAGE_REQ;
	req_msg.server_id = SERVER_ID;
	req_msg.client_id = user_info.id;
	
	// Envia requisicao ao servidor
	msgsnd(mqid,&req_msg,sizeof(struct request_msg)-sizeof(long),0);

}

void list_mensage(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid){

	req_msg.request=LIST_MENSAGE_REQ;
	req_msg.server_id = SERVER_ID;
	req_msg.client_id = user_info.id;
	
	if (msgrcv(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),user_info.id,0) < 0) {
		printf("msgrcv() falhou\n"); exit(1);
	}
	
	// Apresenta o texto convertido
	printf("Mensagens':\n");
	printf("%s\n",resp_msg.responsedata);

}
	
void post_mensage(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid){
				
	// Prepara envio
	printf("Entre com o texto a ser enviado:\n");
	safegets(req_msg.requestdata,MAXSIZE_MSGDATA);
	
	req_msg.request=POST_MENSAGE_REQ;
	req_msg.server_id = SERVER_ID;
	req_msg.client_id = user_info.id;

}

void list_posts(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid){

	req_msg.request=SHOW_POST_REQ;
	req_msg.server_id = SERVER_ID;
	req_msg.client_id = user_info.id;

	if (msgrcv(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),user_info.id,0) < 0) {
		printf("msgrcv() falhou\n"); exit(1);
	}
	
	// Apresenta o texto convertido
	printf("Mensagens':\n");
	printf("%s\n",resp_msg.responsedata);

}

void delete_mensage(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid){

	req_msg.request=DEL_MENSAGE_REQ;
	req_msg.server_id = SERVER_ID;
	req_msg.client_id = user_info.id;
}
			
void delete_post(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid){

	printf("Qual o indice do post que deve ser excluido:");
	safegets(req_msg.index,MAXSIZE_USERNAME);

	req_msg.request=DEL_POST_REQ;
	req_msg.server_id = SERVER_ID;
	req_msg.client_id = user_info.id;

}
	
void delete_all_posts(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid){

	req_msg.request=DEL_POSTS_REQ;
	req_msg.server_id = SERVER_ID;
	req_msg.client_id = user_info.id;

}

void list_users(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid){

	req_msg.request=SHOW_USERS_REQ;
	req_msg.server_id = SERVER_ID;
	req_msg.client_id = user_info.id;

	if (msgrcv(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),user_info.id,0) < 0) {
		printf("msgrcv() falhou\n"); exit(1);
	}
	
	// Apresenta o texto convertido
	printf("Mensagens':\n");
	printf("%s\n",resp_msg.responsedata);
	
}

void show_id(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid){

	req_msg.request=SHOW_ID_REQ;
	req_msg.server_id = SERVER_ID;
	req_msg.client_id = user_info.id;
	
	if (msgrcv(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),user_info.id,0) < 0) {
		printf("msgrcv() falhou\n"); exit(1);
	}
	
	// Apresenta o texto convertido
	printf("Mensagens':\n");
	printf("%s\n",resp_msg.responsedata);

}

void help_menu(){

	system("cls");
	
	printf("HELP\n");
	printf("   0: Sai do cliente (Fecha janela cliente);\n");
	printf("   1: Send <Usuario> <Texto> (Envia mensagem para usuario definido);\n");
	printf("   2: Msgs (Lista todas as mensagens recebidas);\n");
	printf("   3: Post <mensagem> (Posta uma mensagem no forum comunitario);\n");
	printf("   4: Show (Mostra mensagens do forum comunitario);\n");
	printf("   5: Del Msgs (Deleta todas as mensagens enviadas ao usuario)\n");
	printf("   6: Del Post <Numero> (Deleta post com numero definido pelo usuario);\n");
	printf("   7: Del Posts (Deleta todos os posts do forum);\n");
	printf("   8: Users (Lista todos os usuarios logados)\n");
	printf("   9: MyId (Mostra ID do usuario);\n");
	printf("   10: Help (Menu de ajuda)\n");
	printf("   11: Para o servidor e sai do cliente (Parada Total do servidor e saida do cliente);\n");	

}

void stop_all(struct request_msg req_msg, struct response_msg resp_msg, struct user_record user_info, int mqid){

	// Prepara requisicao de parada de servidor
	req_msg.request = STOP_SERVER_REQ;
	req_msg.server_id = SERVER_ID;
	req_msg.client_id = user_info.id;
	
	// Envia requisicao ao servidor
	msgsnd(mqid,&req_msg,sizeof(struct request_msg)-sizeof(long),0);
	
	// usando o PID do processo cliente como tipo de mensagem
	if (msgrcv(mqid,&resp_msg,sizeof(struct response_msg)-sizeof(long),user_info.id,0) < 0) {
		printf("msgrcv() falhou\n"); exit(1);
	}
	
	// Apresenta o texto da resposta
	printf("%s\n",resp_msg.responsedata);

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

