#include <Windows.h>	//für COM-port unter windows
#include <iostream>		//für eingabe / ausgabe in der konsole
#include <string>		//für textverarbeitung
#include <thread>		//für kurze pausen
#include <chrono>		//für zeitmessung
#include <sstream>		//für die stringstream funktion (einlesen aus einem stream)


class SerielleVerbindung {		// klasse für die serielle verbindung mit dem mikrocontroller

public:		//diese Funktionen und Variablen sind von außen sichtbar und können auch benutzt werden;
	//konstruktor "serielleVerbindung" (muss gleich heißen wie die klasse): portname und baudrate werden übergeben;
	//durch "const std::wstring&" wird eine schreibgeschützte referenz übergeben - ohne "const" nicht schreibgeschützt, ohne "&" wird der ganze "text" übergeben -> mehr speicher notwendig und langsamer
	//"DWORD" ist double Word;
	//"baudrate" ist übertragungsgeschwindigkeit für COM-port - hier CBR_9600;
	SerielleVerbindung(const std::wstring& portName, DWORD baudrate = CBR_9600)
		: portName_(portName), baudrate_(baudrate), VerbindungsVerwaltung(INVALID_HANDLE_VALUE) {
	}	//Initialisierungsleiste - diese weist direkt die werte zu

//dekonstruktor "serielleVerbindung": schließt den COM-port sicher
	~SerielleVerbindung() {
		verbindungSchliessen();
	}

	//"bool": diese funktion gibt einen wahrheitswert zurück: true oder false
	//funktion zum öffnen der verbindung über den COM-port:
		//"CeateFileW": öffnet den COM-port unter windows;
		//"portName_.c_str()": name des COM.port
		//"GENERIC_READ | GENERIC_WRITE": zugriffsrechte: lesen und schreiben erlaubt
		//"0": keine gemeinsame nutzung
		//"NULL": keine sicherheitsatribute
		//"OPEN_EXISTING": öffne nur wenn COM-port bereits vorhanden
		//"0": keine speziellen flags
		//"NULL": kein vorlagenhandle
	bool verbindungOEffnen() {
		VerbindungsVerwaltung = CreateFileW(portName_.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);


		//prüfung ob das öffnen des COM-ports erfolgreich war (bzw. hier nicht erfolgreich war):
		if (VerbindungsVerwaltung == INVALID_HANDLE_VALUE) {			//vergleich, ob die variable verbindungsverwaltung den wert "INVALID_HANDLE_VALUE" (vordefinierter fehlerwert) hat;
			//"std::" -> Zugriff auf etwas aus der standardbibliothek, hier "cerr" (console error) -> Fehlerausgabe;
			std::cerr << "Fehler: COM-Port konnte nicht geoeffnet werden!\n";		//text innerhalb von "" wird bei erfüllung der if-bedingung ausgegeben;
			return false;		//bricht die funktion ab und gibt false zurück
		}

		//COM-port einstellungen setzen
		//DCB ist ein datentype der alle einstellungen für den seriellen COM-port enthält (Device Control Block):
		DCB portEinstellungen = { 0 };		//DCB struktur mit dem namen "portEinstellungen" und alle felder auf null setzt -> sicherer Anfangswert (nullinitialisierung);
		portEinstellungen.DCBlength = sizeof(portEinstellungen);		//größe der struktur;


		//"GetCommState"-funktion füllt DCB-struktur mit den aktuellen einstellungen;
		if (!GetCommState(VerbindungsVerwaltung, &portEinstellungen)) {		//wenn "GetCommState" nicht (!) funktioniert (fehlschlägt)...
			std::cerr << "Fehler: COM-Port Einstellungen konnten nicht gelesen werden!\n";		//...wird diese fehlermeldung ausgegeben;
			verbindungSchliessen();		//COM-port sauber schließen;
			return false;				//bricht funktion ab und gibt false zurück
		}

		//serielle schnittstellenparameter
		portEinstellungen.BaudRate = baudrate_;		//üertragungsrate
		portEinstellungen.ByteSize = 8;				//wie viele bits hat ein zeichen
		portEinstellungen.StopBits = ONESTOPBIT;	//wie viele stop-bits am ende eines zeichens
		portEinstellungen.Parity = NOPARITY;		//ist fehlerprüfung aktiviert (nein)

		//prüfung ob windows die seriellen einstellungen anwenden konnte...
		if (!SetCommState(VerbindungsVerwaltung, &portEinstellungen)) {						//... wenn nicht, dann...
			std::cerr << "Fehler: COM-Port Einstellungen konnten nicht gesetzt werden!\n";	//...wird diese fehlermeldung ausgegeben...
			verbindungSchliessen();		//... und COM-port sauber geschlossen
			return false;				//bricht funktion ab und gibt false zurück
		}

		//wenn alles bis hier her erfolgreich war, steht das programm hier - COM-port wurde erfolgreich geöffnet und konfiguriert;
		return true;		//beendet funktion und gibt true zurück;
	}

