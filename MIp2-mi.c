/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer mi.c que implementa la capa d'aplicació de MI, sobre la capa de */
/* transport TCP (fent crides a la interfície de la capa TCP -sockets-).  */
/* Autors: X, Y                                                           */
/*                                                                        */
/**************************************************************************/

/* Inclusió de llibreries, p.e. #include <sys/types.h> o #include "meu.h" */
/*  (si les funcions externes es cridessin entre elles, faria falta fer   */
/*   un #include "MIp2-mi.h")                                             */
#include "MIp2-mi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
/* Definició de constants, p.e., #define MAX_LINIA 150                    */

/* Declaració de funcions internes que es fan servir en aquest fitxer     */
/* (les seves definicions es troben més avall) per així fer-les conegudes */
/* des d'aqui fins al final de fitxer.                                    */
int TCP_CreaSockClient(const char *IPloc, int portTCPloc);
int TCP_CreaSockServidor(const char *IPloc, int portTCPloc);
int TCP_DemanaConnexio(int Sck, const char *IPrem, int portTCPrem);
int TCP_AcceptaConnexio(int Sck, char *IPrem, int *portTCPrem);
int TCP_Envia(int Sck, const char *SeqBytes, int LongSeqBytes);
int TCP_Rep(int Sck, char *SeqBytes, int LongSeqBytes);
int TCP_TancaSock(int Sck);
int TCP_TrobaAdrSockLoc(int Sck, char *IPloc, int *portTCPloc);
int TCP_TrobaAdrSockRem(int Sck, char *IPrem, int *portTCPrem);
int HaArribatAlgunaCosa(const int *LlistaSck, int LongLlistaSck);

int Nick_Envia(int Sck, const char *SeqBytes, int LongSeqBytes);
int Nick_Rep(int Sck, char *SeqBytes, int LongSeqBytes);
/* Definicio de funcions EXTERNES, és a dir, d'aquelles que en altres     */
/* fitxers externs es faran servir.                                       */
/* En termes de capes de l'aplicació, aquest conjunt de funcions externes */
/* formen la interfície de la capa MI.                                    */

/* Inicia l’escolta de peticions remotes de conversa a través d’un nou    */
/* socket TCP en el #port “portTCPloc” i una @IP local qualsevol (és a    */
/* dir, crea un socket “servidor” o en estat d’escolta – listen –).       */
/* Retorna -1 si hi ha error; l’identificador del socket d’escolta de MI  */
/* creat si tot va bé.                                                    */
int MI_IniciaEscPetiRemConv(int portTCPloc)
{
	return TCP_CreaSockServidor("0.0.0.0",portTCPloc);
}

/* Escolta indefinidament fins que arriba una petició local de conversa   */
/* a través del teclat o bé una petició remota de conversa a través del   */
/* socket d’escolta de MI d’identificador “SckEscMI” (un socket           */
/* “servidor”).                                                           */
/* Retorna -1 si hi ha error; 0 si arriba una petició local; SckEscMI si  */
/* arriba una petició remota.                                             */
int MI_HaArribatPetiConv(int SckEscMI)
{
	int llistaS[2]={0,SckEscMI};
	return HaArribatAlgunaCosa(llistaS,2);
}

