#include <MFRC522.h>
#include <Servo.h>
#include <stdlib.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2); 

#define SERVO_PIN1 6
#define SERVO_PIN2 5
#define BUZZER_PIN 8 
#define RST_PIN 9
#define SS_PIN 10
#define SENSORE  3 //SENSORE MAGNETICO 
#define buttonPin 7


int Aperto1 = 0; //Movimento in gradi del servo della prima serratura 
int Chiuso1 = 110; 
int Aperto2 = 300; //Movimento in gradi del servo della seconda serratura 
int Chiuso2 = 90;
int led_rosso = 2; 
int led_verde = 4; 
int stato = LOW; 
int sensorState = 0;  //variabile per lo switch magnetico 
bool fail = false; // gestire i tentativi rimanenti
int max_tentativi = 4; 
int tentativi = max_tentativi; 
String Chiave; 
String content= "";
int address = 0; // Indirizzo iniziale della EEPROM

MFRC522 RFID(SS_PIN, RST_PIN);
Servo Serratura1, Serratura2;

void setup() {
  lcd.init();         
  lcd.backlight();    
  
  Serial.begin(9600); //Inizializza la comunicazione Seriale
  SPI.begin(); //Inizializzia il bus per la connessione SPI
  RFID.PCD_Init();  //Inizializza Il modulo RFID
  
  
  Serratura1.attach(SERVO_PIN1); //servo1 al pin 6
  Serratura1.write(Chiuso1);
  Serial.println("chiusura serratura1");
  delay(2000); 
  Serratura1.detach(); // Spegni il servo 1
  
  Serratura2.attach(SERVO_PIN2); //servo2 al pin 5
  Serratura2.write(Chiuso2);
  Serial.println("chiusura serratura2");
  delay(2000); 
  Serratura2.detach(); // Spegni il servo 2
  
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);
  
  Serial.println("Il sistema è pronto");
  Serial.println();

  pinMode(led_rosso, OUTPUT); 
  pinMode(led_verde, OUTPUT); 
  pinMode(buttonPin, INPUT); 
  pinMode(SENSORE, INPUT_PULLUP);

  Serial.println("lettura eeprom...");
  // Verificare se il valore è vuoto o predefinito
  Chiave = readStringFromEEPROM(address);
  //Chiave ="??????";   
  Serial.println("eeprom letta");

  //controlliamo se la chiave letta in memoria rispetta il pattern XX__XX__XX__XX ovvero il formato della chive
  if (!checkPattern(Chiave)){
    Serial.println("Nessuna chiave trovata nella EEPROM all'indirizzo specificato.");
    lcd.clear();  
    lcd.setCursor(5, 0);
    lcd.print("CARTA NON");
    lcd.setCursor(3, 1);
    lcd.print("IN MEMORIA"); 
    delay(1000); 
    Chiave = setup_dispositivo(); //se la carta non è stata trovata in memoria entra in setup-mode
  } else {
    //se la chiave è già presente in memoria sarà usata quella per verificare lo sblocco della serratura
    Serial.print("chiave trovata nella EEPROM all'indirizzo specificato: ");
    Serial.println(Chiave);
    lcd.clear();  
    lcd.setCursor(5, 0);
    lcd.print("CARTA");
    lcd.setCursor(3, 1);
    lcd.print("IN MEMORIA");
  }
  delay(2000); 
}

