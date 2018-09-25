// "Copyright [2018] Gavan Adrian-George, 324CA"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

// Functie pentru refacerea crc-ului primit in pachetul de la ksender.
unsigned short luat_crc_ksender (msg* r) {
  int dimensiune_date = r->len - 7;
  unsigned short crc_sender = 0;
  crc_sender= ((unsigned char)r->payload[5 + dimensiune_date] << 8)
              | (unsigned char)r->payload[4 + dimensiune_date];
  return crc_sender;
}

int main(int argc, char** argv) {
    msg t;
    msg *r;
    init(HOST, PORT);

    // Se porneste cu seq de la minus 1 pentru ca in primul while fie se va
    // aduna 2 la secventa fie se va folosi secventa primita din ksender.
    int seq = -1;
    int contor_timeout = 0;
    int contor_primiri = 0;
    int timeout = 0;
    int expected_seq;
    int fisier = 0;
    char* numeFisier = NULL;
    unsigned short crc;
    unsigned short crc_sender;

    // Se primeste Send-Init.
    while(1) {
      r = receive_message_timeout(TIME * MS);

      // Daca nu a asteptat pachetul de 3 ori si nu l-a primit se inchide
      // conexiunea.
      if (r == NULL) {
        contor_timeout++;
        if (contor_timeout >= 3) {
          printf("R: Pachetul S a fost asteptat de 3 ori cate TIME! \n");
          return -1;
        }
        // Cazul cand kreceiver primeste pachetul.
      } else {

        // Se primeste Send-Init si se verifica corectitudinea pachetului.
        contor_timeout = 0;
        crc = crc16_ccitt(r->payload, LENGTHSENDINITCHECK);
        crc_sender = luat_crc_ksender(r);

        // Daca crc nu este ok, se trimite NAK si se intra iar in while.
        if(crc != crc_sender) {
          seq = (seq + 2) % VAL_MOD;
          creeaza_pachete_data_vid(&t, seq, 'N');
          send_message(&t);

          // Daca crc este ok se trimite un ACK cu informatiile de la kreceiver.
        } else {
          seq = ((int)r->payload[2] + 1) % VAL_MOD;
          timeout = (int)r->payload[5];
          creeaza_pachete_SendInit(&t, seq, 'Y');
          send_message(&t);
          break;
        }
      }
    }
    // Se reseteaza cele 2 payload-uri.
    memset(t.payload, 0, 1400);
    memset(r->payload, 0, 1400);

    // Se receptioneaza fisierele.
    while(1) {
      r = receive_message_timeout(timeout  * MS);

      // Cazul cand pachetul nu a fost primit.
      if (r == NULL) {
        contor_primiri++;

        // Daca pachetul a fost retransmis de mai mult de 3 ori si nu a
        // fost primit se incheie conexiunea.
        if (contor_primiri > 3) {
          printf("R: Un pachet a fost retrimis de mai mult de 3 ori! \n");
          free(numeFisier);
          close(fisier);
          return -1;
        }
        // Cazul cand primeste pachetele.
      } else {

        // Se calculeaza noul crc si vechiul crc.
        crc = crc16_ccitt(r->payload, r->len - 3);
        crc_sender = luat_crc_ksender(r);
        contor_primiri = 0;

        // Daca crc nu este ok, se trimite NAK si se intra iar in while.
        if(crc != crc_sender) {
          seq = (seq + 2) % VAL_MOD;
          creeaza_pachete_data_vid(&t, seq, 'N');
          send_message(&t);

          // Cazul cand crc corespunde.
        } else {

          // Se verifica daca pachetul primit are secventa corespunzatoare.
          expected_seq = (seq + 1) % VAL_MOD;
          seq = ((int)r->payload[2] + 1) % VAL_MOD;
          int dimensiune_date = r->len - 7;
          char tip = r->payload[3];

          if (expected_seq != (int)r->payload[2] ) {
            //Daca secventa primita nu este cea asteptata, se face continue.
            seq = (expected_seq + 1) % VAL_MOD;
            continue;
          }

          // Daca se primeste un pachet de tip "F" se deschide/creeaza un fisier
          // nou corespunzator.
          if (tip == 'F') {
            numeFisier = (char*)malloc((dimensiune_date  + 5 ) * sizeof(char));
            strcpy(numeFisier, "recv_");
            strncat(numeFisier, &r->payload[4], dimensiune_date);
            fisier = open(numeFisier, O_WRONLY| O_CREAT, 0777);
            if (fisier < 0) {
              printf("EROARE - Fisierul nu a putut fi deschis sau creat! \n");
              return -1;
            }

            // Daca se primeste un pachet de tip "D" se scriu datele in fisier.
          } else if (tip == 'D') {
            write(fisier, &r->payload[4], r->len - 7);

            // Daca se primeste un pachet de tip "Z" se inchide fisierul.
          } else if (tip == 'Z') {
            close(fisier);
            printf("=== Fisierul %s a fost creat cu succes! ===\n", numeFisier);
            free(numeFisier);

            // Daca se primeste un pachet de tip "B" se inchide fisierul
            // si se iese din bucla pentru ca ultimul fisier a fost transmis.
          } else if (tip == 'B') {
            close(fisier);
            break;
          }

          // Se trimite un pachet ACK.
          creeaza_pachete_data_vid(&t, seq, 'Y');
          send_message(&t);
        }
      }
    }

    // Se trimite un ACK pentru pachetul "B" primit de la ksender.
    // Pierderea pachetelor se face (conform enuntului) doar de la emitator la
    // receptor, deci nu mai trebuie verificat daca "ACK"-ul ajunge la ksender.
    creeaza_pachete_data_vid(&t, seq, 'Y');
    send_message(&t);

	return 0;
}