/* Crea una conversa iniciada per una petició local que arriba a través   */
/* del teclat: crea un socket TCP “client” (en un #port i @IP local       */
/* qualsevol), a través del qual fa una petició de conversa a un procés   */
/* remot, el qual les escolta a través del socket TCP ("servidor") d'@IP  */
/* “IPrem” i #port “portTCPrem” (és a dir, crea un socket “connectat” o   */
/* en estat establert – established –). Aquest socket serà el que es farà */
/* servir durant la conversa.                                             */
/* Omple “IPloc*” i “portTCPloc*” amb, respectivament, l’@IP i el #port   */
/* TCP del socket del procés local.                                       */
/* El nickname local “NicLoc” i el nickname remot són intercanviats amb   */
/* el procés remot, i s’omple “NickRem*” amb el nickname remot. El procés */
/* local és qui inicia aquest intercanvi (és a dir, primer s’envia el     */
/* nickname local i després es rep el nickname remot).                    */
/* "IPrem" i "IPloc*" són "strings" de C (vectors de chars imprimibles    */
/* acabats en '\0') d'una longitud màxima de 16 chars (incloent '\0').    */
/* "NicLoc" i "NicRem*" són "strings" de C (vectors de chars imprimibles  */
/* acabats en '\0') d'una longitud màxima de 300 chars (incloent '\0').   */
/* Retorna -1 si hi ha error; l’identificador del socket de conversa de   */
/* MI creat si tot va bé.                                                 */
int MI_DemanaConv(const char *IPrem, int portTCPrem, char *IPloc, int *portTCPloc, const char *NicLoc, char *NicRem)
{
	bzero(NicRem,sizeof(NicRem));
	int cfd=TCP_CreaSockClient("0.0.0.0", 0);
	if (cfd<0) {
		perror("Error al crear socket client. ");
		return (-1);
	}
	if (TCP_DemanaConnexio(cfd,IPrem,portTCPrem)<0) {
		perror("Error al conectar-se. ");
		return (-1);
	}
	if (Nick_Envia(cfd,NicLoc,strlen(NicLoc))<0) {
		perror("Error al enviar Nickname. ");	
		return(-1);
	}
	if (Nick_Rep(cfd,NicRem,300)!=1) {
		perror("Error al rebre Nickname. ");	
		return(-1);
	}
	if (TCP_TrobaAdrSockLoc(cfd,IPloc,*&portTCPloc)<0) {
		perror("Error al obtenir informació local. ");	
		return(-1);
	}
	return cfd;
}

/* Crea una conversa iniciada per una petició remota que arriba a través  */
/* del socket d’escolta de MI d’identificador “SckEscMI” (un socket       */
/* “servidor”): accepta la petició i crea un socket (un socket            */
/* “connectat” o en estat establert – established –), que serà el que es  */
/* farà servir durant la conversa.                                        */
/* Omple “IPrem*”, “portTCPrem*”, “IPloc*” i “portTCPloc*” amb,           */
/* respectivament, l’@IP i el #port TCP del socket del procés remot i del */
/* socket del procés local.                                               */
/* El nickname local “NicLoc” i el nickname remot són intercanviats amb   */
/* el procés remot, i s’omple “NickRem*” amb el nickname remot. El procés */
/* remot és qui inicia aquest intercanvi (és a dir, primer es rep el      */
/* nickname remot i després s’envia el nickname local).                   */
/* "IPrem*" i "IPloc*" són "strings" de C (vectors de chars imprimibles   */
/* acabats en '\0') d'una longitud màxima de 16 chars (incloent '\0').    */
/* "NicLoc" i "NicRem*" són "strings" de C (vectors de chars imprimibles  */
/* acabats en '\0') d'una longitud màxima de 300 chars (incloent '\0').   */
/* Retorna -1 si hi ha error; l’identificador del socket de conversa      */
/* de MI creat si tot va bé.                                              */
int MI_AcceptaConv(int SckEscMI, char *IPrem, int *portTCPrem, char *IPloc, int *portTCPloc, const char *NicLoc, char *NicRem)
{
	bzero(NicRem,sizeof(NicRem));
	int cfd=TCP_AcceptaConnexio(SckEscMI,IPrem, *&portTCPrem);
	if (Nick_Rep(cfd,NicRem,300)!=1) {
		perror("Error al rebre Nickname. ");	
		return(-1);
	}
	if (Nick_Envia(cfd,NicLoc,strlen(NicLoc))<0) {
		perror("Error al enviar Nickname. ");	
		return(-1);
	}
	if (TCP_TrobaAdrSockLoc(cfd,IPloc,*&portTCPloc)<0) {
		perror("Error al obtenir informació local. ");	
		return(-1);
	}
	if (TCP_TrobaAdrSockRem(cfd,IPrem,*&portTCPrem)<0) {
		perror("Error al obtenir informació local. ");	
		return(-1);
	}
	return cfd;
}

/* Escolta indefinidament fins que arriba una línia local de conversa a   */
/* través del teclat o bé una línia remota de conversa a través del       */
/* socket de conversa de MI d’identificador “SckConvMI” (un socket        */
/* "connectat”).                                                          */
/* Retorna -1 si hi ha error; 0 si arriba una línia local; SckConvMI si   */
/* arriba una línia remota.                                               */
int MI_HaArribatLinia(int SckConvMI)
{
	int llistaS[2]={0,SckConvMI};
	return HaArribatAlgunaCosa(llistaS,2);
}

