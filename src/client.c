#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 2048

// Variables globales
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];
char *ip;

void str_overwrite_stdout() {
  printf("%s", "> ");
  fflush(stdout);
}

void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}

void send_msg_handler() {
  char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {};

  while(1) {
  	str_overwrite_stdout();
    fgets(message, LENGTH, stdin);
    str_trim_lf(message, LENGTH);

    if (strcmp(message, "exit") == 0) {
			break;
    } else {
      sprintf(buffer, "%s(%s): %s\n", name,ip, message);
      send(sockfd, buffer, strlen(buffer), 0);
    }

		bzero(message, LENGTH);
    bzero(buffer, LENGTH + 32);
  }
  catch_ctrl_c_and_exit(2);
}

void recv_msg_handler() {
	char message[LENGTH] = {};
  while (1) {
		int receive = recv(sockfd, message, LENGTH, 0);
    if (receive > 0) {
      printf("%s", message);
      str_overwrite_stdout();
    } else if (receive == 0) {
			break;
    } else {
			// -1
		}
		memset(message, 0, sizeof(message));
  }
}

int main(int argc, char **argv){
	struct sockaddr_in adresse;
	socklen_t longueur;
	if(argc != 3){
		printf("Usage:  %s <port> <ip>  \n", argv[0], argv[1]);
		return EXIT_FAILURE;
	}

	ip= argv[2];
	
	int port = atoi(argv[1]);

	signal(SIGINT, catch_ctrl_c_and_exit);

	printf("Veuillez entrer votre nom : ");
  fgets(name, 32, stdin);
  str_trim_lf(name, strlen(name));

	if (strlen(name) > 32 || strlen(name) < 2){
		printf("Le nom doit comporter moins de 30 et plus de 2 caractères.\n");
		return EXIT_FAILURE;
	}
	
	struct sockaddr_in server_addr;

	/* Socket settings */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[1]));
	inet_aton(argv[2], &server_addr.sin_addr);
	
	
	// Connect to Server
	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
	if (err == -1) {
		printf("ERREUR: connexion\n");
		return EXIT_FAILURE;
	}
	
	longueur = sizeof(struct sockaddr_in);
	if (getsockname(sockfd, (struct sockaddr*)&server_addr, &longueur) < 0)
	{
		fprintf(stderr, "Erreur getsockname\n");
		return -1;
	}
	ip= inet_ntoa(adresse.sin_addr);
	
	// Send name
	send(sockfd, name, 32, 0);

	printf("==========================================================\n");
	printf("=== Bienvenu dans notre CHATROOM - By Lickos ===\n");
	printf("==========================================================\n\n");


	pthread_t send_msg_thread;
  if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0){
		printf("ERREUR: pthread\n");
    return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
  if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0){
		printf("ERREUR: pthread\n");
		return EXIT_FAILURE;
	}

	while (1){
		if(flag){
			printf("\nBye\n");
			break;
    }
	}

	close(sockfd);

	return EXIT_SUCCESS;
}