/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer lumi.c que implementa la capa d'aplicació de LUMI, sobre la     */
/* de transport UDP (fent crides a la interfície de la capa UDP           */
/* -sockets-).                                                            */
/* Autors: X, Y                                                           */
/*                                                                        */
/**************************************************************************/

/* Inclusió de llibreries, p.e. #include <sys/types.h> o #include "meu.h" */
/*  (si les funcions externes es cridessin entre elles, faria falta fer   */
/*   un #include "MIp2-lumi.h")                                           */
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
#include "MIp2-lumi.h"
/* Definició de constants, p.e., #define MAX_LINIA 150                    */
#define MAX_LINIA 300

/* Declaració de funcions internes que es fan servir en aquest fitxer     */
/* (les seves definicions es troben més avall) per així fer-les conegudes */
int LUMI_EngegarClient(char * adrecaMI,int *fd);
int LUMI_registrarClient(int sck,char *buffer,int fd);
int LUMI_desregistrarClient(int sck,char *buffer,int fd);
int LUMI_peticioLocClient(int sckUDP,char *MIrem,char *adrecaMI,int fd,char *IPrem,int *portR);
int LUMI_respondrePeticioClient(int sck,int PortTCP,int scKTCP,int fd);
int LUMI_EnviaLinia(int Sck, const char *IPrem, int portUDPrem, const char *SeqBytes, int LongSeqBytes,int fd);
char LUMI_RepLinia(int Sck, char *IPrem, int *portUDPrem, char *SeqBytes, int LongSeqBytes,int fd);
int LUMI_EngegarServidor(int sckL,struct usuari *tu,int numUsuaris,char * domini);
int LUMI_registrarServ(int sck,char *buffer,char *IPrem,int portRem,struct usuari *t,int numU,int fd);
int LUMI_desregistrarServ(int sck,char *buffer,char *IPrem,int portRem,struct usuari *t,int numU,int fd);
int LUMI_peticioLocServ(int sck,char *IPrem,int portRem,char *buffer,char *domini,struct usuari *t,int numU,int fd);
int LUMI_respostaLocServ(int sck,char *buffer,char *domini,struct usuari *t,int numU,int fd);
int obtenirInfoUsuari(char *IPrem,int *portRem,char *nom,struct usuari *t,int numU);
int posarOcupat(char *nom,struct usuari *t,int numU);
int obtenirIPlocal(char * IPloc);
/* des d'aqui fins al final de fitxer.                                    */
/* Com a mínim heu de fer les següents funcions internes:                 */
int UDP_CreaSockLoc();
int UDP_CreaSock(const char *IPloc, int portUDPloc);
int UDP_EnviaA(int Sck, const char *IPrem, int portUDPrem, const char *SeqBytes, int LongSeqBytes);
int UDP_RepDe(int Sck, char *IPrem, int *portUDPrem, char *SeqBytes, int LongSeqBytes);
int UDP_TancaSock(int Sck);
int UDP_TrobaAdrSockLoc(int Sck, char *IPloc, int *portUDPloc);
int HaArribatAlgunaCosaEnTemps(const int *LlistaSck, int LongLlistaSck, int Temps);
int ResolDNSaIP(const char *NomDNS, char *IP);
int Log_CreaFitx(const char *NomFitxLog);
int Log_Escriu(int FitxLog, const char *MissLog);
int Log_TancaFitx(int FitxLog);


/* Definicio de funcions EXTERNES, és a dir, d'aquelles que en altres     */
/* fitxers externs es faran servir.                                       */
/* En termes de capes de l'aplicació, aquest conjunt de funcions externes */
/* formen la interfície de la capa LUMI.                                  */
/* Les funcions externes les heu de dissenyar vosaltres...                */
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

/*Crea un socket UDP, registra al client de adrecaMI i crea el fitxer de log (es guarda l'identificador a fd).
*adrecaMI te el següent format: nom@domini.
* Retorna l'identificador de socket si tot va be, altrament -1.
*/
int LUMI_EngegarClient(char * adrecaMI,int *fd) {
	char buffer[300];bzero(buffer,300);
	//creem socket UDP
	int sckUDP=UDP_CreaSock("0.0.0.0",0); if(sckUDP<0) error("ERROR al crear socket UDP");
	//creem fitxer log
	printf("Nom del fitxer de log?\n");
	bzero(buffer,MAX_LINIA);read(0,buffer,MAX_LINIA);buffer[strlen(buffer)-1]='\0';
	*fd=Log_CreaFitx(buffer);if(*fd<0) {perror("ERROR al crear fitxer de log");return -1;}
	if(LUMI_registrarClient(sckUDP,adrecaMI,*fd)<0)  {perror("ERROR al registrar client");return -1;}
	return sckUDP;
}