/* Envia a través del socket de conversa de MI d’identificador            */
/* “SckConvMI” (un socket “connectat”) la línia “Linia” escrita per       */
/* l’usuari local.                                                        */
/* "Linia" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0'), no conté el caràcter fi de línia ('\n') i té una longitud       */
/* màxima de 300 chars (incloent '\0').                                   */
/* Retorna -1 si hi ha error; el nombre de caràcters n de la línia        */
/* enviada (sense el ‘\0’) si tot va bé (0 <= n <= 299).                  */
int MI_EnviaLinia(int SckConvMI, const char *Linia)
{
	char missatge[300];
	bzero(missatge,300);
	missatge[0]='L';
	strcat(missatge, Linia);
	return TCP_Envia(SckConvMI,missatge,strlen(missatge)-1);
}

/* Rep a través del socket de conversa de MI d’identificador “SckConvMI”  */
/* (un socket “connectat”) una línia escrita per l’usuari remot, amb la   */
/* qual omple “Linia”, o bé detecta l’acabament de la conversa per part   */
/* de l’usuari remot.                                                     */
/* "Linia*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0'), no conté el caràcter fi de línia ('\n') i té una longitud       */
/* màxima de 300 chars (incloent '\0').                                   */
/* Retorna -1 si hi ha error; -2 si l’usuari remot acaba la conversa; el  */
/* nombre de caràcters n de la línia rebuda (sense el ‘\0’) si tot va bé  */
/* (0 <= n <= 299).                                                       */
int MI_RepLinia(int SckConvMI, char *Linia)
{
	int r;int i;char missatge[300];
	bzero(missatge,300);bzero(Linia,sizeof(Linia));
	r=TCP_Rep(SckConvMI,missatge,300);
	if (r < 0) {
		perror("ERROR al llegir de socket. ");
		return(-1);
	}else if (r==0)  {     
		return (-2);
	}else if (missatge[0]=='L') {
		i=0;
		for (i=1;i<r;i++) Linia[i-1]=missatge[i];
		return r;
	}else {
		perror("ERROR: codi de missatge desconegut, s'ignora el missatge.");
		return -1;
	}
}

/* Acaba la conversa associada al socket de conversa de MI                */
/* d’identificador “SckConvMI” (un socket “connectat”).                   */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int MI_AcabaConv(int SckConvMI)
{
	return TCP_TancaSock(SckConvMI);
}

/* Acaba l’escolta de peticions remotes de conversa que arriben a través  */
/* del socket d’escolta de MI d’identificador “SckEscMI” (un socket       */
/* “servidor”).                                                           */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int MI_AcabaEscPetiRemConv(int SckEscMI)
{
	return TCP_TancaSock(SckEscMI);
}


/* Definicio de funcions INTERNES, és a dir, d'aquelles que es faran      */
/* servir només en aquest mateix fitxer.                                  */

/* Crea un socket TCP “client” a l’@IP “IPloc” i #port TCP “portTCPloc”   */
/* (si “IPloc” és “0.0.0.0” i/o “portTCPloc” és 0 es fa/farà una          */
/* assignació implícita de l’@IP i/o del #port TCP, respectivament).      */
/* "IPloc" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; l’identificador del socket creat si tot     */
/* va bé.                                                                 */
int TCP_CreaSockClient(const char *IPloc, int portTCPloc)
{
	struct sockaddr_in sAdr;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       return -1;
	bzero((char *) &sAdr, sizeof(sAdr)); //inicialitza tot a zeros
	sAdr.sin_family=AF_INET;
	sAdr.sin_port=htons(portTCPloc);
	sAdr.sin_addr.s_addr=inet_addr(IPloc);
	if (bind(sockfd, (struct sockaddr *) &sAdr,sizeof(sAdr)) < 0)
		return -1;
	return sockfd;
}

/* Crea un socket TCP “servidor” (o en estat d’escolta – listen –) a      */
/* l’@IP “IPloc” i #port TCP “portTCPloc” (si “IPloc” és “0.0.0.0” i/o    */
/* “portTCPloc” és 0 es fa una assignació implícita de l’@IP i/o del      */
/* #port TCP, respectivament).                                            */
/* "IPloc" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; l’identificador del socket creat si tot     */
/* va bé.                                                                 */
int TCP_CreaSockServidor(const char *IPloc, int portTCPloc)
{
	struct sockaddr_in sAdr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       return -1;
	bzero((char *) &sAdr, sizeof(sAdr)); //inicialitza tot a zeros
	sAdr.sin_family=AF_INET;
	sAdr.sin_port=htons(portTCPloc);
	sAdr.sin_addr.s_addr=inet_addr(IPloc);
	if (bind(sockfd, (struct sockaddr *) &sAdr,sizeof(sAdr)) < 0)
		return -1;
	listen(sockfd,5);
	return sockfd;
}

