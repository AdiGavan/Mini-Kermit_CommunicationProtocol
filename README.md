"Copyright [2018] Gavan Adrian-George, 324CA"

Nume, prenume: Gavan, Adrian-George

Grupa, seria: 324CA

Tema 1 PC - MINI-KERMIT

Compilare si rulare:
====================

- Fisierele Makefile si run_experiment.sh sunt exact cele din scheletul de cod.
- Singurele fisiere modificate sunt kreceiver.c, ksender.c, lib.h (define-uri, noi structuri si
functii fara a modifica structura "msg" din lib.h) + fisierul README.
- Arhiva trimisa este incarcata fara executabile, asa ca mai intai trebuie sa se intre in folderul
link_emulator, se apeleaza make, dupa se revine in folderul cu restul fisierelor si se apeleaza
si aici make. Apoi se ruleaza run_experiment.sh cu comanda "./run_experiment.sh".
- Au fost date permisiunile 777 fisierului run_experiment.sh.

Prezentarea implementarii:
==========================

A. lib.h
========

1. S-au utilizat mai multe define-uri pentru anumite dimensiuni.
2. Structura "PacketKermit" este o structura ce contine campurile unui pachet MINI-KERMIT.
3. Structura "DataSendInit" contine campurile folosite pentru campul DATA din pachetul de tip "S".
4. Functia "initializareDataSendInit" initializeaza campurile unui pachet de tipul DataSendInit cu
valorile corspunzatoare fiecarui camp.
5. Functia "void creeaza_pachete_data_vid(msg* t, int seq, char tip)":
- Creeaza un pachet in care campul DATA este vid. Aceasta functie creeaza pachetele de tipul "Z",
"B", "Y" si "N".
- Mai intai se completeaza primele 4 campuri si campul MARK din pachetul de tip "PacketKermit",
se pun in payload-ul structurii "msg" si apoi se calculeaza crc-ul.
- Se pune crc-ul in payload, se pune si MARK, iar intr-un final se seteaza dimensiunea pachetului.
6. Functia "void creeaza_pachete_SendInit(msg* t, int seq, char tip)":
- Creeaza un pachet de tipul "S". Se urmeaza aceeasi pasi de mai sus, doar ca in aceasta functie
se initializeaza o structura de tipul "DataSendInit" si se copiaza toata aceastra structura in
campul "DATA" al pachetului "PacketKermit", urmand ca apoi sa fie copiata in payload.
7. Functia "void creeaza_pachete_Header(msg* t, int seq, char tip, char* argv)":
- Creeaza pachete de tipul "F".
- Diferenta este ca in campul "DATA" se pune numele fisierului. Functia primeste ca parametru
numele fisierului, se calculeaza lungimea acestuia cu "strlen(argv)" si apoi se copiaza numele
in campul "DATA". Lungimea numelui trebuie stiuta pentru a putea calcula crc si pentru a putea
pune usor datele in payload.
8. Functia "void creeaza_pachete_Date(msg* t, int seq, char tip, int octeti_cititi, char* buffer)":
- Creeaza pachete de tipul "D".
- Functia primeste numarul de octeti cititi din fisier si un buffer in care sunt pusi octetii
cititi. In campul "DATA" se copiaza buffer-ul, iar numarul de octeti citit se foloseste pentru
a stii cat sa se citeasca din buffer, pentru a calcula crc-ul si pentru a putea pune datele in
payload.

B. ksender.c
============

1. Functia "int trimite_si_asteapta_ACK(msg* t, int seq, int dimensiune_date, int timeout)":
- Functia trimite un packet si asteapta un raspuns de la kreceiver. Functia returneaza noua
secventa sau "-1" in cazul in care pachetul a fost retrimis de 3 ori.
- Se formeaza un loop infinit.
- Se trimite mesajul si se asteapta un raspuns.
- Daca nu se primeste un raspuns, atunci se reintra in bucla si se retrimite iar mesajul.
Se incearca retrimiterea pachetelor de inca 3 ori. Daca ksender nu primeste raspuns, se incheie
conexiunea.
- crc-ul nu trebuie verificat in ksender pentru ca pierderea sau coruperea pachetelor se face doar
de la emitator la receptor.
- Daca ksender primeste raspuns, se verifica daca este "ACK" sau "NACK".
- Daca este "ACK", doar se calculeaza noua secventa si se iese din bucla. Daca este "NACK", se
calculeaza noua secventa, se actualizeaza crc-ul si se retrimite pachetul.