void loop() {
  lcd.home();
  lcd.clear();
  
  if(fail == false)
  {
  
  digitalWrite(led_rosso, HIGH);
  lcd.setCursor(1, 0);
  lcd.print("BENVENUTO.!");
  // durante la sezione benvenuto è possibile entrare in setup mode è cambiare la carta da utilizzare
  for (int posizione = 0; posizione <= 15; posizione++) {     
        int buttonState = digitalRead(buttonPin);  
        // Esegui la funzione per entrare in setup-mode (tenere premuto il pulsante 3 secondi)
        enter_setup_mode(buttonState);
        delay(500);
        lcd.scrollDisplayRight();  //la scritta BENVENUTO! si muove verso destra un carattere alla volta ogni mezzo secondo 
      }

  lcd.clear();  
  lcd.setCursor(3, 0);
  lcd.print("INSERIRE LA");
  lcd.setCursor(5, 1);
  lcd.print("CARTA.!");

  delay(1000); 
  }
  else
  {
    if(tentativi > 0)
    {
     
    lcd.setCursor(0, 0);
    lcd.print("TENTATIVI");
    lcd.setCursor(0, 1);
    lcd.print("RIMANENTI:");    
    lcd.print(tentativi); //mostra i tentativi rimanenti 
    delay(1000); 
    }
    else
    {
      
      lcd.setCursor(3, 0);
      lcd.print("TENTATIVI");
      lcd.setCursor(3, 1);
      lcd.print("ESAURITI.!");  

      lcd.clear(); 
      lcd.setCursor(5, 0);
      lcd.print("ATTENDI");
      lcd.setCursor(1, 1);
      
      delay(1000);
      //se i tentativi sono esauriti attendi 15 secondi 
      for(int i = 15; i >= 0; i--) 
      {
         
        lcd.clear(); // Cancella il display LCD ad ogni iterazione
        lcd.setCursor(3, 0); // Imposta il cursore sulla prima riga
        lcd.print("mancano : ");
        lcd.setCursor(2, 1); 
        lcd.print(i); // Stampa il contatore dei secondi rimanenti
        lcd.setCursor(5, 1); 
        lcd.print("Secondi");
        delay(1000); // Attendi 1 secondo
        
      }   
      
      tentativi = max_tentativi - 1; //tentativi rimanenti 3 
      fail = false; 
      
    }
  }
  
  //Tiene sotto controllo il modulo RFID-RC522 e verifica se ci sono nuove chiavi
  if (!RFID.PICC_IsNewCardPresent()){
    return;
    }
    
  //Selezione la carta letta
  if (!RFID.PICC_ReadCardSerial()){
    return;
    }
  
  //Stampa sul monitor seriale il codice esadecimale della chiave
  Serial.print("Codice chiave: ");
  content= "";
  for (byte i = 0; i < RFID.uid.size; i++){
    
     Serial.print(RFID.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(RFID.uid.uidByte[i], HEX);
     content.concat(String(RFID.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(RFID.uid.uidByte[i], HEX));
    }
  Serial.println();
  Serial.print("Messaggio: ");
  content.toUpperCase();
  
  //Se la stringa letta dal modulo corrisponde a quella memorizzata esegue l'apertura della serratura, se no sottrae un tentativo

  if(content.substring(1) == Chiave)
  { 
    fail = false;    
    tentativi = max_tentativi;  // se la carta inserita è coretta resettiamo i tentativi 
     
    lcd.clear();   
    digitalWrite(led_rosso,LOW); // spegni il led rosso
    lampeggio(led_verde); // il led verde lampeggia un numero random di volte 
    digitalWrite(led_verde, HIGH); // il led verde si accende 

    // la serratura rimane aperta fin quando non si preme il bottone
    Accesso_Consentito();

    
    
    /* dopo la chiamata ad accesso consentito il servo torna in posizione chiusa quindi bisogna spegnere il led verde
     *  e accendere quello rosso 
     */
    digitalWrite(led_rosso, HIGH); 
    digitalWrite(led_verde,LOW);
    
  }
  else
  {  
    Serial.print("Carta inserita: ");
    Serial.println(content.substring(1));
    Serial.print("Carta corretta: ");
    Serial.println(Chiave);
    lcd.clear();   
    lampeggio(led_rosso); // il led rosso lampeggia random volte 
    digitalWrite(led_rosso, HIGH); // quando il servo è chiuso allora il led rosso è acceso
    digitalWrite(led_verde,LOW);
    Accesso_Negato(); 
    
    lcd.setCursor(0, 0);
    lcd.print("CARTA SBAGLIATA.");
    delay(2000); 
    
    fail = true; // è stata inserita la carta sbagliata 
    tentativi--; //scalare il numero di tentativi 
    
  }

  
 

  
}

//Funzione Accesso Consentito
void Accesso_Consentito(){
  Serial.println("Accesso consentito");
  
  //il buzzer emmette dei suoni per segnalare l'apertura della scatola 
  tone(BUZZER_PIN, 800);
  delay(100);
  noTone(BUZZER_PIN);
  delay(50);
  tone(BUZZER_PIN, 800);
  delay(150);
  noTone(BUZZER_PIN);
  
  Serratura1.attach(SERVO_PIN1); //servo1 al pin 6
  Serratura1.write(Aperto1);
  Serial.println("apertura serratura1");
  delay(1000); 
  Serratura1.detach(); // Spegni il servo 1
    
  Serratura2.attach(SERVO_PIN2); //servo2 al pin 5
  Serratura2.write(Aperto2);
  Serial.println("apertura serratura2");
  delay(1000); 
  Serratura2.detach(); // Spegni il servo 2
  
  //------ LCD

  lcd.setCursor(4, 0);
    lcd.print("SERRATURA");
    lcd.setCursor(4, 1);
    lcd.print("APERTA.!");
    delay(3000); 
    
    lcd.clear(); 
    lcd.setCursor(2, 0);
    lcd.print("BENVENUTO.!");
    lcd.setCursor(2, 1);
    lcd.print(Chiave);
  
  
  // attende finche il button non viene premuto, la scatola è chiusa, la carta è stata rimossa
  while (stato!=HIGH) {
   if(verifica_chiusura()==true)
   {
     stato = !stato;
     delay(200); 
     segnaleAcusticoLungo(); 
     lampeggio(led_rosso);
     
   }     
    
  }

  
  Serratura1.attach(SERVO_PIN1); //servo1 al pin 6
  Serratura1.write(Chiuso1);
  Serial.println("chiusura serratura1");
  delay(1000); 
  Serratura1.detach(); // Spegni il servo 1
  
  Serratura2.attach(SERVO_PIN2); //servo2 al pin 5
  Serratura2.write(Chiuso2);
  Serial.println("chiusura serratura2");
  delay(1000); 
  Serratura2.detach(); // Spegni il servo 2
  
  
  stato = LOW; 
  
 }


bool verifica_chiusura() {
  // Verifica se il pulsante è stato premuto
  if (digitalRead(buttonPin) == HIGH) {
    Serial.println("Il pulsante è stato premuto");
    
    // Verifica se la carta RFID è stata letta
    if (letturaRFID()) {
      Serial.println("La carta RFID non è presente");
      // Verifica se lo switch magnetico è chiuso
      if (switch_megnetico()) {
        Serial.println("Lo switch magnetico è chiuso");
        return true; // Se tutte le condizioni sono soddisfatte, restituisci true
      } else {
        Serial.println("Lo switch magnetico è aperto");
        chiudi_scatola();
        return false; // Se lo switch non è chiuso, restituisci false
      }
    } else {
      //se la carta è ancora presente in memoria, allora verifichiamo l'apertura della scatola
      Serial.println("La carta RFID è stata letta");
      rimuovi_carta();
       if (switch_megnetico()) {
        Serial.println("Lo switch magnetico è chiuso");        
      } else {
        Serial.println("Lo switch magnetico è aperto");
        chiudi_scatola();        
      }
      
      return false; // in ogni caso se la carta non è stata letta, restituisci false
    }   
    
  }
  
  return false; // Se il pulsante non è stato premuto, restituisci false
}


void chiudi_scatola()
{
  //mostrare sul display "chiudere la scatola!"
  lcd.clear(); 
  lcd.setCursor(1, 0);
  lcd.print("CHIUDERE LA");
  lcd.setCursor(1, 1);
  lcd.print("SCATOLA.!");
  delay(3000); 
  segnaleAcusticoBreve(); 
  lampeggio(led_verde);
  digitalWrite(led_verde, HIGH); 
}

void rimuovi_carta()
{
  //mostrare sul display "rimuovere la carta! "
  lcd.clear(); 
  lcd.setCursor(1, 0);
  lcd.print("RIMUOVERE LA");
  lcd.setCursor(1, 1);
  lcd.print("CARTA.!");
  delay(3000);
  segnaleAcusticoBreve(); 
  lampeggio(led_verde); 
  digitalWrite(led_verde, HIGH); 
}

bool switch_megnetico()
{
  // se la scatola è chiusa return true e si può chiudere la serratura 
  // se la scatola è aperta return false non si può chiudere la serratura 
    sensorState = digitalRead(SENSORE);
    if (sensorState == HIGH) {
     return true; 
    } else {
     return false; 
   }  
}

// funzione lampeggio led
void lampeggio(int led) {
  int numIterazioni = random(2, 6); // Genera un numero casuale tra 2 e 5 inclusi
  for (int i = 0; i < numIterazioni; i++) {
    digitalWrite(led, HIGH);   // Accendi il LED
    delay(100);                // Attendi
    digitalWrite(led, LOW);    // Spegni il LED
    delay(100);                // Attendi
  }
}



bool letturaRFID() {
   // Verifica se una nuova carta RFID è stata rilevata
  if (!RFID.PICC_IsNewCardPresent()) //la negazione del controllo IsNewCardPresent permette di rilevare la carta utilizzata per sbloccare la scatola 
  {
    
    Serial.println("Controllo RFID...");
    RFID.PICC_IsNewCardPresent(); 
     if (RFID.PICC_ReadCardSerial()) {
      Serial.println("CARTA PRESENTE!");
      // Stampa l'ID della carta RFID sul monitor seriale
      Serial.print("ID della carta RFID: ");
      for (byte i = 0; i < RFID.uid.size; i++) {
        Serial.print(RFID.uid.uidByte[i] < 0x10 ? "0" : "");
        Serial.print(RFID.uid.uidByte[i], HEX);
      }
      Serial.println();
      //la carta non è stata rimossa; 
      return false; 
     }else
     {
      //la carta è stata rimossa
      Serial.println("CARTA ASSENTE!"); 
      return true; 
     }
    
  }
  
  delay(100); 
  
}



void segnaleAcusticoBreve() {
  tone(BUZZER_PIN, 1000); // Attiva il buzzer con una frequenza di 1000 Hz
  delay(100); // Tieni il buzzer attivo per 100 millisecondi
  tone(BUZZER_PIN, 500); // Attiva il buzzer con una frequenza di 1000 Hz
  delay(100); // Tieni il buzzer attivo per 100 millisecondi
  tone(BUZZER_PIN, 1000); // Attiva il buzzer con una frequenza di 1000 Hz
  delay(100); // Tieni il buzzer attivo per 100 millisecondi
  noTone(BUZZER_PIN); // Disattiva il buzzer
}

void segnaleAcusticoLungo() {
  tone(BUZZER_PIN, 1000); // Attiva il buzzer con una frequenza di 1000 Hz
  delay(500); // Tieni il buzzer attivo per 500 millisecondi
  tone(BUZZER_PIN, 1000); // Attiva il buzzer con una frequenza di 1000 Hz
  delay(500); // Tieni il buzzer attivo per 500 millisecondi
  noTone(BUZZER_PIN); // Disattiva il buzzer
}


//Funzione Accesso Negato
void Accesso_Negato(){
  Serial.println("Accesso negato");
  tone(BUZZER_PIN, 500);
  delay(300);
  noTone(BUZZER_PIN);
  }


/*
 * funzione leggiValoreCarta, chiamata in setup mode, permette di rilevare una nuova carta inserita, ma permette anche la terminazione della setup mode in due casi: 
 * 1- il button viene premuto per 3 secondi 
 * 2- passano 10 secondi senza leggere alcuna carta
 * la terminazione sarà possibile solo se la carta è già presente in memoria, in caso contrario verrà visualizzato sullo schermo LCD "INSERIRE UNA CARTA!"
 * è il loop continuerà fin quando non sarà inserita una carta
 */
String leggiValoreCarta() {
  
  delay(1000); 
  bool buttonState = false;
  bool buttonPressed = false; // Flag per tenere traccia se il pulsante è stato premuto
  bool go_to_return = false; //Flag per la terminazione del ciclo 
  unsigned long buttonStartTime;
  unsigned long loopStartTime = millis();   // Tempo di inizio del ciclo
  
  while (!RFID.PICC_IsNewCardPresent()) {
    blink_setup();
    buttonState = digitalRead(buttonPin);
    
    // Se il pulsante è premuto, inizia il conteggio del tempo per il controllo del pulsante
    if (buttonState == HIGH && !buttonPressed) {
      buttonStartTime = millis();
      Serial.println("Button pressed");
      buttonPressed = true; // Imposta il flag a true quando il pulsante viene premuto per la prima volta
    }

    // Se il pulsante è rilasciato, resetta il flag
    if (buttonState == LOW) {
      buttonPressed = false;
    }

    // Verifica se il pulsante è stato premuto per 3 secondi
    if (buttonPressed && millis() - buttonStartTime >= 3000) {
      Serial.println("Il pulsante è stato premuto per 3 secondi, try to end setup");
      go_to_return = true; 
    }

    // Verifica se sono passati 10 secondi dall'inizio del ciclo
    if (millis() - loopStartTime >= 10000) {
      Serial.println("Sono passati 10 secondi, try to end setup");
      go_to_return = true; 
    }

    /*
     * se una delle condizioni per l'uscita si è verificata allora verifichiamo la presenza della chiave in memoria
     * in caso questa fosse già presente, il ciclo termina è la Chiave utilizzata sarà quella precedente all'entrate in setup-mode
     * in caso non fosse presente in memoria, lo si segnala all'utente e il ciclo riprende
     */
     
    if(go_to_return)
    {
      if (checkPattern(Chiave))
      {
        //la chiave è già presente in memoria 
        Serial.println("carta già presente in memoria, setup end");
        return "null";
      }
      else
      {
        //la carta è assente in memoria 
        Serial.println("carta non presente in memoria, setup continue");
        lcd.clear(); 
        lcd.setCursor(1, 0);
        lcd.print("INSERIRE");
        lcd.setCursor(1, 1);
        lcd.print("UNA CARTA !");
        
        for(int i=0; i<3; i++)
        {
          blink_setup(); //attendiamo 1,5 secondi con il blink setup 
        }      
        
        go_to_return = false; 
        lcd.clear(); 
        lcd.setCursor(1, 0);
        lcd.print("SETUP");
        lcd.setCursor(1, 1);
        lcd.print("IN CORSO...");    
        blink_setup(); 
        
        loopStartTime = millis();//riniziamo il conteggio dei 10 secondi    
      }
    }
    
  }


  /*
   * se la carta è stata rilevata e il ciclo non è stato interroto restituiamo 
   * la chiave letta
   */
  
  // Leggi l'ID della carta RFID
  if (RFID.PICC_ReadCardSerial()) {
    // Stampa sul monitor seriale il codice esadecimale della chiave
    Serial.print("Codice chiave: ");
    String content = "";
    for (byte i = 0; i < RFID.uid.size; i++) {
      Serial.print(RFID.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(RFID.uid.uidByte[i], HEX);
      content.concat(String(RFID.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(RFID.uid.uidByte[i], HEX));
    }
    Serial.println();
    content.toUpperCase();
    return content.substring(1); // Restituisci il valore della chiave 
  } else {
    // Errore durante la lettura della carta
    Serial.println("Errore durante la lettura della carta RFID");
    return "null"; // Restituisci una stringa vuota in caso di errore
  }
}

/*
 * per confermare la scelta della carta è neccessario premere due volte il pulsante, in caso 
 * in cui il button non viene premuto, o viene premuto una sola volta allora torna in setup-mode
 * e la carta letta non sarà utilizzata 
 */
int leggiButton() {
  unsigned long tempoInizio = millis(); // tempo di inizio
  unsigned long tempoLimite = 5000; // Tempo limite di 5 secondi
  int pressioni = 0;
  int statoPrecedente = LOW;
  boolean premuto = false; 
  lcd.clear(); 
  while (millis() - tempoInizio < tempoLimite) {
    
    blink_setup(); 
    
    
    int statoAttuale = digitalRead(buttonPin);
    
    // Verifica se il pulsante è stato premuto
    if (statoAttuale == HIGH && statoPrecedente == LOW && !premuto) {
      
      // Incrementa il conteggio delle pressioni
      pressioni++;
      premuto = true;
      digitalWrite(led_rosso, HIGH);  
      digitalWrite(led_verde, HIGH); 
      delay(100); 
      digitalWrite(led_rosso, LOW); 
      digitalWrite(led_verde, LOW);    

      //se le pressioni rilevate sono due allora il ciclo termina è la carta viene confermata 
      if (pressioni==2)
      {
        return pressioni; 
      }
      // Aggiorna lo stato precedente del pulsante
      statoPrecedente = statoAttuale;
      
    }

    lcd.setCursor(1, 0);
    lcd.print("PRESSIONI: ");
    lcd.setCursor(11, 0);
    lcd.print(pressioni);
    
    // Verifica se il pulsante è stato rilasciato
    if (statoAttuale == LOW && statoPrecedente == HIGH) {
      // Aggiorna lo stato precedente del pulsante
      premuto = false;
      statoPrecedente = statoAttuale;
    }
  }
  
  //se le pressioni rilevate non sono due allora la carta letta sarà scartata 
  return pressioni;
}



String setup_dispositivo() {
  int result;  
  
  Serial.println("Setup start! ");
  while (true) {
   
    
    // Inizio setup
    lcd.clear(); 
    lcd.setCursor(1, 0);
    lcd.print("SETUP");
    lcd.setCursor(1, 1);
    lcd.print("IN CORSO...");    
    blink_setup();    

       
    // Inserire una carta 
    String valore_carta = leggiValoreCarta();
    //se il valore_carta è null allora il ciclo della lettura della carta è stato terminato, oppure c'è stato un'errore nella lettura 
    
    if(valore_carta.equals("null"))
    {
      break; 
    }
    
    Serial.println(valore_carta);
    lcd.clear(); 
    lcd.setCursor(2, 0);
    lcd.print("LETTURA...");    
    blink_setup(); 
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("ID LETTO: ");
    lcd.setCursor(2, 1);
    lcd.print(valore_carta);
    
    for (int i = 0; i <= 3; i++) {
      blink_setup(); 
    }
    
    // Chiedi all'utente se vuole utilizzare questa carta
    while (true) {
      blink_setup(); 
      lcd.clear(); 
      lcd.setCursor(1, 0);
      lcd.print("UTILIZZARE");
      lcd.setCursor(1, 1);
      lcd.print("QUESTA CARTA.?");
      for (int i = 0; i <= 3; i++) {
        blink_setup(); 
      }
      result = leggiButton(); // Se premi una volta no, due volte si 
      if (result == 2) {
        // La chiave da utilizzare sarà questa carta 
        return valore_carta; 
      } else {
        break;
      }
    }
  }
  lcd.clear(); 
  lcd.setCursor(1, 0);
  lcd.print("NESSUNA");
  lcd.setCursor(1, 1);
  lcd.print("CARTA LETTA!");
  delay(1000); 
  return Chiave; 
  
}


void blink_setup()
{
    digitalWrite(led_rosso, HIGH);   
    delay(250); 
    digitalWrite(led_rosso, LOW);     
    digitalWrite(led_verde, HIGH);    
    delay(250);    
    digitalWrite(led_verde, LOW); 
}

void enter_setup_mode(int buttonState) {
  // Variabile statica per memorizzare lo stato precedente del pulsante
  static int lastButtonState = LOW;
  // Variabile per memorizzare il tempo in cui è iniziato il conteggio
  static unsigned long startTime = 0;

  // Se il pulsante è stato premuto
  if (buttonState != lastButtonState) {
    if (buttonState == HIGH) {
      // Memorizza il tempo in cui è iniziato il conteggio
      startTime = millis();
    } else {
      // Se il pulsante è stato rilasciato, resetta il tempo di inizio
      startTime = 0;
    }
  }

  // Se il pulsante è stato premuto per almeno 3 secondi
  if (buttonState == HIGH && millis() - startTime >= 3000) {
    // Esegui le azioni della modalità di setup
    Chiave = setup_dispositivo(); 
    // alla conclusione della setup-mode scriviamo il nuovo valore della chiave nella eeprom 
    writeStringToEEPROM(address, Chiave); 
    
    delay(1000);
    lcd.clear(); 
    lcd.setCursor(1, 0);
    segnaleAcusticoBreve(); 
    lcd.print("END SETUP!");    
    digitalWrite(led_rosso, HIGH);
    digitalWrite(led_verde, HIGH);  
    for (int posizione = 0; posizione <= 15; posizione++) {
        delay(250);
        lcd.scrollDisplayRight();  
    }  
    digitalWrite(led_rosso, LOW);
    digitalWrite(led_verde, LOW);
    lcd.clear(); 
  }

  // Aggiorna lo stato del pulsante
  lastButtonState = buttonState;
}

/*
 * leggimo nella memoria eeprom di arduino all'indirizzo dato fin quando non troviamo il carattere terminatore 
 * che indica la fine della chiave 
 */
 
String readStringFromEEPROM(int addr) {
  String data = "";
  char ch = EEPROM.read(addr);
  int i = 0;
  while (ch != '\0') {
    data += ch; //concata alla stringa data il carattere letto  
    i++;
    ch = EEPROM.read(addr + i);
  }
  return data;
}

/*
 * scriviamo in memoria, all'indirizzo speficato la chiave e concludiamo con il carattere terminatore 
 */
void writeStringToEEPROM(int addr, String data) {
  int dataSize = data.length();
  for (int i = 0; i < dataSize; i++) {
    EEPROM.write(addr + i, data[i]);
  }
  EEPROM.write(addr + dataSize, '\0'); // Aggiungere il terminatore di stringa
}


bool checkPattern(String str) {
  // Controlla la lunghezza della stringa
  if (str.length() != 11) { // 11 è la lunghezza del pattern "A1 B2 C3 D4"
    return false;
  }
  
  // Controlla se i caratteri vuoti sono nelle posizioni specifiche
  if (str.charAt(2) != ' ' || str.charAt(5) != ' ' || str.charAt(8) != ' ') {
    return false;
  }
  
  // Controlla che i caratteri nelle posizioni specifiche siano alfanumerici
  for (int i = 0; i < str.length(); i++) {
    if (i == 2 || i == 5 || i == 8) {
      continue; // Salta gli spazi
    }
    if (!isAlphaNumeric(str.charAt(i))) {
      return false;
    }
  }
  
  return true;
}

bool isAlphaNumeric(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}