/* El socket TCP “client” d’identificador “Sck” demana una connexió al    */
/* socket TCP “servidor” d’@IP “IPrem” i #port TCP “portTCPrem” (si tot   */
/* va bé es diu que el socket “Sck” passa a l’estat “connectat” o         */
/* establert – established –).                                            */
/* "IPrem" és un "string" de C (vector de chars imprimibles acabat en     */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int TCP_DemanaConnexio(int Sck, const char *IPrem, int portTCPrem)
{
	struct sockaddr_in sAdr;
	bzero((char *) &sAdr, sizeof(sAdr)); //inicialitza tot a zeros
	sAdr.sin_family=AF_INET;
	sAdr.sin_port=htons(portTCPrem);
	sAdr.sin_addr.s_addr=inet_addr(IPrem);
	return (connect(Sck,(struct sockaddr *)&sAdr, sizeof(sAdr)));
}

/* El socket TCP “servidor” d’identificador “Sck” accepta fer una         */
/* connexió amb un socket TCP “client” remot, i crea un “nou” socket,     */
/* que és el que es farà servir per enviar i rebre dades a través de la   */
/* connexió (es diu que aquest nou socket es troba en l’estat “connectat” */
/* o establert – established –; el nou socket té la mateixa adreça que    */
/* “Sck”).                                                                */
/* Omple “IPrem*” i “portTCPrem*” amb respectivament, l’@IP i el #port    */
/* TCP del socket remot amb qui s’ha establert la connexió.               */
/* "IPrem*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; l’identificador del socket connectat creat  */
/* si tot va bé.                                                          */
int TCP_AcceptaConnexio(int Sck, char *IPrem, int *portTCPrem)
{
	struct sockaddr_in cliAdr;
	socklen_t clilen = sizeof(cliAdr);
    int csockfd = accept(Sck, (struct sockaddr *) &cliAdr, &clilen);
	strcpy(IPrem,inet_ntoa(cliAdr.sin_addr));
	*portTCPrem=ntohs(cliAdr.sin_port);
	return csockfd; 
}

/* Envia a través del socket TCP “connectat” d’identificador “Sck” la     */
/* seqüència de bytes escrita a “SeqBytes” (de longitud “LongSeqBytes”    */
/* bytes) cap al socket TCP remot amb qui està connectat.                 */
/* "SeqBytes" és un vector de chars qualsevol (recordeu que en C, un      */
/* char és un enter de 8 bits) d'una longitud >= LongSeqBytes bytes.      */
/* Retorna -1 si hi ha error; el nombre de bytes enviats si tot va bé.    */
int TCP_Envia(int Sck, const char *SeqBytes, int LongSeqBytes)
{
	return write(Sck,SeqBytes,LongSeqBytes);
}

/* Rep a través del socket TCP “connectat” d’identificador “Sck” una      */
/* seqüència de bytes que prové del socket remot amb qui està connectat,  */
/* i l’escriu a “SeqBytes*” (que té una longitud de “LongSeqBytes” bytes),*/
/* o bé detecta que la connexió amb el socket remot ha estat tancada.     */
/* "SeqBytes*" és un vector de chars qualsevol (recordeu que en C, un     */
/* char és un enter de 8 bits) d'una longitud <= LongSeqBytes bytes.      */
/* Retorna -1 si hi ha error; 0 si la connexió està tancada; el nombre de */
/* bytes rebuts si tot va bé.                                             */
int TCP_Rep(int Sck, char *SeqBytes, int LongSeqBytes)
{
	return read(Sck,SeqBytes,LongSeqBytes);
}

/* S’allibera (s’esborra) el socket TCP d’identificador “Sck”; si “Sck”   */
/* està connectat es tanca la connexió TCP que té establerta.             */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int TCP_TancaSock(int Sck)
{
	return close(Sck);
}