int LUMI_registrarClient(int sck,char *adrecaMI,int fd){
	char usuari[30],domini[30],IP[16],aux[300],buffer[300];bzero(aux,300);bzero(usuari,30);bzero(domini,30);
	int LlistaSck[0],arrivat,socketEscolta,serverSocket,intents=0;
	sscanf(adrecaMI,"%[^@]@%s",usuari,domini);
	if(ResolDNSaIP(domini,IP)<0){
		error("ERROR al resoldre DNS");
	}
	aux[0]='R';
	usuari[strlen(usuari)]='\0';
	strcat(aux,usuari);
	while (intents<5) {
		LUMI_EnviaLinia(sck,IP,5555,aux,strlen(aux),fd);
		LlistaSck[0]=sck;
		arrivat=HaArribatAlgunaCosaEnTemps(LlistaSck,1,3);
		if(arrivat==-1){perror("ERROR al enviar missatge");return -1;}
		else if(arrivat==-2){
			perror("S'ha expirat el temps");
			if (intents>5) return -1;
		}else if(arrivat==sck){
				bzero(buffer,sizeof(buffer));
				if(LUMI_RepLinia(sck,IP,&serverSocket,buffer,sizeof(buffer),fd)!='A')
				{
					perror("ERROR Format desconegut");
					return -1;
				}
				if(buffer[0]=='0'){perror("ERROR, format incorrecte\n");return -1;}
				else if(buffer[0]=='1'){printf("Registre Correcte\n");return 1;}
				else if(buffer[0]=='2'){perror("No existeix l'usuari\n");return -1;}
		}
		intents++;
	}
	return 1;
}

int LUMI_desregistrarClient(int sck,char *buffer,int fd){
	char usuari[30],domini[30],IP[16],aux[300];bzero(aux,300);bzero(usuari,30);bzero(domini,30);
	int LlistaSck[0],arrivat,socketEscolta,serverSocket,intents=0;
	sscanf(buffer,"%[^@]@%s",usuari,domini);
	if(ResolDNSaIP(domini,IP)<0){
		perror("ERROR al resoldre DNS");return -1;
	}
	aux[0]='D';
	usuari[strlen(usuari)]='\0';
	strcat(aux,usuari);
	while (intents<5) {
		LUMI_EnviaLinia(sck,IP,5555,aux,strlen(aux),fd);
		LlistaSck[0]=sck;
		arrivat=HaArribatAlgunaCosaEnTemps(LlistaSck,1,3);
		if(arrivat==-1){perror("ERROR al enviar missatge");return -1;}
		else if(arrivat==-2){perror("S'ha expirat el temps");if(intents>5)return -1;}
		else if(arrivat==sck){
				bzero(buffer,sizeof(buffer));
				if(LUMI_RepLinia(sck,IP,&serverSocket,buffer,sizeof(buffer),fd)=='z')
				{
					perror("ERROR Format desconegut");
					return -1;
				}
				if(buffer[0]=='0'){perror("ERROR, format incorrecte\n");return -1;}
				else if(buffer[0]=='1'){printf("Desregistre Correcte\n");return 1;}
				else if(buffer[0]=='2'){perror("No existeix l'usuari\n");return -1;}
		}
		intents++;
	}
	return 1;
}


int LUMI_peticioLocClient(int sck,char *adrecaMIpreguntada,char *adrecaMIpreguntador,int fd,char *IPrem,int *PortRem){
	char codiresposta[30],usuari[30],domini[30],IP[16],aux[300],buffer[300];bzero(IPrem,sizeof(IPrem));bzero(aux,300);bzero(buffer,300);bzero(usuari,30);bzero(codiresposta,30);bzero(domini,30);
	int LlistaSck[0],arrivat,socketEscolta,serverSocket,intents=0;
	sscanf(adrecaMIpreguntador,"%[^@]@%s",usuari,domini);
	if(ResolDNSaIP(domini,IP)<0){
		perror("ERROR al resoldre DNS");return -1;
	}
	aux[0]='L';
	usuari[strlen(usuari)-1]='\0';
	strcat(aux,adrecaMIpreguntada);
	strcat(aux,":");
	strcat(aux,adrecaMIpreguntador);
	while (intents<5) {
		LUMI_EnviaLinia(sck,IP,5555,aux,strlen(aux),fd);
		LlistaSck[0]=sck;
		arrivat=HaArribatAlgunaCosaEnTemps(LlistaSck,1,3);
		if(arrivat==-1){perror("ERROR al enviar missatge\n");return -1;}
		else if(arrivat==-2){perror("S'ha expirat el temps\n");if(intents>5)return -1;}
		else if(arrivat==sck){
				bzero(buffer,sizeof(buffer));
				if(LUMI_RepLinia(sck,IP,&serverSocket,buffer,sizeof(buffer),fd)!='Y')
				{
					perror("ERROR Format desconegut");
					return -1;
				}
				sscanf(buffer,"%[^:]:%d:%[^:]:%[^:]",IPrem,*&PortRem,adrecaMIpreguntada,adrecaMIpreguntador);
				codiresposta[0]=IPrem[0];
				for (arrivat=1;arrivat<strlen(IPrem);arrivat++)IPrem[arrivat-1]=IPrem[arrivat];
				IPrem[strlen(IPrem)-1]='\0';
				for(arrivat=1;arrivat<strlen(codiresposta);arrivat++) IPrem[arrivat-1]=codiresposta[arrivat]; 
				if(codiresposta[0]=='0'){perror("ERROR, format incorrecte\n");return -1;}
				else if(codiresposta[0]=='1'){printf("Connexió establida\n");return 1;}
				else if(codiresposta[0]=='2'){perror("No existeix l'usuari\n");return -1;}
				else if(codiresposta[0]=='3'){perror("Usuari offline\n");return -1;}
				else if(codiresposta[0]=='4'){perror("Usuari ocupat\n");return -1;}
				else return -1;
		}
		intents++;
	}return 1;
}



