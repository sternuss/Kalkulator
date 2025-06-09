void setup() {          //wird einmal beim start des mikrocontroller ausgeführt
  Serial.begin(9600);   //starte serielle kommunikation mit 9600 baud
  while(!Serial);       //warte bis die serielle verbindung bereit ist
}

void loop() {   //diese funktion läuft dauerhaft in einer schleife
  static String empfangenerText = "";   //speichert zeichen, bis zeile vollständig ist

  //zeichenweise empfangen
  while(Serial.available()) {   //solange zeichen über die serielle schnittstelle verfügbar sind...
    char einzelnesZeichen = Serial.read();    //...lies ein einzelnes Zeichen vom PC

    if(einzelnesZeichen == '\n' || einzelnesZeichen == '\r') { //wenn das Zeichen die "enter" - taste gedrückt wird (kann als \n ODER \r gesendet werden)...
      empfangenerText.trim();   //...entferne überflüssige leerzeichen vorne/hinten
      rechneAusdruck(empfangenerText);    //verarbeite den ganzen ausdruck
      empfangenerText = "";   //lösche den alten text für die nächste eingabe
      }
      else{ //wenn noch keine enter taste gedrückt wurde...
        empfangenerText += einzelnesZeichen;   //...hänge das gelesene zeichen an den bisherigen empfangenen text an
        }
      }
}

