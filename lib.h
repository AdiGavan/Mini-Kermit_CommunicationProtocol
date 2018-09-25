// "Copyright [2018] Gavan Adrian-George, 324CA"
#ifndef LIB
#define LIB
#define MAXL 250
#define TIME 5
// Lungime "S" fara primele 2 campuri.
#define LENGTHSENDINIT 16
// Lungime packet fara DATA si primele 2 campuri.
#define LENGTHMINIKERMIT 5
// Lungime DATA "S".
#define LENGTHDATAINIT 11
// Lungime "S" pana in CHECK.
#define LENGTHSENDINITCHECK 15
// Lungime pachet cu DATA vid (1 octet) si fara primele 2 campuri.
#define LENGTHDATAVID 5
// Lungime pachet pana in CHECK cu DATA vid.
#define LENGTHDATAVIDCHECK 4
// Lungimea pachetului pana in DATA.
#define LEN_BEFORE_DATA 4
// Milisecunde.
#define MS 1000
// Numarul pentru modul.
#define VAL_MOD 64

typedef struct {
    int len;
    char payload[1400];
} msg;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

// Structura pentru un pachet MINI-KERMIT;
typedef struct {
    char soh;
    char len;
    char seq;
    char type;
    char data[MAXL];
    unsigned short check;
    char mark;
} __attribute__((__packed__)) PacketKermit;

// Structura pentru campul DATA al unui pachet "S";
typedef struct {
    char maxl;
    char time;
    char npad;
    char padc;
    char eol;
    char qctl;
    char qbin;
    char chkt;
    char rept;
    char capa;
    char r;
} __attribute__((__packed__)) DataSendInit;

// Functie pentru initializarea unui pachet de tip "S";
void initializareDataSendInit (DataSendInit* packet) {
  (*packet).maxl = MAXL;
  (*packet).time = TIME;
  (*packet).npad = 0x00;
  (*packet).padc = 0x00;
  (*packet).eol = 0x0D;
  (*packet).qctl = 0x00;
  (*packet).qbin = 0x00;
  (*packet).chkt = 0x00;
  (*packet).rept = 0x00;
  (*packet).capa = 0x00;
  (*packet).r = 0x00;
}

/* Functie care creeaza un pachet MINI-KERMIT cu campul DATA vid;
   Functia este folosita pentru pachetele de tip "Z", "B", "Y", "N". */
void creeaza_pachete_data_vid(msg* t, int seq, char tip) {
  PacketKermit packet;
  DataSendInit dateInit;
  unsigned short crc;

  // Se initializeaza un pachet DataSendInit pentru a avea valoarea EOL.
  initializareDataSendInit (& dateInit);
  packet.soh = 0x01;
  packet.mark = dateInit.eol;

  // Dimensiune cand DATA este vid.
  packet.len = LENGTHDATAVID;
  packet.seq = seq;
  packet.type = tip;

  // Se pun campurile SOH, LEN, SEQ si TYPE in pachetul de tip "msg".
  memcpy(t->payload, &packet, LEN_BEFORE_DATA);

  // Campul DATA este vid, asa ca campul DATA nu va exista.
  // Se calculeaza suma de control si se pune in msg. Se pune si MARK in msg.
  crc = crc16_ccitt(t->payload, LENGTHDATAVIDCHECK);
  packet.check = crc;
  memcpy(t->payload + LENGTHDATAVIDCHECK , &packet.check, 2);
  memcpy(t->payload + LENGTHDATAVIDCHECK + 2, &packet.mark, 1);

  // Se actualizeaza lungimea payload-ului pachetului msg.
  t->len = packet.len + 2;
}