int LUMI_respondrePeticioClient(int sck,int PortTCP,int scKTCP,int fd){
	char aux[300],buffer[300],nomMIpreguntada[60],dominiPreguntat[60],nomMIpreguntador[60],dominiPreguntador[60],IP[16],IPloc[16];
	bzero(aux,300);bzero(buffer,300);bzero(IP,16);bzero(IPloc,16);
	int codiresposta,portUDP,paux;
	if(LUMI_RepLinia(sck,IP,&portUDP,buffer,sizeof(buffer),fd)!='L') {
				perror("ERROR Format desconegut");
				return -1;
	}
	sscanf(buffer,"%[^@]@%[^:]:%[^@]@%s",nomMIpreguntada,dominiPreguntat,nomMIpreguntador,dominiPreguntador);
	if(ResolDNSaIP(dominiPreguntat,IP)<0){
		error("ERROR al resoldre DNS");
	}obtenirIPlocal(IPloc);
	sprintf (aux,"Y1%s:%d:%s",IPloc, PortTCP, buffer);
	LUMI_EnviaLinia(sck,IP,5555,aux,strlen(aux),fd);
	return 1;
}



/* Envia a través del socket UDP d’identificador “Sck” la seqüència de    */
/* bytes escrita a “SeqBytes” (de longitud “LongSeqBytes” bytes) cap al   */
/* socket remot que té @IP “IPrem” i #port UDP “portUDPrem”.              */
/* Escriu al fitxer fd el la informacio (@IP i port) corresponen a qui    */
/* s'envia el missatge, a més del missatge i la longitud d'aquest.        */
/* "IPrem" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0')                */
/* "SeqBytes" és un vector de chars qualsevol (recordeu que en C, un      */
/* char és un enter de 8 bits) d'una longitud >= LongSeqBytes bytes       */
/* Retorna -1 si hi ha error; el nombre de bytes enviats si tot va bé.    */
int LUMI_EnviaLinia(int Sck, const char *IPrem, int portUDPrem, const char *SeqBytes, int LongSeqBytes,int fd)
{
	char aux[MAX_LINIA]; bzero(aux,MAX_LINIA);
	sprintf (aux, "E: %s:UDP:%d, %s, %d\n", IPrem, portUDPrem, SeqBytes, LongSeqBytes);
	if (Log_Escriu(fd,aux)<0) {perror("ERROR al escriure a log");return -1;}
	
	return UDP_EnviaA(Sck,IPrem,portUDPrem,SeqBytes,LongSeqBytes);
}


/* Rep a través del socket UDP d’identificador “Sck” una seqüència de     
* bytes que prové d'un socket remot i l’escriu a “SeqBytes*” (que té     
* una longitud de “LongSeqBytes” bytes) sense el primer caracter.
* El primer caracter correspon al codi tipus de missatge que pot ser:
* 'R','A','D','L' i 'Y'.    
* Omple "IPrem*" i "portUDPrem*" amb respectivament, l'@IP i el #port    
* UDP del socket remot.   
* Escriu al fitxer identificat per fd el missatge rebut, la informacio de qui l'ha enviat (@IP i port) i la longitude del missatge                                               
* "IPrem*" és un "string" de C (vector de chars imprimibles acabat en    
* '\0') d'una longitud màxima de 16 chars (incloent '\0')                
* "SeqBytes*" és un vector de chars qualsevol (recordeu que en C, un     
* char és un enter de 8 bits) d'una longitud <= LongSeqBytes bytes       
* Retorna 'z' si hi ha error; 'f' si el codi no coincideix amb cap dels dits anteriorment;
* altrament el caracter del tipus de missatge  */
char LUMI_RepLinia(int Sck, char *IPrem, int *portUDPrem, char *SeqBytes, int LongSeqBytes,int fd)
{
	char c;int i;
	char aux[MAX_LINIA];
	i=UDP_RepDe(Sck,IPrem,portUDPrem,SeqBytes,LongSeqBytes);
    if(i<0){
		perror("ERROR al rebre linia");return 'z';
	}
	sprintf (aux, "R: %s:UDP:%d, %s, %d\n", IPrem, *portUDPrem, SeqBytes, i);
	if (Log_Escriu(fd,aux)<0) {perror("ERROR al escriure a log");return 'z';}
	c=SeqBytes[0];
	for (i=1;i<strlen(SeqBytes);i++)SeqBytes[i-1]=SeqBytes[i];
	SeqBytes[strlen(SeqBytes)-1]='\0';
	
	return c;
}



