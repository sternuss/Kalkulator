#include <Windows.h>	//f�r COM-port unter windows
#include <iostream>		//f�r eingabe / ausgabe in der konsole
#include <string>		//f�r textverarbeitung
#include <thread>		//f�r kurze pausen
#include <chrono>		//f�r zeitmessung
#include <sstream>		//f�r die stringstream funktion (einlesen aus einem stream)


class SerielleVerbindung {		// klasse f�r die serielle verbindung mit dem mikrocontroller

public:		//diese Funktionen und Variablen sind von au�en sichtbar und k�nnen auch benutzt werden;
	//konstruktor "serielleVerbindung" (muss gleich hei�en wie die klasse): portname und baudrate werden �bergeben;
	//durch "const std::wstring&" wird eine schreibgesch�tzte referenz �bergeben - ohne "const" nicht schreibgesch�tzt, ohne "&" wird der ganze "text" �bergeben -> mehr speicher notwendig und langsamer
	//"DWORD" ist double Word;
	//"baudrate" ist �bertragungsgeschwindigkeit f�r COM-port - hier CBR_9600;
	SerielleVerbindung(const std::wstring& portName, DWORD baudrate = CBR_9600)
		: portName_(portName), baudrate_(baudrate), VerbindungsVerwaltung(INVALID_HANDLE_VALUE) {
	}	//Initialisierungsleiste - diese weist direkt die werte zu

//dekonstruktor "serielleVerbindung": schlie�t den COM-port sicher
	~SerielleVerbindung() {
		verbindungSchliessen();
	}

	//"bool": diese funktion gibt einen wahrheitswert zur�ck: true oder false
	//funktion zum �ffnen der verbindung �ber den COM-port:
		//"CeateFileW": �ffnet den COM-port unter windows;
		//"portName_.c_str()": name des COM.port
		//"GENERIC_READ | GENERIC_WRITE": zugriffsrechte: lesen und schreiben erlaubt
		//"0": keine gemeinsame nutzung
		//"NULL": keine sicherheitsatribute
		//"OPEN_EXISTING": �ffne nur wenn COM-port bereits vorhanden
		//"0": keine speziellen flags
		//"NULL": kein vorlagenhandle
	bool verbindungOEffnen() {
		VerbindungsVerwaltung = CreateFileW(portName_.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);


		//pr�fung ob das �ffnen des COM-ports erfolgreich war (bzw. hier nicht erfolgreich war):
		if (VerbindungsVerwaltung == INVALID_HANDLE_VALUE) {			//vergleich, ob die variable verbindungsverwaltung den wert "INVALID_HANDLE_VALUE" (vordefinierter fehlerwert) hat;
			//"std::" -> Zugriff auf etwas aus der standardbibliothek, hier "cerr" (console error) -> Fehlerausgabe;
			std::cerr << "Fehler: COM-Port konnte nicht geoeffnet werden!\n";		//text innerhalb von "" wird bei erf�llung der if-bedingung ausgegeben;
			return false;		//bricht die funktion ab und gibt false zur�ck
		}

		//COM-port einstellungen setzen
		//DCB ist ein datentype der alle einstellungen f�r den seriellen COM-port enth�lt (Device Control Block):
		DCB portEinstellungen = { 0 };		//DCB struktur mit dem namen "portEinstellungen" und alle felder auf null setzt -> sicherer Anfangswert (nullinitialisierung);
		portEinstellungen.DCBlength = sizeof(portEinstellungen);		//gr��e der struktur;


		//"GetCommState"-funktion f�llt DCB-struktur mit den aktuellen einstellungen;
		if (!GetCommState(VerbindungsVerwaltung, &portEinstellungen)) {		//wenn "GetCommState" nicht (!) funktioniert (fehlschl�gt)...
			std::cerr << "Fehler: COM-Port Einstellungen konnten nicht gelesen werden!\n";		//...wird diese fehlermeldung ausgegeben;
			verbindungSchliessen();		//COM-port sauber schlie�en;
			return false;				//bricht funktion ab und gibt false zur�ck
		}

		//serielle schnittstellenparameter
		portEinstellungen.BaudRate = baudrate_;		//�ertragungsrate
		portEinstellungen.ByteSize = 8;				//wie viele bits hat ein zeichen
		portEinstellungen.StopBits = ONESTOPBIT;	//wie viele stop-bits am ende eines zeichens
		portEinstellungen.Parity = NOPARITY;		//ist fehlerpr�fung aktiviert (nein)

		//pr�fung ob windows die seriellen einstellungen anwenden konnte...
		if (!SetCommState(VerbindungsVerwaltung, &portEinstellungen)) {						//... wenn nicht, dann...
			std::cerr << "Fehler: COM-Port Einstellungen konnten nicht gesetzt werden!\n";	//...wird diese fehlermeldung ausgegeben...
			verbindungSchliessen();		//... und COM-port sauber geschlossen
			return false;				//bricht funktion ab und gibt false zur�ck
		}

		//wenn alles bis hier her erfolgreich war, steht das programm hier - COM-port wurde erfolgreich ge�ffnet und konfiguriert;
		return true;		//beendet funktion und gibt true zur�ck;
	}