2. Prezentarea algoritmului din ksender:
- Se creeaza un pachet de tipul "S" si se trimite catre kreceiver.
- Trimiterea si asteptarea raspunsului pentru acest pachet este identica cu metoda descrisa la
punctul B.1, diferenta fiind faptul ca atunci cand primeste "ACK", trebuie sa se memoreze MAXL si
TIME-ul primite de la kreceiver.
- Se aloca un buffer de dimensiunea "MAXL" primita de la kreceiver pentru a retine datele citite
din fisiere.
- Se creaza un for pentru a putea trimite fiecare fisier primit ca argument.
- Se creeaza un pachet de tip "F" si se trimite catre kreceiver cu functia descrisa la punctul B.1.
- Functia intoarce noua secventa in caz de succes, iar in caz de esec "seq" va deveni -1, moment in
care conexiunea se va inchide.
- Daca conexiunea nu s-a terminat, se deschide fisierul.
- Se creaza un loop in care se creeaza pachete de tipul "DATA" si se trimit catre kreceiver. In
loop se verifica daca conexiunea trebuie incheiata, iar conditia de iesire din loop este atunci
cand nu mai exista octeti de citit din fisiere.
- Daca datele s-au trimis corect, se trimite un pachet de tipul "Z" pentru a ii transmite
kreceiver-ului ca s-a terminat transmiterea datelor fisierului curent.
- Dupa terminarea for-ului (for-ul se termina doar daca toate fisierele au fost transmise corect)
se transmite un packet de tipul "B", pentru a informa kreceiver-ul ca toate fisierele au fost
transmise.
- Mereu se verifica daca conexiunea trebuie incheiata fortat si trebuie sa se aiba grija ca memoria
alocata sa fie eliberata si fisierele sa fie inchise.

C. kreceiver.c
==============

1. Functia "unsigned short luat_crc_ksender (msg* r)":
- Functia extrage crc-ul din pachetul trimis de catre ksender si returneaza crc-ul.

2. Prezentarea algoritmului din kreceiver:
- Mai intai se asteapta primirea pachetului "S".
- Se creeaza un loop infinit.
- Daca pachetul nu a ajuns, se asteapta de inca 2 ori primirea lui. Daca pachetul nu ajunge dupa
trei incercari se termina conexiunea.
- Daca pachetul ajunge, se calculeaza crc-ul pentru pachetul primit si se compara cu valoarea
crc-ului din pachetul primit. Daca crc-urile difera, atunci se actualizeaza secventa, se
creeaza un pachet de tip "NAK", se trimite pachetul catre ksender si se reintra in bucla pentru
ca kreceiver trebuie sa primeasca pachetul corect. Daca crc-urile corespund, se actualizeaza
secventa, se creeaza si se trimite un pachet de tipul "ACK" si se iese din loop.
- Daca pachetul "S" a fost primit corect, se creeaza un nou loop pentru a primii fisierele.
- In loop se verifica daca un pachet pierdut a fost retransmis de 3 ori. Daca a fost retransmis
de 3 ori, se dezaloca variabila cu numele fisierului curent, se inchide fisierul curent si se
incheie conexiunea.
- Daca pachetul a fost primit se calculeaza crc-ul pentru pachetul primit si se compara cu
crc-ul din pachetul de la ksender. Daca acestea difera, se actualizeaza secventa, se creeaza un
pachet de tip "NAK", se trimite catre ksender si se reintra in bucla pentru ca kreceiver va astepta
din nou pachetul.
- Daca crc-urile sunt la fel, se verifica daca secventa primita este cea corecta. Avem valoarea
ultimei secvente, deci adunand 1 la aceasta secventa vom obtine numarul de secventa pe care
kreceiver il asteapta. Daca secventele nu corespund, se face continue si se reintra in bucla (se
ignora pachetul primit).
- Daca secventele corespund, se verifica tipul pachetului primit.
- Daca pachetul este de tip "F", se creeaza fisierul corespunzator (recv_numefisier).
- Daca pachetul este de tip "D", se scriu datele in fisier.
- Daca pachetul este de tip "Z", se inchide fisierul, se afiseaza ca "Fisierul nume_fisier a fost
creat cu succes!" si se elibereaza memoria pentru variabila cu numele fisierului.
- Daca pachetul este de tip "B", se inchide fisierul curent si se iese din loop pentru ca acest
pachet spune ca ksender a trimis toate fisierele si conexiunea trebuie inchisa.
- Se creeaza si se trimite un pachet de tip "ACK" pentru pachetul "B" primit de la ksender.
Nu este nevoie sa tratam situatia in care ultimul "ACK" se poate pierde pentru ca pierderea si
coruperea octetilor se face doar in sensul emitator-receptor.