/*Engega un servidor de LUMI al socket sckL */
int LUMI_EngegarServidor(int sckL,struct usuari *tu,int numUsuaris,char * domini) {
	int portRem,aux,llistaS[2],fd,tanquem,i;
	char c,IPrem[16],buffer[MAX_LINIA];
	////creem fitxer de Log
	printf("Nom del fitxer de log?\n");
	bzero(buffer,MAX_LINIA);read(0,buffer,MAX_LINIA);buffer[strlen(buffer)-1]='\0';
	fd=Log_CreaFitx(buffer);if(fd<0) error("ERROR al crear fitxer de log");
	
	tanquem=0;
	while(!tanquem) {
		printf("Servidor funcionant... (pitjar qualsevol tecla per tancar)\n");
		llistaS[1]=sckL;
		llistaS[0]=0;
		aux=HaArribatAlgunaCosaEnTemps(llistaS,2,-1);
		if (aux==-2) printf("Temps d'espera acavat sense rebre missatge");
		else if (aux==sckL) {
			bzero(buffer,MAX_LINIA);
			c=LUMI_RepLinia(sckL,IPrem,&portRem,buffer,sizeof(buffer),fd);
			if (c=='z') error("ERROR al rebre missatge");
			printf("client: %s. Rebut del client amb @IP: %s i port %d.\n",buffer,IPrem, portRem);
			if (c=='R') {
				if (LUMI_registrarServ(sckL,buffer,IPrem,portRem,tu,numUsuaris,fd)<0) perror("ERROR al registrar client");
				i=0;
				for (i=0;i<numUsuaris;i++) printf("%s -> %s : %d (%c)\n",tu[i].nom,tu[i].IP,tu[i].portUDP,tu[i].estat);				
			}else if (c=='D') {
				if (LUMI_desregistrarServ(sckL,buffer,IPrem,portRem,tu,numUsuaris,fd)<0) perror("ERROR al registrar client");
				i=0;
				for (i=0;i<numUsuaris;i++) printf("%s -> %s : %d (%c)\n",tu[i].nom,tu[i].IP,tu[i].portUDP,tu[i].estat);	
			}else if (c=='L') {
				//LUMI_PeticioLoc();
				if (LUMI_peticioLocServ(sckL,IPrem,portRem,buffer,domini,tu,numUsuaris,fd)<0) perror("ERROR al localitzar un client");
			}else if (c=='Y') {
				//LUMI_RespostaLoc();
				if (LUMI_respostaLocServ(sckL,buffer,domini,tu,numUsuaris,fd)<0) perror("ERROR al respondre a una localitzacio");
			}else {
				//formatIncorrecte();
				printf("Format incorrecte\n");
			}
		}else if(aux==0){
			tanquem=1;read(0,buffer,MAX_LINIA);
		}else  error("ERROR al select");
	}
	printf("Tancant servidor.\n");
	close(sckL);
	
	return 0;
}	


/* Registra al usuari de nom buffer  a la
 * IPrem i el portRem. Per fer-ho es busca a la taula t l'usuari amb el seu
 * nom i s'actualitza les seves dades amb IPrem i portRem.
 * Després s'envia un missatge a traves de sck al client amb IPrem i portRem amb el format 'A'codi,
 * en que codi pot ser 0 (format incorrecte), 1 (registre correcte) o 2 (no existeix usuari).
 * S'escriu al fitxer identificat per fd els missatges enviats i rebuts.
 * Retorna -1 en cas d'error (usuari no existeix), altrament 0*/
int LUMI_registrarServ(int sck,char *buffer,char *IPrem,int portRem,struct usuari *t,int numU,int fd) {
	int i,trobat=0;

	//busquem el nom a la taula
	i=0;
	while(i<numU && !trobat) {
		if (strcmp(t[i].nom,buffer)==0) trobat=1;
		else i++;
	}

	if (!trobat) {
		if(LUMI_EnviaLinia(sck,IPrem,portRem,"A2",strlen("A2"),fd)<0) perror("ERROR al enviar missatge");
		return -1;
	}	//si usuari no existeix

	//actualitzem informacio
	strcpy(t[i].IP,IPrem);
	t[i].portUDP=portRem;
	t[i].estat='c';
	
	//enviem missatge de confirmacio de registre
	if(LUMI_EnviaLinia(sck,IPrem,portRem,"A1",strlen("A1"),fd)<0) {perror("ERROR al enviar missatge");return -1;}
	return 0;
}

