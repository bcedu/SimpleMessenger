/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer p2p.c que implementa la interfície aplicació-usuari de          */
/* l'aplicació de MI amb l'agent de LUMI integrat, sobre les capes        */
/* d'aplicació de MI i LUMI (fent crides a les interfícies de les capes   */
/* MI -fitxers mi.c i mi.h- i LUMI -lumi.c i lumi.h- ).                   */
/* Autors: X, Y                                                           */
/*                                                                        */
/**************************************************************************/

/* Inclusió de llibreries, p.e. #include <stdio.h> o #include "meu.h"     */
/* Incloem "MIp2-lumi.h" per poder fer crides a la interfície de LUMI     */
#include "MIp2-lumi.h"
#include "MIp2-mi.h"
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

int main(int argc,char *argv[])
{
	char nickL[300],nickR[300],IPrem[16],IPloc[16],buffer[300],missatge[300];
	int sckE,sckC,portE,portR,portL,n,r;
	int tanquem=0;
	int volemSortir=0;
	int iniciemConversa=0;
	
	int sckUDP,fd;
	char adrecaMI[300],MIrem[300];bzero(adrecaMI,300);bzero(MIrem,300);
	int llistaS[3];

	//demanem nom, adreça MI i port d'escolta
	printf("Com et dius?\n");
	read(0,nickL,300);
	printf("Quina adreça MI tens?\n");
	scanf("%s",adrecaMI);
	printf("Entreu el numero de port:\n");
	scanf("%d",&portE);
	
	//Creem socket d'escolta al port d'escolta
	sckE=MI_IniciaEscPetiRemConv(portE);
	if (sckE<0) {
		perror("Error en crear socket servidor. "); 
		exit(-1); 
	}
	sckUDP=LUMI_EngegarClient(adrecaMI,&fd);if (sckUDP<0) return -1;
	while (tanquem==0) {		
		while(!iniciemConversa) {
			printf("Entra la direcció MI a la que et vos connectar: \n (O espera a rebre una trucada...)\n");
			//esperem entrada de teclat ("truquem") o peticio de connexió (ens "truquen")
			llistaS[0]=0;
			llistaS[1]=sckE;
			llistaS[2]=sckUDP;
			n=HaArribatAlgunaCosaEnTemps(llistaS,3,-1);
			if (n==0) { //som client
				scanf("%s",MIrem);
				printf("La @MI introduida es: %s\n",MIrem);
				if (LUMI_peticioLocClient(sckUDP,MIrem,adrecaMI,fd,IPrem,&portR)>=0){
					sckC=MI_DemanaConv(IPrem,portR,IPloc,&portL,nickL,nickR);
					if (sckC>=0) {
						iniciemConversa=1;
						printf("Sock LOC: @IP %s,TCP, #port %d\n",IPloc,portL);
						printf("Sock REM: @IP %s,TCP, #port %d\n",IPrem,portR);
					}else perror("Error al demanar conversa. "); 
				}else perror("Error en fer localitzacio."); 
			}else if (n==sckE) { //som servidor
				sckC=MI_AcceptaConv(sckE, IPrem,&portR,IPloc,&portL,nickL,nickR);
				if (sckC>=0) {
					printf("S'ens han connectat\n");iniciemConversa=1;
					printf("Sock LOC: @IP %s,TCP, #port %d\n",IPloc,portL);
					printf("Sock REM: @IP %s,TCP, #port %d\n",IPrem,portR);
				}else perror("Error al acceptar conversa. "); 
			}else if (n==sckUDP) { //ens demanen info
				LUMI_respondrePeticioClient(sckUDP,portE,sckE,fd);
			}else  perror("Error al Select. "); 
		}
		while (!volemSortir) {
			printf("Envia el teu missatge:\n ('Esc'per sortir)\n");
			n=MI_HaArribatLinia(sckC);
			if (n==0) {
				bzero(buffer,300);
				r=read(0,buffer,299);
				if (buffer[0]==27) volemSortir=1; //si hem escrit Esc volem sortir
				else if (MI_EnviaLinia(sckC, buffer)<0) {
					perror("Error al enviar linia. "); 
					exit(-1); 
				}
			}else if (n==sckC) {
				bzero(buffer,300);
				r=MI_RepLinia(sckC, buffer);
				if (r==-2) {
					printf("%s tanca la connexió\n",nickR);
					volemSortir=1;
				}else if (r>=0) printf("%s: %s\n",nickR,buffer);
				else {
					perror("Error al rebre linia. "); 
					exit(-1); 
				}
			}else {
				perror("Error al Select. "); 
				exit(-1); 
			}
		}
		MI_AcabaConv(sckC);
		printf("Vols tancar la aplicacio?\n  (0->NO | 1->SI)  ");scanf("%d",&tanquem);
		if(!tanquem) {
				tanquem=0;
				volemSortir=0;
				iniciemConversa=0;
				LUMI_registrarClient(sckUDP,adrecaMI,fd);
		}
	}
	LUMI_desregistrarClient(sckUDP,adrecaMI,fd);
	MI_AcabaEscPetiRemConv(sckE);
	return 0;
}