	//funktion zum schließen der verbindung, wenn diese nicht mehr benötigt wird;
	void verbindungSchliessen() {								//methode um COM-port zu schließen
		if (VerbindungsVerwaltung != INVALID_HANDLE_VALUE) {	//prüfung, ob COM-port überhaupt geöffnet ist;
			CloseHandle(VerbindungsVerwaltung);					//"CloseHandel" ist Windows-API-funktion um offenen COM-port zu schließen;
			VerbindungsVerwaltung = INVALID_HANDLE_VALUE;		//setzt "VerbindungsVerwaltung" auf ungültig;
		}
	}

	//funktion zum senden einer zeile (rechenausdruck)
	//funktion heißt "zeileSenden"
	bool	zeileSenden(const std::string& text) {			//string: zeichenkette; der "text" wird mittels referenz schreibgeschützt übergeben
		std::string textMitZeilenumbruch = text + "\n";		//ergänzt den text um einen zeilenumbruch, da arduino am Ende "\n² erwartet (Eingabe ist abgeschlossen);
		DWORD anzahlGesendeteZeichen;						//speichert wie viele bytes tatsächlich gesendet wurden;

		//"WriteFile" ist windows-API-funktion, die die Daten an den COM-port sendet;
		//"verbindungsVerwaltung": an welchen port
		//"textMitZeilenumbruch.c_str()": was senden (als c-string)
		//"textMitZeilenumbruch.size()": wie viele bytes
		//"&anzahlGesendeteZeichen": speicheradresse der variable "anzahlGesendeteZeichen" ("&" ist zeiger auf die variable)
		//"NULL": kein overlapped-mode: WriteFile wartet, bis operation abgeschlossen ist;
		return WriteFile(VerbindungsVerwaltung, textMitZeilenumbruch.c_str(), textMitZeilenumbruch.size(), &anzahlGesendeteZeichen, NULL);
	}

