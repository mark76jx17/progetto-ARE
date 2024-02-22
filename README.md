# progetto-ARE
video funzionamento: https://youtu.be/HczYyFtvmeQ
link drive alle immagini del progetto e allo schema del circuito: https://drive.google.com/drive/folders/1zdgQuA1f47QRJ__KuSI5YgSUtakY0Su_?usp=sharing

--------------------------------------
installare le librerie: LiquidCrystal I2C (by MArco Schwarts versione 1.1.2), MFRC522 (versione 1.4.10) 

Descrizione:
Questo progetto Arduino implementa un sistema di sblocco di una scatola (con due servomotori che servono da serratura) utilizzando carte RFID (Identificazione a Radio Frequenza) per il controllo degli accessi. Il sistema è composto da un microcontrollore Arduino uno, un modulo lettore RFID,modulo LCD con interfaccia I2C, due servo motori SG90 per i meccanismi di blocco/sblocco, LED per il feedback visivo, un buzzer per segnali sonori e un interruttore magnetico per monitorare lo stato di apertura della scatola (https://www.amazon.it/dp/B085XQLQ3N?psc=1&ref=ppx_yo2ov_dt_b_product_details).

Caratteristiche principali:

Accesso tramite Carte RFID: Gli utenti possono aprire la scatola presentando una carta RFID valida al lettore.
Controllo degli Accessi: Solo gli identificatori di carta RFID corrispondenti ottengono l'accesso, in caso si tentasse di aprire la scatola con una carta sbagliata il numero di tentativi disponibili scenderebbe 
ad ogni tentativo fino a 0, a 0 si dovranno attendere 15 secondi poi sarà possibile riniziare le letture della carta.
Modalità di Configurazione: Fornisce una modalità di configurazione per impostare le chiavi di accesso, consentendo una gestione delle carte autorizzati.
Feedback Visivo e Sonoro: Utilizza LED e un buzzer per fornire feedback visivo e sonoro durante le operazioni di sblocco e blocco della porta.

Sezione principale del ciclo:
Attendere che venga presentata una nuova carta RFID.
Leggere l'identificatore univoco della carta RFID.
Confrontare l'identificatore della carta con una chiave memorizzata.
Se la carta corrisponde alla chiave memorizzata, concedere l'accesso aprendo il lucchetto e attivando il LED verde. In caso contrario, negare l'accesso, attivare il LED rosso e decrementare il contatore dei tentativi rimanenti.
Se i tentativi rimanenti raggiungono zero, entrare in un periodo di countdown.
Durante il periodo di cooldown, il sistema attende per un certo periodo prima di reimpostare il contatore dei tentativi rimanenti e consentire nuovi tentativi di accesso.

Gestione degli Errori: 
Fornisce feedback attraverso LED, display LCD e suono per tentativi di accesso riusciti o falliti.

Utilizzo del Pulsante: 
Consente l'avvio e l'uscita dalla setup-mode, e la chiusura della scatola 

Utilizzo della EEPROM: 
Memorizza e recupera un valore chiave per il confronto durante i tentativi di accesso, legge dall'eprom mediante la funzione .

Sezione di Setup:
durante la sezione benvenuto è possibile entrare in setup mode è cambiare la carta da utilizzare. la prima funzione chiamata è la enter_setup_mode() che in modo non bloccante verifica se il button viene premuto per 3 secondi, se si allora effetua la chiamata ad una seconda funzione setup_dispositivo(), che verifica l'inserimento di una nuova carta chiamando la funzione leggiValoreCarta().
  funzione leggiValoreCarta, permette di rilevare una nuova carta inserita, ma permette anche la terminazione  della setup mode in due casi: 
  1- il button viene premuto per 3 secondi 
  2- passano 10 secondi senza leggere alcuna carta
  la terminazione sarà possibile solo se la carta è già presente in memoria, in caso contrario verrà visualizzato sullo schermo LCD "INSERIRE UNA CARTA!" è il loop continuerà fin quando non sarà inserita una carta. 
per confermare l'utilizzo di una carta bisogna premere il pulsante due volte, quindi si chiama leggiButton():
  per confermare la scelta della carta è neccessario premere due volte il pulsante, in caso 
  in cui il button non viene premuto, o viene premuto una sola volta allora torna in setup-mode e
  la carta letta non sarà utilizzata 
se tutte le condizioni sono verificate allora la nuova carta viene registrata chiamando la funzione writeStringToEEPROM();
 
Funzioni:
Accesso_Consentito(): Apre il lucchetto, segnala l'accesso consentito con suono e LED, attende che il pulsante venga premuto, quindi chiude il lucchetto.
Accesso_Negato(): Segnala l'accesso negato con un suono.
verifica_chiusura(): Controlla se la scatola è chiusa leggendo un interruttore magnetico.
lampeggio(): Fa lampeggiare i LED.
letturaRFID(): Legge i dati della carta RFID.
segnaleAcusticoBreve() e segnaleAcusticoLungo(): Producono segnali acustici brevi e lunghi, rispettivamente.
leggiValoreCarta(): Legge il valore della carta RFID.
leggiButton(): conferma con 2 pressioni il valore della carta letto
setup_dispositivo(): Entra in modalità di configurazione per configurare il sistema leggendo e memorizzando i dati della carta RFID.
blink_setup(): Fa lampeggiare i LED durante la modalità di configurazione.
enter_setup_mode(int): Gestisce l'ingresso nella modalità di configurazione.
readStringFromEEPROM(int): leggimo nella memoria eeprom di arduino all'indirizzo dato fin quando non troviamo il carattere terminatore che indica la fine della chiave 
writeStringToEEPROM(int, String): scriviamo in memoria, all'indirizzo speficato la chiave e concludiamo con il carattere terminatore 
checkPattern(String): controlla se la stringa passatta è conforme al formato della chiave 
isAlphaNumeric(char): chimata da checkPattern per verificare se il carattere passato è alfanumerico 