/* Desregistra al usuari de nom buffer.
 * Per fer-ho es busca a la taula t l'usuari amb el seu
 * nom i es posen la IP i el portUDP a -1.
 * Després s'envia un missatge a traves de sck al client amb IPrem i portRem amb el format 'A'codi,
 * en que codi pot ser 0 (format incorrecte), 1 (desregistre correcte) o 2 (no existeix usuari).
 * Escriu al fitxer fd la informacio dels missatges enviats i rebuts.
 * Retorna -1 en cas d'error (usuari no existeix), altrament 0*/
int LUMI_desregistrarServ(int sck,char *buffer,char *IPrem,int portRem,struct usuari *t,int numU,int fd) {
	int i,trobat=0;

	//busquem el nom a la taula
	i=0;
	while(i<numU && !trobat) {
		if (strcmp(t[i].nom,buffer)==0) trobat=1;
		else i++;
	}

	if (!trobat) {
		if(LUMI_EnviaLinia(sck,IPrem,portRem,"A2",strlen("A2"),fd)<0) perror("ERROR al enviar missatge");
		return -1;
	}	//si usuari no existeix

	//actualitzem informacio
	strcpy(t[i].IP,"-1");
	t[i].portUDP=-1;
	t[i].estat='d';

	//enviem missatge de confirmacio de registre
	if(LUMI_EnviaLinia(sck,IPrem,portRem,"A1",strlen("A1"),fd)<0) {perror("ERROR al enviar missatge");return -1;}
	return 0;
}


/* Localitza al usuari de @MIpreguntada continguda a buffer.
 * Buffer conte @MIpreguntada:@MIpreguntador.
 * 
 * Si el domini de la @MIpreguntada és igual al domini entrat, es busca a la taula t l'usuari amb el seu nom.
 * Si no es troba l'usuari s'envia el missatge Y2:-1:@MIpreguntada:@MIpreguntador a traves de sck a IPrtem i portRem.
 * Si es trova i està "offline" s'envia el missatge Y3:-1:@MIpreguntada:@MIpreguntador a IPrtem i portRem.
 * Si es trova i està "ocupat" s'envia el missatge Y4:-1:@MIpreguntada:@MIpreguntador a IPrtem i portRem.
 * 
 * Si es trova i està "online" s'envia el missatge L@MIpreguntada:@MIpreguntador a IP i portUDP del usuari trobat.
 *
 * Si el domini de la @MIpreguntada és diferent al domini entrat, s'envia el missatge L@MIpreguntada:@MIpreguntador
 * al servidor amb el domini de la @MIpreguntada. Si no es pot resoldre DNS s'envia Y2:-1:@MIpreguntada:@MIpreguntador i es retorna -1.
 *
 * Si en algun moment es detecta que el format del missatge es incorrecte s'envia Y0:-1:@MIpreguntada:@MIpreguntador.
 *
 * S'escriu al fitxer fd la informacio de tots els missatges rebuts i enviats
 *
 * Si es produeix algun error retorna -1; altrament 0.  */