// Functie care creeaza un pachet MINI-KERMIT de tipul "S".
void creeaza_pachete_SendInit(msg* t, int seq, char tip) {
  PacketKermit packet;
  DataSendInit dateInit;
  unsigned short crc;

  // Se initializeaza un pachet DataSendInit pentru a avea valoarea EOL.
  initializareDataSendInit (& dateInit);
  packet.soh = 0x01;
  packet.mark = dateInit.eol;

  // Dimensiune cand se iau 11 octeti in data (pentru "S").
  packet.len = LENGTHSENDINIT;
  packet.seq = seq;
  packet.type = tip;

  // Se pun campurile completate in payload-ul structuri de tip msg.
  memcpy(packet.data, &dateInit, LENGTHDATAINIT);
  memcpy(t->payload, &packet, LENGTHSENDINITCHECK);

  // Se calculeaza suma de control si se pune in msg. Se pune si MARK in msg.
  crc = crc16_ccitt(t->payload, LENGTHSENDINITCHECK);
  packet.check = crc;
  memcpy(t->payload + LENGTHSENDINITCHECK, &packet.check, 2);
  memcpy(t->payload + LENGTHSENDINITCHECK + 2, &packet.mark, 1);

  // Se actualizeaza lungimea payload-ului pachetului msg.
  t->len = packet.len + 2;
}

// Functie care creeaza un pachet MINI-KERMIT de tipul "F".
void creeaza_pachete_Header(msg* t, int seq, char tip, char* argv) {
  PacketKermit packet;
  DataSendInit dateInit;
  unsigned short crc;

  // Se retine lungimea numelui fisierului.
  int lungime_date = strlen(argv);

  // Se initializeaza un pachet DataSendInit pentru a avea valoarea EOL.
  initializareDataSendInit (& dateInit);
  packet.soh = 0x01;
  packet.mark = dateInit.eol;

  /* Dimensiune cand in DATA este numele fisierului
     si adunam 5 pentru campurile SEQ, TYPE, CHECK si MARK.*/
  packet.len = lungime_date + 5;
  packet.seq = seq;
  packet.type = tip;

  // Se pun campurile completate in payload-ul structuri de tip msg.
  memcpy(packet.data, argv, lungime_date);
  memcpy(t->payload, &packet, LEN_BEFORE_DATA + lungime_date);

  // Se calculeaza suma de control si se pune in msg. Se pune si MARK in msg.
  crc = crc16_ccitt(t->payload, LEN_BEFORE_DATA + lungime_date);
  packet.check = crc;
  memcpy(t->payload + LEN_BEFORE_DATA + lungime_date, &packet.check, 2);
  memcpy(t->payload + LEN_BEFORE_DATA + lungime_date + 2, &packet.mark, 1);

  // Se actualizeaza lungimea payload-ului pachetului msg.
  t->len = packet.len + 2;
}

// Functie care creeaza un pachet MINI-KERMIT de tipul "D".
void creeaza_pachete_Date(msg* t, int seq, char tip,
                          int octeti_cititi, char* buffer) {

  PacketKermit packet;
  DataSendInit dateInit;
  unsigned short crc;

  // Se initializeaza un pachet DataSendInit pentru a avea valoarea EOL.
  initializareDataSendInit (& dateInit);
  packet.soh = 0x01;
  packet.mark = dateInit.eol;

  /* Dimensiune cand in DATA sunt octeti cititi din fisier
     si adunam 5 pentru campurile SEQ, TYPE, CHECK si MARK.*/
  packet.len = octeti_cititi + 5;
  packet.seq = seq;
  packet.type = tip;

  // Se pun campurile completate in payload-ul structuri de tip msg.
  memcpy(packet.data, buffer, octeti_cititi);
  memcpy(t->payload, &packet, LEN_BEFORE_DATA + octeti_cititi);

  // Se calculeaza suma de control si se pune in msg. Se pune si MARK in msg.
  crc = crc16_ccitt(t->payload, LEN_BEFORE_DATA + octeti_cititi);
  packet.check = crc;
  memcpy(t->payload + LEN_BEFORE_DATA + octeti_cititi, &packet.check, 2);
  memcpy(t->payload + LEN_BEFORE_DATA + octeti_cititi + 2, &packet.mark, 1);

  // Se actualizeaza lungimea payload-ului pachetului msg.
  t->len = octeti_cititi + 5 + 2;
}

#endif