	//funktion zum empfangen einer antwortzeile vom mikrocontroller
	std::string zeileEmpfangen(int warteZeitInMillisekunden = 1000) {	//rückgabewert: std::string (Textzeile); wartezeit bis abgebrochen wird 1000ms;
		std::string empfangenerText;		//leere zeichenkette, in die die eingelesene zeile aufgebaut wird;
		char einzelnesZeichen;				//zwischenspeicher für eine einzelnes empfangenes zeichen;
		DWORD zeichenGelesen;				//Variable, in die windows die anzahl gelesener bytes schreibt;

		auto startZeit = std::chrono::steady_clock::now();		//speichert den aktuellen zeitpunkt;

		while (true) {		//endlosschleife, bis break kommt;

			//"ReadFile(VerbindungsVerwaltung" liest ein einzelnes zeichen vom COM-port 
			//"&einzelnesZeichen": Zielpuffer, wohin mit den Daten
			//"1": wie viele bytes sollen gelesen werden
			//"&zeichenGelesen": Zielpuffer, wie viele daten wurden gelesen
			//"NULL": kein overlapped-modus;
			//"&& zeichenGelesen == 1": nur wenn genau 1 byte empfangen wurde, soll der code im if-block ausgeführt werden;
			if (ReadFile(VerbindungsVerwaltung, &einzelnesZeichen, 1, &zeichenGelesen, NULL) && zeichenGelesen == 1) {

				if (einzelnesZeichen == '\n' || einzelnesZeichen == '\r') {		//wenn das empfangene zeichen ein zeilenumbruch (\n) oder wagenrücklauf (\r) ist ...
					break;			//...dann ist zeile beendet;
				}
				empfangenerText += einzelnesZeichen;		//wenn das zeichen kein zeilenende ist, wird es an die antwort angehängt;
			}

			auto aktuelleZeit = std::chrono::steady_clock::now();		//erneut den aktuellen Zeitpunkt abfragen;
			//berechnung der vergangenen zeit seit "startZeit":
			//"std::chrono::duration_cast<std::chrono::milliseconds>(aktuelleZeit - startZeit).count()" berechnet den zeitunterschied...
			///... und vergleicht ihn mit "warteZeitInMillisekunden"
			if (std::chrono::duration_cast<std::chrono::milliseconds>(aktuelleZeit - startZeit).count() > warteZeitInMillisekunden) {
				break;	//wenn vergangene zeit > als "wartezeit", dann wird abgebrochen; dies verhindert ewiges warten, wenn der controller nichts sendet;
			}


			//kurze pause damit die schleife nicht zu schnell rennt
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return empfangenerText;		//gibt vollständige empfangene zeile zurück (bei timout kann es auch ein leerer string sein);

	}

	//die funktion "empfangBisEnde" empfängt mehrere Zeilen bis -EOF- erkannt wird oder bis der timeout abgelaufen ist;
		//die zeile "-EOF-" vom mikrocontroller markiert das ende seiner nachricht;
	std::string empfangBisEnde(const std::string& endMarke = "-EOF-", int maxTimeout = 3000) {
		std::string gesamteAntwort;						//speichert die antwort zeile für zeile
		std::string aktuelleZeile;						//jeweils eine zeile, die gerade empfangen wurde
		auto start = std::chrono::steady_clock::now();	//zeitpunkt, wann gestartet wurde

		while (true) {				//endlosschleife, bis mit break ausgestiegen wird;
			aktuelleZeile = zeileEmpfangen(500);		//liest eine neue zeile von "zeileEmpfangen" und wartet dabei maximal 500ms auf eine zeile;

			if (!aktuelleZeile.empty()) {		//prüfung ob etwas empfangen wurde
				if (aktuelleZeile == endMarke) {		//wenn die empfangene zeile gleich der endmarke -EOF- ist, ist die antwort komplett
					break;		//dann wird die schleife beendet;
				}
				gesamteAntwort += aktuelleZeile + "\n";		//die empfangene zeile wird an "gesamteAntwort" angehängt + zeilenumbruch;
			}

			auto jetzt = std::chrono::steady_clock::now();		//aktuelle zeit messen, für die berechnung der zeitdauer seit dem "start";
			if (std::chrono::duration_cast<std::chrono::milliseconds>(jetzt - start).count() > maxTimeout) {		//überprüfung der wartezeit
				gesamteAntwort += "\nAntwort unvollstaendig (Timeout oder fehlendes -EOF-)\n";			//wenn wartezeit überschritten (3000ms), dann wird diese meldung abgegeben;
				break;				//schleife wird beendet, da wartezeit überschritten;
			}
		}

		return gesamteAntwort;			//gibt die gesamte empfangene Antwort als einen einzigen textblock zurück;
	}


private:		//dies ist nur innerhalb der klasse selbst zugänglich;
	std::wstring portName_;				//COM-port Name, L"\\\\.\\COM5"
	DWORD baudrate_;						//baudrate (9600)
	HANDLE VerbindungsVerwaltung;			//verbindungs - handle (wird vom system verwaltet)
};



//da der mikrocontroller mit "long" arbeitet, kann dieser nur in einem bestimmten zahlenberich richtig arbeiten
//werden zahlen ausserhalb dieses bereiches gesendet, werden falsche ergebnisse geliefert (überlauf)
//funktion "istLongImBereich" nimmt eine zeichnkette entgegen und prüft ob diese als "long" gültig wäre
//durch den vergleich als string (text) wird überprüft ob der text überhaupt in das format "long" passt bevor man die zeichenkette in eine zahl umwandelt
bool istLongImBereich(const std::string& text) {		//string: zeichenkette; der "text" wird mittels referenz schreibgeschützt übergeben
	const std::string LONG_MAX_STR = "2147483640";		//grösster positiver wert für "long"
	const std::string LONG_MIN_STR = "-2147483640";		//kleinster negativer wert für "long"

	std::string kontrolleText = text;		//erstellt eine kopie des strings "text" mit dem namen "kontrolleText" (dadurch wird das original nicht verändert)

	//entfernt alle leerzeichen aus dem string
	//"remove(kontrolleText.begin(), kontrolleText.end(), ' '": verschiebt alle zeichen die nicht ' ' sind nach vorne (remove löscht nichts, es verschiebt nur und "markiert" das ende
	//"kontrolleText.erase(...,  kontrolleText.end())": löscht alle zeichen ab der position, die "remove" zurückgegeben hat, bis zum ende
	kontrolleText.erase(remove(kontrolleText.begin(), kontrolleText.end(), ' '), kontrolleText.end());

	if (kontrolleText.empty())		//wenn der string leer ist...
		return false;				//...wird die funktion abgebrochen und false zurückgegeben

	//prüfung ob eine negative zahl als text noch innerhalb des gültigen "long" bereichs liegt
	if (kontrolleText[0] == '-') {								//wenn die eingabe mit einem minuszeichen beginnt wird mit dem negativen grenzwert geprüft
		if (kontrolleText.length() > LONG_MIN_STR.length())		//wenn die eingegebene "zahl" mehr stellen hat als "LONG_MIN_STR"...
			return false;										//...kann sie nicht gültig sein, die funktion wird abgebrochen und false zurückgegeben
		if (kontrolleText.length() == LONG_MIN_STR.length() && kontrolleText > LONG_MIN_STR)	//wenn die eingegebene "zahl" gleich viele stellen wie der grenzwert hat wird sie zeichenweise verglichen...
			return false;				//...und wenn diese kleiner (größer negativ) ist, wird die funktion abgebrochen und false zurückgegeben
	}

	//wenn kein minuszeichen vorne steht, wird geprüft, ob eine positive zahl als text noch innerhalb des gültigen "long" bereichs liegt
	else

	{
		if (kontrolleText.length() > LONG_MAX_STR.length())		//wenn die eingegebene "zahl" mehr stellen hat als "LONG_MAX_STR"...
			return false;										//...kann sie nicht gültig sein, die funktion wird abgebrochen und false zurückgegeben
		if (kontrolleText.length() == LONG_MAX_STR.length() && kontrolleText > LONG_MAX_STR)	//wenn die eingegebene "zahl" gleich viele stellen wie der grenzwert hat wird sie zeichenweise verglichen...
			return false;				//...und wenn diese größer ist, wird die funktion abgebrochen und false zurückgegeben
	}
	return true;	//wenn keine der vorherigen bedigungen false zurückgegeben hat, wird true zurückgegeben

};
//prüfung ob der eingegebene Text eine gültige ganze zahl darstellt
//funktion "istGanzeZahl" nimmt eine zeichnkette entgegen und prüft ob diese als ganze zahl gültig wäre
bool istGanzeZahl(const std::string& text) {

	if (text.empty())		//wenn der string leer ist...
		return false;		//...wird die funktion abgebrochen und false zurückgegeben

	size_t start = 0;		//die variable "start" gibt an bei welchem zeichen die prüfung beginnt
	if (text[0] == '-' || text[0] == '+') start = 1;		//wenn das erste zeichen ein minuszeichen oder ein pluszeichen ist, dann beginnt die prüfung erst ab dem nächsten stelle

	for (size_t zeichen = start; zeichen < text.length(); ++zeichen) {		//schleife läuft über alle zeichen im text; beginnt bei "start", solange "zeichen" kleiner als die länge des strings ist; erhöht bei jedem durchlauf ("++zeichen")
		if (!isdigit(text[zeichen]))		//"!isdigit" prüft ob "zeichen" eine ziffer ist; wenn nicht (!) dann...
			return false;					//...wird die funktion abgebrochen und false zurückgegeben
	}
	return true;
};

//hauptprogramm; "main()" wird als erste ausgeführt wenn das programm startet;
int main() {
	SerielleVerbindung verbindung(L"\\\\.\\COM5");		//erzeugung eines objektes (verbindung) der klasse "SerielleVerbindung" mit dem portnamen;

	if (!verbindung.verbindungOEffnen()) {			//aufruf der methode "verbindungOEffnen" - diese vesucht den COM-port zu öffnen; wenn nicht "!" (also das öffnen fehlschlägt)...
		return 1;		//... dann wird das programm mit dem fehlercode 1 beendet;
	}



	//danach startet die anwendung sichtbar für den benutzter; "cout" -> console output; "<<" -> einfügeoperator: schiebt den text nach cout; "\n" -> zeilenumbruch;
	std::cout << "PC-Taschenrechner gestartet.\n";
	std::cout << "Gib Rechenausdrueke mit ganzen Zahlen und in den vier Grundrechnungsarten ein.\n";
	std::cout << "Mit 'exit' beenden.\n";

	while (true) {				//endlosschleife bis mit break beendet wird;
		std::string eingabe;		//erstellung einer neuen zeichenkette "eingabe" für das was der benutzer eingibt;
		std::cout << "\n> ";		//zeigt in der konsole ">" für die eingabe;
		std::getline(std::cin, eingabe);		//die eingabe von der tastatur wird in der variable "eingabe" gespeichert;

		if (eingabe == "exit")		//wenn der benutzer "exit" eingibt...
			break;					//...dann wird die schleife beendet (break);

		if (eingabe.empty())		//wenn der benutzer nur enter drückt (leere eingabe)...
			continue;				//... dann wird der aktuelle schleifendurchlauf übersprungen;


		const std::string erlaubteOperatoren = "+-*/";		//erlaubte rechenoperatoren festlegen

		//"size_t": datentyp für größen und indizes
		//"eingabe.find_first_of(erlaubteOperatoren, 1)": "find_first_of" sucht im string "eingabe" nach dem ersten zeichen von "erlaubteOperatoren"; "1": ab dem index 1 wird gezählt (falls erste "zahl" negativ ist)
		//die posiion des operators wird in "opPos" gespeichert
		size_t	opPos = eingabe.find_first_of(erlaubteOperatoren, 1);
		if (opPos == std::string::npos) {											//wenn ein ausdruck ohne operator eingegeben wird...
			std::cerr << "Ungueltiger Ausdruck! Bitte gib z.B. ein: 123+45\n";		//...dann wird diese meldung ausgegeben...
			continue;															//...und der schleifendurchlauf übersprungen, und nächste eingabe sofort erlaubt;
		}

		//zerlegung des strings in die bestandteile "zahl operator zahl"
		std::string zahl1Text = eingabe.substr(0, opPos);		//der teil der eingabe ab index 0 (anfang) bis opPos (position des operators) -> wird in "zahl1Text" gespeichert
		std::string	operatorText = eingabe.substr(opPos, 1);	//nimmt genau ein Zeichen ab position "opPos" -> wird in "operatorText" gespeichert
		std::string zahl2Text = eingabe.substr(opPos + 1);		//der teil der eingabe ab "opPos"+1 bis zum ende -> wird in "zahl2Text" gespeichert

		//prüfung der eingabe auf ganze zahlen
		//prüfung der ersten zahl
		if(!istGanzeZahl(zahl1Text)) {											//wenn die erste zahl nicht im format einer ganzen zahl ist...
			std::cerr << "Erste Zahl ist keine ganze Zahl!\n";					//...dann wird diese meldung ausgegeben...
			continue;															//...und der schleifendurchlauf übersprungen, und nächste eingabe sofort erlaubt;
		}
		//prüfung der zweiten zahl
		if(!istGanzeZahl(zahl2Text)) {											//wenn die zweite zahl nicht im format einer ganzen zahl ist...
			std::cerr << "Zweite Zahl ist keine ganze Zahl!\n";					//...dann wird diese meldung ausgegeben...
			continue;															//...und der schleifendurchlauf übersprungen, und nächste eingabe sofort erlaubt;
		}

		//prüfung ob string "zahl1Text" innerhalb des gültigen zahlenbereichs "istLongImBereich" liegt...
		if (!istLongImBereich(zahl1Text)) {											//... wenn nicht (!)...
			std::cerr << "Erste Zahl ist ausserhalb des gueltigen Bereichs!\n";		//...dann wird diese meldung ausgegeben...
			continue;																//...und der schleifendurchlauf übersprungen, und nächste eingabe sofort erlaubt;
		}

		//prüfung ob string "zahl1Text" innerhalb des gültigen zahlenbereichs "istLongImBereich" liegt...
		if (!istLongImBereich(zahl2Text)) {											//... wenn nicht (!)...
			std::cerr << "Zweite Zahl ist ausserhalb des gueltigen Bereichs!\n";	//...dann wird diese meldung ausgegeben...
			continue;																//...und der schleifendurchlauf übersprungen, und nächste eingabe sofort erlaubt;
		}

		//alles gültig -> an mikrocontroller senden
		if (!verbindung.zeileSenden(eingabe)) {			//wenn beim senden der benutzereingabe an den mikrocontroller ein fehler auftritt ...
			std::cerr << "Fehler beim Senden!\n";		//...dann wird diese meldung ausgegeben ...
			continue;									//...und der schleifendurchlauf übersprungen, und nächste eingabe sofort erlaubt;
		}

		std::string antwort = verbindung.empfangBisEnde("-EOF-");		// wartet auf die antwort des mikrocontrollers; empfang läuft zeile für zeile bis "-EOF-" empfangen wird;
		std::cout << "Antwort vom Mikrocontroller:\n" << antwort << std::endl;		//gibt die empfangene antwort des mikrocontrollers in der konsole aus;
	}

	verbindung.verbindungSchliessen();			//aufruf der methode "verbindungSchliessen" aus der klasse "serielleVerbindung"; diese schließt den COM-port;
	std::cout << "\nProgramm beendet.\n";		//gibt diese meldung an den benutzer aus;
	return 0;									//beendet das "main()" programm mit dem wert 0;

};