int LUMI_peticioLocServ(int sck,char *IPrem,int portRem,char *buffer,char *domini,struct usuari *t,int numU,int fd) {
	char nomMIpreguntada[100],dominiPreguntat[100],nomMIpreguntador[100],dominiPreguntador[100],IP[16];
	int i=0,portUDP;
	char aux[MAX_LINIA];bzero(aux,MAX_LINIA);
	bzero(nomMIpreguntada,100);bzero(dominiPreguntat,100);bzero(nomMIpreguntador,100);bzero(dominiPreguntador,100);
	if(sscanf(buffer,"%[^@:]@%[^@:]:%[^@:]@%[^@:]",nomMIpreguntada,dominiPreguntat,nomMIpreguntador,dominiPreguntador)!=4) {
			perror("ERROR format de missatge incorrecte");
			strcpy(aux,"Y0::");
			strcat(aux, buffer);
			if(LUMI_EnviaLinia(sck,IPrem,portRem,aux,strlen(aux),fd)<0) {perror("ERROR al enviar missatge");return -1;}
			return -1;
	}
	if (strcmp(domini,dominiPreguntat)==0)  {
		i=obtenirInfoUsuari(IP,&portUDP,nomMIpreguntada,t,numU);
		bzero(aux,MAX_LINIA);
		if (i==0) {
			strcpy(aux,"Y4:-1:");
			strcat(aux, buffer);
			if(LUMI_EnviaLinia(sck,IPrem,portRem,aux,strlen(aux),fd)<0) {perror("ERROR al enviar missatge");return -1;}
			return-1;
		}else if(i==-1){
			strcpy(aux,"Y3:-1:");
			strcat(aux, buffer);
			if(LUMI_EnviaLinia(sck,IPrem,portRem,aux,strlen(aux),fd)<0) {perror("ERROR al enviar missatge");return -1;}
			return-1;
		}else if(i<-1) {
			strcpy(aux,"Y2:-1:");
			strcat(aux, buffer);
			if(LUMI_EnviaLinia(sck,IPrem,portRem,aux,strlen(aux),fd)<0) {perror("ERROR al enviar missatge");return -1;}
			return-1;
		}else if (i>=1) if(posarOcupat(nomMIpreguntada,t,numU)<0)  perror("ERROR al canviar estat de usuari");
	}else {
		if (ResolDNSaIP(dominiPreguntat,IP)<0) {
			perror("ERROR al trobar DNS servidor");
			strcpy(aux,"Y2:-1:");
			strcat(aux, buffer);
			if(LUMI_EnviaLinia(sck,IPrem,portRem,aux,strlen(aux),fd)<0) {perror("ERROR al enviar missatge");return -1;}
			return -1;
		}
		portUDP=5555;
	}
	aux[0]='L';
	strcat(aux, buffer);
	if(LUMI_EnviaLinia(sck,IP,portUDP,aux,strlen(aux),fd)<0) {perror("ERROR al enviar missatge");return -1;}
	return 0;
}

/* Buffer conte codiResposta@IP:portTCP:@MIpreguntada:@MIpreguntador.
 * 
 * Si el domini de la @MIpreguntador és igual al domini entrat, es busca a la taula t l'usuari amb el seu nom.  
 * Si es trova s'envia el missatge YcodiResposta@IP:portTCP:@MIpreguntada:@MIpreguntador a IP i portUDP del usuari trobat.
 * Si no es trova s'envia  Y2:-1:@MIpreguntada:@MIpreguntador i es retorna -1.
 *
 * Si el domini de la @MIpreguntador és diferent al domini entrat, s'envia el missatge YcodiResposta@IP:portTCP:@MIpreguntada:@MIpreguntado
 * al servidor amb el domini de la @MIpreguntador. Si no es pot resoldre DNS es retorna -1.
 *
 * Si en algun moment es detecta que el format del missatge es incorrecte es retorna -1.
 * 
 * Si es produeix algun error retorna -1; altrament 0.  */
int LUMI_respostaLocServ(int sck,char *buffer,char *domini,struct usuari *t,int numU,int fd) {
	char nomMIpreguntada[100],dominiPreguntat[100],nomMIpreguntador[100],dominiPreguntador[100],IP[16];
	int i=0,portUDP;
	char aux[MAX_LINIA];bzero(aux,MAX_LINIA);
	bzero(nomMIpreguntada,100);bzero(dominiPreguntat,100);bzero(nomMIpreguntador,100);bzero(dominiPreguntador,100);
	sscanf(buffer,"%[^@:]:%d:%[^@:]@%[^@:]:%[^@:]@%[^@:]",aux,&i,nomMIpreguntada,dominiPreguntat,nomMIpreguntador,dominiPreguntador);
	if (strcmp(domini,dominiPreguntador)==0)  {
		i=obtenirInfoUsuari(IP,&portUDP,nomMIpreguntador,t,numU);
		if(i<-1) {
			printf("ERROR: Usuari %s no existeix\n",nomMIpreguntador);
			return-1;
		}
	}else {
		if (ResolDNSaIP(dominiPreguntador,IP)<0) {perror("ERROR al trobar DNS servidor");return -1;}
		portUDP=5555;
	}
	bzero(aux,MAX_LINIA);aux[0]='Y';
	strcat(aux, buffer);
	if(LUMI_EnviaLinia(sck,IP,portUDP,aux,strlen(aux),fd)<0) {perror("ERROR al enviar missatge");return -1;}
	return 0;
}


/* Busca a la taula t el usuari amb nom igual al nom entrat i emplena IPrem
 * i portRem amb les seves dades.
 * Si el usuari no es troba retorna -2;si es troba pero estat='d'(desconectat) retorna -1;
 * si es troba pero estat='o' (ocupat) retorna 0 i si es troba i estat='c' retorna 1.*/
int obtenirInfoUsuari(char *IPrem,int *portRem,char *nom,struct usuari *t,int numU){
	int i,trobat=0;
	//busquem el nom a la taula
	i=0;
	while(i<numU && !trobat) {
		if (strcmp(t[i].nom,nom)==0) trobat=1;
		else i++;
	}
	if (!trobat) return -2;
	else if (t[i].estat=='d') return -1;
	else if (t[i].estat=='o') return 0;
	else if (t[i].estat=='c') {
		strcpy(IPrem,t[i].IP);
		*portRem=t[i].portUDP;
		return 1;
	}
	return -2;
}