void rechneAusdruck(String eingabeText) {//funktion zur verarbeitung und berechnung des rechenausdrucks
  long zahl1, zahl2;      //die beiden zahlen im rechenausdruck
  char operatorSymbol;    //der rechenoperator

  //versuche den text wie folgt zu analysieren: zahl rechenzeichen zahl
  if(sscanf(eingabeText.c_str(),"%ld %c %ld", &zahl1, &operatorSymbol, &zahl2) == 3) { //versuche den text wie folgt zu analysieren: zahl rechenzeichen zahl
    long ergebnis = 0;    //variable für das berechnete ergebnis
    bool gueltigeEingabe = true;    //kontrollvariable zur fehlerbehandlung

    Serial.println("Es wurde eingegeben: ");    //dies wird an den PC ausgegeben
    Serial.print(eingabeText);                  //kontrollanzeige am PC, was vom benutzer eingegeben wurde
    Serial.println();

    //überlauf-grenzen
    const long MIN = -2147483640L;    //grenze negativer zahlenbereich
    const long MAX = 2147483640L;     //grenze positiver zahlenbereich


    switch(operatorSymbol) {  //rechne je nach erkanntem rechenzeichen - mit überlaufprüfung
      case '+':               //wenn das rechenzeichen + erkannt wird         
        if((zahl2 > 0 && zahl1 > MAX - zahl2) || (zahl2 < 0 && zahl1 < MIN -zahl2)) {   //berechnung ob ergebnis außerhalb des erlaubten zahlenbereich ist...
          Serial.println("Ergebnis ausserhalb des Zahlenbereichs (Addition)!");    //...wenn ja dann ausgabe dieses textes an PC
          Serial.println("-EOF-");    // endmarker des gesendeten textes (end of file)
          gueltigeEingabe = false;    //kontrollvariable ist falsch
          return;   //frühzeitiges zurückkehren, da zahlenbereich überschritten
          }
          
          ergebnis = zahl1 + zahl2;   // wenn voherige prüfungen negativ, dann berechne die zahlen so
          break;    //beendet die addition

       case '-':            // wenn das rechenzeichen - erkannt wird       
        if((zahl2 < 0 && zahl1 > MAX + zahl2) || (zahl2 > 0 && zahl1 < MIN + zahl2)) {    //berechnung ob ergebnis außerhalb des erlaubten zahlenbereich ist...
          Serial.println("Ergebnis ausserhalb des Zahlenbereichs (Subtraktion)!");   //...wenn ja dann ausgabe dieses textes an PC
          Serial.println("-EOF-");    // endmarker des gesendeten textes (end of file)
          gueltigeEingabe = false;    //kontrollvariable ist falsch
          return;   //frühzeitiges zurückkehren, da zahlenbereich überschritten
          }
          ergebnis = zahl1 - zahl2;   // wenn voherige prüfungen negativ, dann berechne die zahlen so
          break;    //beendet die subtraktion

       case '*':        // wenn das rechenzeichen * erkannt wird
        if(zahl1 > 0) {   //wenn erste zahl positiv
          if(zahl2 > MAX / zahl1 || zahl2 < MIN / zahl1)  {   //wird auf diese weise geprüft, ob das ergebnis außerhalb des erlaubten zahlebereichs ist
            Serial.println("Ergebnis ausserhalb des Zahlenbereichs (Multiplikation)!");    //...wenn ja dann ausgabe dieses textes an PC
            Serial.println("-EOF-");    // endmarker des gesendeten textes (end of file)
            gueltigeEingabe = false;    //kontrollvariable ist falsch
            return;   //frühzeitiges zurückkehren, da zahlenbereich überschritten
            }
          }
          else if(zahl1<0)  {   //wenn erste zahl negativ
            if(zahl2 != 0 && (zahl2 < MAX / zahl1 || zahl2 > MIN / zahl1))  {   //wird auf diese weise geprüft, ob das ergebnis außerhalb des erlaubten zahlebereichs ist
              Serial.println("Ergebnis ausserhalb des Zahlenbereichs (Multiplikation)!");    //...wenn ja dann ausgabe dieses textes an PC
              Serial.println("-EOF-");    // endmarker des gesendeten textes (end of file)
              gueltigeEingabe = false;    //kontrollvariable ist falsch
              return;   //frühzeitiges zurückkehren, da zahlenbereich überschritten
              }
            }
        
          ergebnis = zahl1 * zahl2;   // wenn voherige prüfungen negativ, dann berechne die zahlen so
          break;    //beendet die multiplikation

        case '/':       // wenn das rechenzeichen / erkannt wird
          if(zahl2 != 0) { //wenn die zweite zahl keine null ist, dann
            float ergebnisFloat = (float)zahl1 / (float)zahl2;   //ergebnis der division wird in eine fließkommazahl umgewandelt
            Serial.print("Ergebnis (auf drei Stellen gerundet): ");
            Serial.println(ergebnisFloat, 3); // das ergebnis der division an den PC zurücksenden
            Serial.println("-EOF-");  // endmarker des gesendeten textes (end of file)
            return;     //frühzeitiges zurückkehren, da die ausgabe der division bereits erfolgt ist
            
            }
            else {  //ansonsten (wenn die zweite zahl eine null ist)
              Serial.println("Fehler: Division durch Null ist nicht erlaubt!"); // sende diese Meldung an den PC
              Serial.println("-EOF-");  // endmarker des gesendeten textes (end of file)
              gueltigeEingabe = false; //kontrollvariable ist falsch
              }
          break;    // beendet die division
        default:    //wenn kein gültiges rechenzeichen eingegeben (erkannt) wurde, wird folgende meldung ausgegeben
          Serial.println("Fehler: Unbekannter Rechenoperator! Erlaubt sind nur ganze Zahlen und die Operationen +, -, * und /");
          Serial.println("-EOF-");    // endmarker des gesendeten textes (end of file)
          gueltigeEingabe = false;    //kontrollvariable ist falsch
        }

      if(gueltigeEingabe){       //wenn alle eingaben korrekt sind, sende das berechnete ergebnis zurück an den PC
        Serial.println("Ergebnis: ");   //sendet dies als standardtext vor...
        Serial.println(ergebnis);   //...diesem, der das endergebnis der berechnung darstellt
        Serial.println("-EOF-");  // endmarker des gesendeten textes (end of file)
        } 
      }
   else {    //wenn der ausdruck nicht im richtigen format ist
        Serial.println("Fehler: Ungueltige Eingabe. Bitte Format 'Zahl Operator Zahl' verwenden (z. B. 18*3)");
        Serial.println("Erlaubte Operatoren: +, -, * und /");
        Serial.println("Es wurde eingegeben: ");
        Serial.print(eingabeText);    // es wird der vom benutzer eingegebene ausdruck angezeigt, um fehleingaben zu erkennen
        Serial.print("\n");     //fügt eine neue zeile nach "eingabeText" ein
        Serial.println("-EOF-");  // endmarker des gesendeten textes (end of file)
        }
  
  }