	//funktion zum schlie�en der verbindung, wenn diese nicht mehr ben�tigt wird;
	void verbindungSchliessen() {								//methode um COM-port zu schlie�en
		if (VerbindungsVerwaltung != INVALID_HANDLE_VALUE) {	//pr�fung, ob COM-port �berhaupt ge�ffnet ist;
			CloseHandle(VerbindungsVerwaltung);					//"CloseHandel" ist Windows-API-funktion um offenen COM-port zu schlie�en;
			VerbindungsVerwaltung = INVALID_HANDLE_VALUE;		//setzt "VerbindungsVerwaltung" auf ung�ltig;
		}
	}

	//funktion zum senden einer zeile (rechenausdruck)
	//funktion hei�t "zeileSenden"
	bool	zeileSenden(const std::string& text) {			//string: zeichenkette; der "text" wird mittels referenz schreibgesch�tzt �bergeben
		std::string textMitZeilenumbruch = text + "\n";		//erg�nzt den text um einen zeilenumbruch, da arduino am Ende "\n� erwartet (Eingabe ist abgeschlossen);
		DWORD anzahlGesendeteZeichen;						//speichert wie viele bytes tats�chlich gesendet wurden;

		//"WriteFile" ist windows-API-funktion, die die Daten an den COM-port sendet;
		//"verbindungsVerwaltung": an welchen port
		//"textMitZeilenumbruch.c_str()": was senden (als c-string)
		//"textMitZeilenumbruch.size()": wie viele bytes
		//"&anzahlGesendeteZeichen": speicheradresse der variable "anzahlGesendeteZeichen" ("&" ist zeiger auf die variable)
		//"NULL": kein overlapped-mode: WriteFile wartet, bis operation abgeschlossen ist;
		return WriteFile(VerbindungsVerwaltung, textMitZeilenumbruch.c_str(), textMitZeilenumbruch.size(), &anzahlGesendeteZeichen, NULL);
	}

