/**************************************************************************/
/*                                                                        */
/* P2 - MI amb sockets TCP/IP - Part II                                   */
/* Fitxer capçalera de lumi.c                                             */
/*                                                                        */
/* Autors: X, Y                                                           */
/*                                                                        */
/**************************************************************************/

struct usuari {
   char nom[30];
   char IP[16];
   int portUDP;
   char estat;
};

/* Declaració de funcions externes de lumi.c, és a dir, d'aquelles que es */
/* faran servir en un altre fitxer extern, p.e., MIp2-p2p.c,              */
/* MIp2-nodelumi.c, o MIp2-agelumic.c. El fitxer extern farà un #include  */
/* del fitxer .h a l'inici, i així les funcions seran conegudes en ell.   */
/* En termes de capes de l'aplicació, aquest conjunt de funcions externes */
/* formen la interfície de la capa LUMI.                                  */
/* Les funcions externes les heu de dissenyar vosaltres...                */
int LUMI_EngegarClient(char * adrecaMI,int *fd);
int LUMI_registrarClient(int sck,char *buffer,int fd);
int LUMI_desregistrarClient(int sck,char *buffer,int fd);
int LUMI_peticioLocClient(int sckUDP,char *MIrem,char *adrecaMI,int fd,char *IPrem,int *portR);
int LUMI_respondrePeticioClient(int sck,int PortTCP,int scKTCP,int fd);
int LUMI_EngegarServidor(int sckL,struct usuari *tu,int numUsuaris,char * domini);



