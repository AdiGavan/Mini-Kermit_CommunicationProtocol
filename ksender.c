// "Copyright [2018] Gavan Adrian-George, 324CA"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

/* Functia trimite un pachet si asteapta raspuns de la kreceiver.
   Pachetele se retrimit de inca 3 ori. Daca tot nu primeste raspuns, se
   returneaza -1. Altfel, se returneaza noua secventa. */
int trimite_si_asteapta_ACK(msg* t, int seq, int dimensiune_date, int timeout) {
  int contor_retrimiteri = 0;
  unsigned short crc = 0;

  while(1){
    send_message(t);
    msg *y = receive_message_timeout(timeout * MS);

    // Cazul cand pachetul nu a fost primit.
    if (y == NULL) {
      contor_retrimiteri ++;

      // Daca pachetul a fost retransmis de mai mult de 3 ori se incheie
      // conexiunea.
      if (contor_retrimiteri > 3) {
        printf("S: Un pachet pierdut a fost retrimis de mai mult de 3 ori! \n");
        return -1;
      }

      // Cazul cand ksender a primit ACK sau NACK.
    } else {
      contor_retrimiteri = 0;

      // Cazul cand ksender primeste ACK.
      if (y->payload[3] == 'Y') {

        // Se actualizeaza noua secventa si se iese din bucla.
        seq = ((int)y->payload[2] + 1) % VAL_MOD;
        break;

        // Cazul cand ksender primeste NAK
      } else if (y->payload[3] == 'N') {

        // Se actualizeaza secventa si crc si se pun in payload.
        // Programul va intra iar in bucla si va retransmite noul pachet.
        seq = (seq + 2) % VAL_MOD;
        t->payload[2] = seq;
        crc = crc16_ccitt(t->payload, 4 + dimensiune_date);
        memcpy(t->payload + 4 + dimensiune_date, &crc, 2);
      }
    }
  }
  return seq;
}

int main(int argc, char** argv) {
    msg t;
    init(HOST, PORT);

    int seq = 0;
    int timeout = 0;
    int file = 0;
    int octeti_cititi;
    int contor_retrimiteri = 0;
    unsigned char maxl = 0;
    unsigned short crc = 0;

    // Se trimite pachetul Send-Init.
    creeaza_pachete_SendInit(&t, seq, 'S');

    while (1) {
      send_message(&t);
      msg *y = receive_message_timeout(TIME * MS);

      // Cazul cand pachetul nu a fost primit.
      if (y == NULL) {
        contor_retrimiteri ++;

        // Daca pachetul a fost trimis de mai mult de 3 ori se incheie
        // conexiunea (kreceiver a asteptat de 3 ori TIME).
        if (contor_retrimiteri >= 3) {
          printf("S: Pachetul S a fost trimis de 3 ori fara raspuns! \n");
          return -1;
        }
        // Cazul cand ksender a primit ACK sau NACK.
      } else {
        contor_retrimiteri = 0;

        // Cazul cand ksender primeste ACK
        if (y->payload[3] == 'Y') {

          // Se actualizeaza noua secventa, dimensiunea maxima a campului Data,
          // durata de timeout si se iese din bucla.
          timeout = (int)y->payload[5];
          seq = ((int)y->payload[2] + 1) % VAL_MOD;
          maxl = (unsigned char)y->payload[4];
          break;

          // Cazul cand ksender primeste NAK
        } else if (y->payload[3] == 'N') {

          // Se actualizeaza secventa si crc si se pun in payload.
          // Programul va intra iar in bucla si va retransmite noul pachet.
          seq = (seq + 2) % VAL_MOD;
          t.payload[2] = seq;
          crc = crc16_ccitt(t.payload, LENGTHSENDINITCHECK);
          memcpy(t.payload + LENGTHSENDINITCHECK, &crc, 2);
        }
      }
    }

    // Se reseteaza payload-ul lui t.
    memset(t.payload, 0, 1400);

    // Se initializeaza un buffer pentru citirea datelor din fisiere.
    char* buffer = (char*)malloc(maxl * sizeof(char));

    // Se face un for pentru a trimite fiecare fisier.
    for (int i = 1; i < argc; i++) {

      // Se trimite File Header-ul
      creeaza_pachete_Header(&t, seq, 'F', argv[i]);
      seq = trimite_si_asteapta_ACK(&t, seq, strlen(argv[i]), timeout);
      if (seq == -1) {
        free(buffer);
        return -1;
      }

      // Se deschide fisierul corespunzator.
      file = open(argv[i], O_RDONLY, 0777);
      if (file < 0) {
        printf("EROARE - Fisierul nu a putut fi deschis! \n");
        free(buffer);
        return -1;
      }

      // Se citesc datele si se transmit pachetele de tip "D".
      while((octeti_cititi = read(file, buffer, maxl)) > 0) {
        creeaza_pachete_Date(&t, seq, 'D', octeti_cititi, buffer);
        seq = trimite_si_asteapta_ACK(&t, seq, octeti_cititi, timeout);
        if (seq == -1) {
          close(file);
          free(buffer);
          return -1;
        }

        // Se reseteaza buffer-ul si payload-ul lui t.
        memset(buffer, 0, maxl);
        memset(t.payload, 0, 1400);
      }

      // Dupa ce toate datele au fost trimise, se trimite packetul de tip "Z".
      creeaza_pachete_data_vid(&t, seq, 'Z');
      seq = trimite_si_asteapta_ACK(&t, seq, 0, timeout);
      if (seq == -1) {
        close(file);
        free(buffer);
        return -1;
      }

      // Se inchide fisierul si se reseteaza payload-ul lui t si buffer-ul.
      close(file);
      memset(buffer, 0, maxl);
      memset(t.payload, 0, 1400);
    }

    // Dupa ce s-au trimis toate fisierele, se trimite pachetul de tip "B".
    creeaza_pachete_data_vid(&t, seq, 'B');
    seq = trimite_si_asteapta_ACK(&t, seq, 0, timeout);
    if (seq == -1) {
      free(buffer);
      return -1;
    }
    free(buffer);

    return 0;
}