int posarOcupat(char *nom,struct usuari *t,int numU){
	int i,trobat=0;
	//busquem el nom a la taula
	i=0;
	while(i<numU && !trobat) {
		if (strcmp(t[i].nom,nom)==0) trobat=1;
		else i++;
	}
	if (!trobat) return -2;
	else t[i].estat='o';
	return 1;
}


/* Definicio de funcions INTERNES, és a dir, d'aquelles que es faran      */
/* servir només en aquest mateix fitxer.                                  */

/* Crea un socket UDP  local sense connectar*/
/* Retorna -1 si hi ha error; l’identificador del socket creat si tot     */
/* va bé.                                                                 */
int UDP_CreaSockLoc() {
	return socket(AF_INET, SOCK_DGRAM, 0);
}

/* Crea un socket UDP a l’@IP “IPloc” i #port UDP “portUDPloc”            */
/* (si “IPloc” és “0.0.0.0” i/o “portUDPloc” és 0 es fa/farà una          */
/* assignació implícita de l’@IP i/o del #port UDP, respectivament).      */
/* "IPloc" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0')                */
/* Retorna -1 si hi ha error; l’identificador del socket creat si tot     */
/* va bé.                                                                 */
int UDP_CreaSock(const char *IPloc, int portUDPloc)
{
	int sockfd;
	struct sockaddr_in servAdr;
	socklen_t len = sizeof(servAdr);
	//Creem el socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return -1;
    
    //Construim la adreça de servidor
    len=sizeof(servAdr);
	bzero((char *) &servAdr, len); //inicialitza tot a zeros
	servAdr.sin_family=AF_INET;
	servAdr.sin_port=htons(portUDPloc);
	servAdr.sin_addr.s_addr=INADDR_ANY; //INADDR_ANY es la adreça de la nostra maquina
	
	//enllaçem el socket a la adreça 
	bind(sockfd, (struct sockaddr *) &servAdr,len);
	return sockfd;
}

/* Envia a través del socket UDP d’identificador “Sck” la seqüència de    */
/* bytes escrita a “SeqBytes” (de longitud “LongSeqBytes” bytes) cap al   */
/* socket remot que té @IP “IPrem” i #port UDP “portUDPrem”.              */
/* "IPrem" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0')                */
/* "SeqBytes" és un vector de chars qualsevol (recordeu que en C, un      */
/* char és un enter de 8 bits) d'una longitud >= LongSeqBytes bytes       */
/* Retorna -1 si hi ha error; el nombre de bytes enviats si tot va bé.    */
int UDP_EnviaA(int Sck, const char *IPrem, int portUDPrem, const char *SeqBytes, int LongSeqBytes)
{
	socklen_t len;
	struct sockaddr_in servAdr;

	//creem adreça servidor
	bzero((char *) &servAdr, sizeof(servAdr)); //inicialitza tot a zeros
	servAdr.sin_family=AF_INET;
	servAdr.sin_port=htons(portUDPrem);
	servAdr.sin_addr.s_addr=inet_addr(IPrem);
	len=sizeof(struct sockaddr_in);

	//enviem missatge
	return sendto(Sck,SeqBytes,LongSeqBytes,0,&servAdr,len);
}

/* Rep a través del socket UDP d’identificador “Sck” una seqüència de     */
/* bytes que prové d'un socket remot i l’escriu a “SeqBytes*” (que té     */
/* una longitud de “LongSeqBytes” bytes).                                 */
/* Omple "IPrem*" i "portUDPrem*" amb respectivament, l'@IP i el #port    */
/* UDP del socket remot.                                                  */
/* "IPrem*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0')                */
/* "SeqBytes*" és un vector de chars qualsevol (recordeu que en C, un     */
/* char és un enter de 8 bits) d'una longitud <= LongSeqBytes bytes       */
/* Retorna -1 si hi ha error; el nombre de bytes rebuts si tot va bé.     */
int UDP_RepDe(int Sck, char *IPrem, int *portUDPrem, char *SeqBytes, int LongSeqBytes)
{
    struct sockaddr_in aux;
    socklen_t len = sizeof(aux);
    int n=recvfrom (Sck, SeqBytes, LongSeqBytes, 0,&aux, &len);
	strcpy(IPrem,inet_ntoa(aux.sin_addr));
	*portUDPrem=ntohs(aux.sin_port);
	return n;
}

/* S’allibera (s’esborra) el socket UDP d’identificador “Sck”.            */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int UDP_TancaSock(int Sck)
{
		 return close(Sck);
}