	//funktion zum empfangen einer antwortzeile vom mikrocontroller
	std::string zeileEmpfangen(int warteZeitInMillisekunden = 1000) {	//r�ckgabewert: std::string (Textzeile); wartezeit bis abgebrochen wird 1000ms;
		std::string empfangenerText;		//leere zeichenkette, in die die eingelesene zeile aufgebaut wird;
		char einzelnesZeichen;				//zwischenspeicher f�r eine einzelnes empfangenes zeichen;
		DWORD zeichenGelesen;				//Variable, in die windows die anzahl gelesener bytes schreibt;

		auto startZeit = std::chrono::steady_clock::now();		//speichert den aktuellen zeitpunkt;

		while (true) {		//endlosschleife, bis break kommt;

			//"ReadFile(VerbindungsVerwaltung" liest ein einzelnes zeichen vom COM-port 
			//"&einzelnesZeichen": Zielpuffer, wohin mit den Daten
			//"1": wie viele bytes sollen gelesen werden
			//"&zeichenGelesen": Zielpuffer, wie viele daten wurden gelesen
			//"NULL": kein overlapped-modus;
			//"&& zeichenGelesen == 1": nur wenn genau 1 byte empfangen wurde, soll der code im if-block ausgef�hrt werden;
			if (ReadFile(VerbindungsVerwaltung, &einzelnesZeichen, 1, &zeichenGelesen, NULL) && zeichenGelesen == 1) {

				if (einzelnesZeichen == '\n' || einzelnesZeichen == '\r') {		//wenn das empfangene zeichen ein zeilenumbruch (\n) oder wagenr�cklauf (\r) ist ...
					break;			//...dann ist zeile beendet;
				}
				empfangenerText += einzelnesZeichen;		//wenn das zeichen kein zeilenende ist, wird es an die antwort angeh�ngt;
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
		return empfangenerText;		//gibt vollst�ndige empfangene zeile zur�ck (bei timout kann es auch ein leerer string sein);

	}

	//die funktion "empfangBisEnde" empf�ngt mehrere Zeilen bis -EOF- erkannt wird oder bis der timeout abgelaufen ist;
		//die zeile "-EOF-" vom mikrocontroller markiert das ende seiner nachricht;
	std::string empfangBisEnde(const std::string& endMarke = "-EOF-", int maxTimeout = 3000) {
		std::string gesamteAntwort;						//speichert die antwort zeile f�r zeile
		std::string aktuelleZeile;						//jeweils eine zeile, die gerade empfangen wurde
		auto start = std::chrono::steady_clock::now();	//zeitpunkt, wann gestartet wurde

		while (true) {				//endlosschleife, bis mit break ausgestiegen wird;
			aktuelleZeile = zeileEmpfangen(500);		//liest eine neue zeile von "zeileEmpfangen" und wartet dabei maximal 500ms auf eine zeile;

			if (!aktuelleZeile.empty()) {		//pr�fung ob etwas empfangen wurde
				if (aktuelleZeile == endMarke) {		//wenn die empfangene zeile gleich der endmarke -EOF- ist, ist die antwort komplett
					break;		//dann wird die schleife beendet;
				}
				gesamteAntwort += aktuelleZeile + "\n";		//die empfangene zeile wird an "gesamteAntwort" angeh�ngt + zeilenumbruch;
			}

			auto jetzt = std::chrono::steady_clock::now();		//aktuelle zeit messen, f�r die berechnung der zeitdauer seit dem "start";
			if (std::chrono::duration_cast<std::chrono::milliseconds>(jetzt - start).count() > maxTimeout) {		//�berpr�fung der wartezeit
				gesamteAntwort += "\nAntwort unvollstaendig (Timeout oder fehlendes -EOF-)\n";			//wenn wartezeit �berschritten (3000ms), dann wird diese meldung abgegeben;
				break;				//schleife wird beendet, da wartezeit �berschritten;
			}
		}

		return gesamteAntwort;			//gibt die gesamte empfangene Antwort als einen einzigen textblock zur�ck;
	}


private:		//dies ist nur innerhalb der klasse selbst zug�nglich;
	std::wstring portName_;				//COM-port Name, L"\\\\.\\COM5"
	DWORD baudrate_;						//baudrate (9600)
	HANDLE VerbindungsVerwaltung;			//verbindungs - handle (wird vom system verwaltet)
};



//da der mikrocontroller mit "long" arbeitet, kann dieser nur in einem bestimmten zahlenberich richtig arbeiten
//werden zahlen ausserhalb dieses bereiches gesendet, werden falsche ergebnisse geliefert (�berlauf)
//funktion "istLongImBereich" nimmt eine zeichnkette entgegen und pr�ft ob diese als "long" g�ltig w�re
//durch den vergleich als string (text) wird �berpr�ft ob der text �berhaupt in das format "long" passt bevor man die zeichenkette in eine zahl umwandelt
bool istLongImBereich(const std::string& text) {		//string: zeichenkette; der "text" wird mittels referenz schreibgesch�tzt �bergeben
	const std::string LONG_MAX_STR = "2147483640";		//gr�sster positiver wert f�r "long"
	const std::string LONG_MIN_STR = "-2147483640";		//kleinster negativer wert f�r "long"

	std::string kontrolleText = text;		//erstellt eine kopie des strings "text" mit dem namen "kontrolleText" (dadurch wird das original nicht ver�ndert)

	//entfernt alle leerzeichen aus dem string
	//"remove(kontrolleText.begin(), kontrolleText.end(), ' '": verschiebt alle zeichen die nicht ' ' sind nach vorne (remove l�scht nichts, es verschiebt nur und "markiert" das ende
	//"kontrolleText.erase(...,  kontrolleText.end())": l�scht alle zeichen ab der position, die "remove" zur�ckgegeben hat, bis zum ende
	kontrolleText.erase(remove(kontrolleText.begin(), kontrolleText.end(), ' '), kontrolleText.end());

	if (kontrolleText.empty())		//wenn der string leer ist...
		return false;				//...wird die funktion abgebrochen und false zur�ckgegeben

	//pr�fung ob eine negative zahl als text noch innerhalb des g�ltigen "long" bereichs liegt
	if (kontrolleText[0] == '-') {								//wenn die eingabe mit einem minuszeichen beginnt wird mit dem negativen grenzwert gepr�ft
		if (kontrolleText.length() > LONG_MIN_STR.length())		//wenn die eingegebene "zahl" mehr stellen hat als "LONG_MIN_STR"...
			return false;										//...kann sie nicht g�ltig sein, die funktion wird abgebrochen und false zur�ckgegeben
		if (kontrolleText.length() == LONG_MIN_STR.length() && kontrolleText > LONG_MIN_STR)	//wenn die eingegebene "zahl" gleich viele stellen wie der grenzwert hat wird sie zeichenweise verglichen...
			return false;				//...und wenn diese kleiner (gr��er negativ) ist, wird die funktion abgebrochen und false zur�ckgegeben
	}

	//wenn kein minuszeichen vorne steht, wird gepr�ft, ob eine positive zahl als text noch innerhalb des g�ltigen "long" bereichs liegt
	else

	{
		if (kontrolleText.length() > LONG_MAX_STR.length())		//wenn die eingegebene "zahl" mehr stellen hat als "LONG_MAX_STR"...
			return false;										//...kann sie nicht g�ltig sein, die funktion wird abgebrochen und false zur�ckgegeben
		if (kontrolleText.length() == LONG_MAX_STR.length() && kontrolleText > LONG_MAX_STR)	//wenn die eingegebene "zahl" gleich viele stellen wie der grenzwert hat wird sie zeichenweise verglichen...
			return false;				//...und wenn diese gr��er ist, wird die funktion abgebrochen und false zur�ckgegeben
	}
	return true;	//wenn keine der vorherigen bedigungen false zur�ckgegeben hat, wird true zur�ckgegeben

};
//pr�fung ob der eingegebene Text eine g�ltige ganze zahl darstellt
//funktion "istGanzeZahl" nimmt eine zeichnkette entgegen und pr�ft ob diese als ganze zahl g�ltig w�re
bool istGanzeZahl(const std::string& text) {

	if (text.empty())		//wenn der string leer ist...
		return false;		//...wird die funktion abgebrochen und false zur�ckgegeben

	size_t start = 0;		//die variable "start" gibt an bei welchem zeichen die pr�fung beginnt
	if (text[0] == '-' || text[0] == '+') start = 1;		//wenn das erste zeichen ein minuszeichen oder ein pluszeichen ist, dann beginnt die pr�fung erst ab dem n�chsten stelle

	for (size_t zeichen = start; zeichen < text.length(); ++zeichen) {		//schleife l�uft �ber alle zeichen im text; beginnt bei "start", solange "zeichen" kleiner als die l�nge des strings ist; erh�ht bei jedem durchlauf ("++zeichen")
		if (!isdigit(text[zeichen]))		//"!isdigit" pr�ft ob "zeichen" eine ziffer ist; wenn nicht (!) dann...
			return false;					//...wird die funktion abgebrochen und false zur�ckgegeben
	}
	return true;
};

//hauptprogramm; "main()" wird als erste ausgef�hrt wenn das programm startet;
int main() {
	SerielleVerbindung verbindung(L"\\\\.\\COM5");		//erzeugung eines objektes (verbindung) der klasse "SerielleVerbindung" mit dem portnamen;

	if (!verbindung.verbindungOEffnen()) {			//aufruf der methode "verbindungOEffnen" - diese vesucht den COM-port zu �ffnen; wenn nicht "!" (also das �ffnen fehlschl�gt)...
		return 1;		//... dann wird das programm mit dem fehlercode 1 beendet;
	}



	//danach startet die anwendung sichtbar f�r den benutzter; "cout" -> console output; "<<" -> einf�geoperator: schiebt den text nach cout; "\n" -> zeilenumbruch;
	std::cout << "PC-Taschenrechner gestartet.\n";
	std::cout << "Gib Rechenausdrueke mit ganzen Zahlen und in den vier Grundrechnungsarten ein.\n";
	std::cout << "Mit 'exit' beenden.\n";

	while (true) {				//endlosschleife bis mit break beendet wird;
		std::string eingabe;		//erstellung einer neuen zeichenkette "eingabe" f�r das was der benutzer eingibt;
		std::cout << "\n> ";		//zeigt in der konsole ">" f�r die eingabe;
		std::getline(std::cin, eingabe);		//die eingabe von der tastatur wird in der variable "eingabe" gespeichert;

		if (eingabe == "exit")		//wenn der benutzer "exit" eingibt...
			break;					//...dann wird die schleife beendet (break);

		if (eingabe.empty())		//wenn der benutzer nur enter dr�ckt (leere eingabe)...
			continue;				//... dann wird der aktuelle schleifendurchlauf �bersprungen;


		const std::string erlaubteOperatoren = "+-*/";		//erlaubte rechenoperatoren festlegen

		//"size_t": datentyp f�r gr��en und indizes
		//"eingabe.find_first_of(erlaubteOperatoren, 1)": "find_first_of" sucht im string "eingabe" nach dem ersten zeichen von "erlaubteOperatoren"; "1": ab dem index 1 wird gez�hlt (falls erste "zahl" negativ ist)
		//die posiion des operators wird in "opPos" gespeichert
		size_t	opPos = eingabe.find_first_of(erlaubteOperatoren, 1);
		if (opPos == std::string::npos) {											//wenn ein ausdruck ohne operator eingegeben wird...
			std::cerr << "Ungueltiger Ausdruck! Bitte gib z.B. ein: 123+45\n";		//...dann wird diese meldung ausgegeben...
			continue;															//...und der schleifendurchlauf �bersprungen, und n�chste eingabe sofort erlaubt;
		}

		//zerlegung des strings in die bestandteile "zahl operator zahl"
		std::string zahl1Text = eingabe.substr(0, opPos);		//der teil der eingabe ab index 0 (anfang) bis opPos (position des operators) -> wird in "zahl1Text" gespeichert
		std::string	operatorText = eingabe.substr(opPos, 1);	//nimmt genau ein Zeichen ab position "opPos" -> wird in "operatorText" gespeichert
		std::string zahl2Text = eingabe.substr(opPos + 1);		//der teil der eingabe ab "opPos"+1 bis zum ende -> wird in "zahl2Text" gespeichert

		//pr�fung der eingabe auf ganze zahlen
		//pr�fung der ersten zahl
		if(!istGanzeZahl(zahl1Text)) {											//wenn die erste zahl nicht im format einer ganzen zahl ist...
			std::cerr << "Erste Zahl ist keine ganze Zahl!\n";					//...dann wird diese meldung ausgegeben...
			continue;															//...und der schleifendurchlauf �bersprungen, und n�chste eingabe sofort erlaubt;
		}
		//pr�fung der zweiten zahl
		if(!istGanzeZahl(zahl2Text)) {											//wenn die zweite zahl nicht im format einer ganzen zahl ist...
			std::cerr << "Zweite Zahl ist keine ganze Zahl!\n";					//...dann wird diese meldung ausgegeben...
			continue;															//...und der schleifendurchlauf �bersprungen, und n�chste eingabe sofort erlaubt;
		}

		//pr�fung ob string "zahl1Text" innerhalb des g�ltigen zahlenbereichs "istLongImBereich" liegt...
		if (!istLongImBereich(zahl1Text)) {											//... wenn nicht (!)...
			std::cerr << "Erste Zahl ist ausserhalb des gueltigen Bereichs!\n";		//...dann wird diese meldung ausgegeben...
			continue;																//...und der schleifendurchlauf �bersprungen, und n�chste eingabe sofort erlaubt;
		}

		//pr�fung ob string "zahl1Text" innerhalb des g�ltigen zahlenbereichs "istLongImBereich" liegt...
		if (!istLongImBereich(zahl2Text)) {											//... wenn nicht (!)...
			std::cerr << "Zweite Zahl ist ausserhalb des gueltigen Bereichs!\n";	//...dann wird diese meldung ausgegeben...
			continue;																//...und der schleifendurchlauf �bersprungen, und n�chste eingabe sofort erlaubt;
		}

		//alles g�ltig -> an mikrocontroller senden
		if (!verbindung.zeileSenden(eingabe)) {			//wenn beim senden der benutzereingabe an den mikrocontroller ein fehler auftritt ...
			std::cerr << "Fehler beim Senden!\n";		//...dann wird diese meldung ausgegeben ...
			continue;									//...und der schleifendurchlauf �bersprungen, und n�chste eingabe sofort erlaubt;
		}

		std::string antwort = verbindung.empfangBisEnde("-EOF-");		// wartet auf die antwort des mikrocontrollers; empfang l�uft zeile f�r zeile bis "-EOF-" empfangen wird;
		std::cout << "Antwort vom Mikrocontroller:\n" << antwort << std::endl;		//gibt die empfangene antwort des mikrocontrollers in der konsole aus;
	}

	verbindung.verbindungSchliessen();			//aufruf der methode "verbindungSchliessen" aus der klasse "serielleVerbindung"; diese schlie�t den COM-port;
	std::cout << "\nProgramm beendet.\n";		//gibt diese meldung an den benutzer aus;
	return 0;									//beendet das "main()" programm mit dem wert 0;

};