/* Donat el socket TCP d’identificador “Sck”, troba l’adreça d’aquest     */
/* socket, omplint “IPloc*” i “portTCPloc*” amb respectivament, la seva   */
/* @IP i #port TCP.                                                       */
/* "IPloc*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int TCP_TrobaAdrSockLoc(int Sck, char *IPloc, int *portTCPloc)
{
	struct sockaddr_in Adr;
	socklen_t longs = sizeof(Adr); 
	int n=getsockname(Sck, (struct sockaddr *)&Adr, &longs);
	strcpy(IPloc,inet_ntoa(Adr.sin_addr));
	*portTCPloc=ntohs(Adr.sin_port);
	return n;
}

/* Donat el socket TCP “connectat” d’identificador “Sck”, troba l’adreça  */
/* del socket remot amb qui està connectat, omplint “IPrem*” i            */
/* “portTCPrem*” amb respectivament, la seva @IP i #port TCP.             */
/* "IPrem*" és un "string" de C (vector de chars imprimibles acabat en    */
/* '\0') d'una longitud màxima de 16 chars (incloent '\0').               */
/* Retorna -1 si hi ha error; un valor positiu qualsevol si tot va bé.    */
int TCP_TrobaAdrSockRem(int Sck, char *IPrem, int *portTCPrem)
{
	struct sockaddr_in Adr;
	socklen_t longs = sizeof(Adr); 
	int n=getpeername(Sck, (struct sockaddr *)&Adr, &longs);
	strcpy(IPrem,inet_ntoa(Adr.sin_addr));
	*portTCPrem=ntohs(Adr.sin_port);
	return n;
}

/* Examina simultàniament i sense límit de temps (una espera indefinida)  */
/* els sockets (poden ser TCP, UDP i stdin) amb identificadors en la      */
/* llista “LlistaSck” (de longitud “LongLlistaSck” sockets) per saber si  */
/* hi ha arribat alguna cosa per ser llegida.                             */
/* "LlistaSck" és un vector d'enters d'una longitud >= LongLlistaSck      */
/* Retorna -1 si hi ha error; si arriba alguna cosa per algun dels        */
/* sockets, retorna l’identificador d’aquest socket.                      */
int HaArribatAlgunaCosa(const int *LlistaSck, int LongLlistaSck)
{
	fd_set conjunt;int i=0;
	FD_ZERO(&conjunt);
	for (i=0;i<LongLlistaSck;i++) FD_SET(LlistaSck[i],&conjunt);
	select(LlistaSck[LongLlistaSck-1]+1, &conjunt, NULL, NULL, NULL);
	for (i=0;i<LongLlistaSck;i++) if (FD_ISSET(LlistaSck[i],&conjunt)) return LlistaSck[i];
	return -1;
}

/* Si ho creieu convenient, feu altres funcions...                        */
/* Envia a través del socket TCP “connectat” d’identificador “Sck” la     */
/* seqüència de bytes escrita a “Nick” (de longitud “LongNick”            */
/* bytes) cap al socket TCP remot amb qui està connectat.                 */
/* "Nick" és un vector de chars que conté el nickname que s'enviarà.      */
/* Seguint el protocol s'enviara un missatge amb una 'N' inicial          */
/* seguida per cadena la cadena Nick que conte el nickname.               */
/* Retorna -1 si hi ha error; el nombre de bytes enviats si tot va bé.    */
int Nick_Envia(int Sck, const char *Nick, int LongNick)
{
	char missatge[300];
	bzero(missatge,300);
	missatge[0]='N';
	strcat(missatge, Nick);
	return TCP_Envia(Sck,missatge,LongNick);
}

/* Rep a través del socket TCP “connectat” d’identificador “Sck” una      */
/* seqüència de bytes que prové del socket remot amb qui està connectat,  */
/* i l’escriu a “Nick*” (que té una longitud de “LongNick” bytes),        */
/* "Nick*" és un vector de chars qualsevol.                               */
/* Retorna -1 si hi ha error; 1 si tot va be.                             */
int Nick_Rep(int Sck, char *Nick, int LongNick)
{
	int r;int i;char missatge[300];
	bzero(missatge,300);bzero(Nick,LongNick);
	r=TCP_Rep(Sck,missatge,LongNick);
	if (r < 0) {
		perror("ERROR al llegir  Nickname. ");
		return(-1);
	}else if (missatge[0]!='N') {
		perror("ERROR: s'esperava Nickname. ");
		return(-1);
	}
	for (i=1;i<r;i++) Nick[i-1]=missatge[i];
	return 1;
}