/* Donat el socket UDP d’identificador “Sck”, troba l’adreça d’aquest     */
/* socket, omplint “IPloc*” i “portUDPloc*” amb respectivament, la seva   */
/* @IP i #port UDP.                                                       */
/* "IPloc*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0')                */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int UDP_TrobaAdrSockLoc(int Sck, char *IPloc, int *portUDPloc)
{
	struct sockaddr_in Adr;
	socklen_t longs = sizeof(Adr); 
	int n=getsockname(Sck, (struct sockaddr *)&Adr, &longs);
	strcpy(IPloc,inet_ntoa(Adr.sin_addr));
	*portUDPloc=ntohs(Adr.sin_port);	
	return n;
}


/* Examina simultàniament durant "Temps" (en [ms] els sockets (poden ser  */
/* TCP, UDP i stdin) amb identificadors en la llista “LlistaSck” (de      */
/* longitud “LongLlistaSck” sockets) per saber si hi ha arribat alguna    */
/* cosa per ser llegida. Si Temps és -1, s'espera indefinidament fins que */
/* arribi alguna cosa.                                                    */
/* "LlistaSck" és un vector d'enters d'una longitud >= LongLlistaSck      */
/* Retorna -1 si hi ha error; retorna -2 si passa "Temps" sense que       */
/* arribi res; si arriba alguna cosa per algun dels sockets, retorna      */
/* l’identificador d’aquest socket.                                       */
int HaArribatAlgunaCosaEnTemps(const int *LlistaSck, int LongLlistaSck, int Temps)
{
	struct timeval tmp;
	fd_set conjunt;int i,aux;

	tmp.tv_sec = Temps;
    tmp.tv_usec = 0;
	FD_ZERO(&conjunt);
	for (i=0;i<LongLlistaSck;i++) FD_SET(LlistaSck[i],&conjunt);

	if (Temps==-1) select(LlistaSck[LongLlistaSck-1]+1, &conjunt, NULL, NULL, NULL);
	else  aux=select(LlistaSck[LongLlistaSck-1]+1, &conjunt, NULL, NULL, &tmp);
	for (i=0;i<LongLlistaSck;i++) if (FD_ISSET(LlistaSck[i],&conjunt)) return LlistaSck[i];
	if (aux==0) return -2;
	else if (aux<0) return -1;
	return -1;
}

/* Donat el nom DNS "NomDNS" obté la corresponent @IP i l'escriu a "IP*"  */
/* "NomDNS" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud qualsevol, i "IP*" és un "string" de C (vector de */
/* chars imprimibles acabat en '\0') d'una longitud màxima de 16 chars    */
/* (incloent '\0').                                                       */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé     */
int ResolDNSaIP(const char *NomDNS, char *IP)
{
	struct hostent* hp;
	struct sockaddr_in adr;
	hp=gethostbyname(NomDNS); //Consultem nom DNS
	if(hp == NULL){ //Si no es pot reconèixer
		strcpy(IP, "");
		return -1;
	}
	else{ //Si es pot reconeixer, guardem a ip la IP corresponent.
		memcpy((void *)&adr.sin_addr, hp->h_addr_list[0], hp->h_length);
		strcpy(IP,inet_ntoa(adr.sin_addr));
		return 1;
	}
}

/* Crea un fitxer de "log" de nom "NomFitxLog".                           */
/* "NomFitxLog" és un "string" de C (vector de chars imprimibles acabat   */
/* en '\0') d'una longitud qualsevol.                                     */
/* Retorna -1 si hi ha error; l'identificador del fitxer creat si tot va  */
/* bé.                                                                    */
int Log_CreaFitx(const char *NomFitxLog)
{
	FILE *fp;
 	fp = fopen(NomFitxLog,"w");
 	fputs("I:fitxer creat\n", fp ); 	
	return (int)fp;	
}

/* Escriu al fitxer de "log" d'identificador "FitxLog" el missatge de     */
/* "log" "MissLog".                                                       */
/* "MissLog" és un "string" de C (vector de chars imprimibles acabat      */
/* en '\0') d'una longitud qualsevol.                                     */
/* Retorna -1 si hi ha error; el nombre de caràcters del missatge de      */
/* "log" (sense el '\0') si tot va bé                                     */
int Log_Escriu(int FitxLog, const char *MissLog)
{
	return fputs(MissLog, (FILE*)FitxLog);
}

/* Tanca el fitxer de "log" d'identificador "FitxLog".                    */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int Log_TancaFitx(int FitxLog)
{
	return fclose((FILE *)FitxLog);
}

int obtenirIPlocal(char * IPloc){
	struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *addr;
    getifaddrs (&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family==AF_INET) {
            sa = (struct sockaddr_in *) ifa->ifa_addr;
            addr = inet_ntoa(sa->sin_addr);
            if(ifa->ifa_name[0]=='e') {strcpy(IPloc,addr);freeifaddrs(ifap);return 1;}
        }
    }
    freeifaddrs(ifap);
	return 1;
}
