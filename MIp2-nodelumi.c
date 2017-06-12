/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer nodelumi.c que implementa la interfície aplicació-administrador */
/* d'un node de LUMI, sobre la capa d'aplicació de LUMI (fent crides a la */
/* interfície de la capa LUMI -fitxers lumi.c i lumi.h-).                 */
/* Autors: X, Y                                                           */
/*                                                                        */
/**************************************************************************/

/* Inclusió de llibreries, p.e. #include <stdio.h> o #include "meu.h"     */
/* Incloem "MIp2-lumi.h" per poder fer crides a la interfície de LUMI     */
#include "MIp2-lumi.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <time.h>

/* Definició de constants, p.e., #define MAX_LINIA 150                    */

#define MAX_LINIA 300
#define nport 5555 
 

int crearTaulaUsuaris(struct usuari *t,char * domini);


int main(int argc,char *argv[])
{
	
 /* Declaració de variables, p.e., int n;                                 */
	int sckL,numUsuaris;
	char domini[50];
	////creem taula d'usuaris
	printf("Creant taula usuaris.\n");
	struct usuari tu[20];
	numUsuaris=crearTaulaUsuaris(tu,domini);
	printf("domini: %s\n",domini);
	int i=0;
	for (i=0;i<numUsuaris;i++) printf("%s -> %s : %d (%c)\n",tu[i].nom,tu[i].IP,tu[i].portUDP,tu[i].estat);
	
	sckL=UDP_CreaSock("0.0.0.0",nport);
	if(sckL<0) error("ERROR al crear socket local");

	LUMI_EngegarServidor(sckL,tu,numUsuaris,domini);
	
	return 0;
 }
 
 
 int crearTaulaUsuaris(struct usuari *t,char *domini)
{
	int n=0;
	char buffer[MAX_LINIA];
	FILE *fp;
 	fp = fopen("MIp2-nodelumi.cfg","r");
 	if (fp<0) error("ERROR al llegir dades de .cfg");
 	fgets(buffer,MAX_LINIA,fp);
	buffer[strlen(buffer)-1]='\0';
	strcpy(domini,buffer);
 	fgets(buffer,MAX_LINIA,fp);
 	while (!feof(fp)) {
		buffer[strlen(buffer)-1]='\0';
		strcpy(t[n].nom,buffer);
		t[n].portUDP=-1;
		t[n].estat='d';
		strcpy(t[n].IP,"-1");
		n++;
		fgets(buffer,MAX_LINIA,fp);
	}
	fclose(fp);
	return n;
}


