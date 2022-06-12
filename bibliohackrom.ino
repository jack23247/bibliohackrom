
/*
 * BiblioHackDay 2019
 * RFID Manager
 */

#include "ESP8266WiFi.h"
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266HTTPClient.h>

// WiFi Password

const char *ssid = "motog";      // Nome AP
const char *pasw = "ciaociao22"; // Password AP

// Section: PINOUT

#define PLG D0 // Pin LED Giallo -> Esercizio
#define PLR D1 // Pin LED Rosso -> Libri OK
#define PLV D2 // Pin LED Verde -> Libri Sbagliati
#define PSP D3 // Pin Speaker

/* Pin Match
 * | RC522 |  Cavo   | ESP8266  |
 * | :---: | :-----: | :------: |
 * |  SDA  |  Viola  | SD3 (10) |
 * |  SCK  | Grigio  |    D5    |
 * | MOSI  | Bianco  |    D7    |
 * | MISO  |  Nero   |    D6    |
 * |  IRQ  |    -    |          |
 * |  GND  |    *    |          |
 * |  RST  | Marrone | SD2 (9)  |
 * | 3.3V  |    *    |          |
 */
#define P_RFID_SDA 10
#define P_RFID_RST 9

MFRC522 mfrc522(P_RFID_SDA, P_RFID_RST);

// Section: FXs

void setup()
{
    pinMode(PSP, OUTPUT);
    pinMode(PLG, OUTPUT);
    pinMode(PLR, OUTPUT);
    pinMode(PLV, OUTPUT);
    beep(50);
    beep(50);
    beep(50);
    digitalWrite(PLG, 1);
    Serial.begin(9600);                // Inizializza la seriale
    Serial.println("\nBiblioHackROM OK!");
    SPI.begin();                       // Inizializza SPI
    mfrc522.PCD_Init();                // Inizializza l'RC522
    mfrc522.PCD_DumpVersionToSerial(); // Stampa la versione del circuito
    WiFi.mode(WIFI_OFF);               // Evita problemi di connessione
    delay(1000);
    WiFi.mode(WIFI_STA); // Client Mode
    setupAPConnection();
    delay(1000);
}

extern int books[] = {0, 0, 0}; // io mi suicidio

void loop()
{
    Serial.flush();
    Serial.println("Main loop!");
    lookForCard();
}

/*===================================*/

void setupAPConnection()
{
    Serial.print("Connessione al WiFi: ");
    WiFi.begin(ssid, pasw);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print("*");
    }
    Serial.print("Connesso! Indirizzo IP: ");
    Serial.print(WiFi.localIP()); // Print the IP address
    Serial.println();
    digitalWrite(PLG, 1);
}

void beep(unsigned char delayms)
{
    analogWrite(PSP, 30);
    delay(delayms);
    analogWrite(PSP, 0);
    delay(delayms);
}

bool lookForCard()
{
    int decimalUid = 0;
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
    {
        beep(100);
        //Serial.println();
        for (int i = 0; i < mfrc522.uid.size; i++)
        {
            decimalUid += mfrc522.uid.uidByte[i];
        }
        //Serial.printf("%d", decimalUid);
    }
    else
    {
        digitalWrite(PLG, 1);
        delay(500);
        digitalWrite(PLG, 0);
        return false;
    }
    
    return startBookLoop(decimalUid);
}

// Sta cosa fa schifo
int matchCardUID(int decimalUid)
{
    switch (decimalUid)
    {
    case 498: // Tessera
        return 830110;
    case 492:
        return 829879;
    default:
        return 666;
    }
}

bool startBookLoop(int decimalUid)
{
    int matricola = matchCardUID(decimalUid);
    Serial.println(matricola);
    if (matricola == 666)
    {
        return bookSeqNack();
    }
    retrieveMatchingBooks(matricola);
    bookPass();
}

bool bookSeqNack()
{
    for (int i = 0; i < 11; i++)
    {
        digitalWrite(PLR, 1);
        beep(50);
        digitalWrite(PLR, 0);
        delay(50);
    }
    cleanupBooks();
    return false;
}

bool bookSeqAck()
{
    cleanupBooks();
    digitalWrite(PLV, 1);
    delay(500);
    beep(500);
    beep(500);
    digitalWrite(PLV, 0);
    return true;
}

// Ottieni lista libri
bool retrieveMatchingBooks(int matricola)
{
    HTTPClient http;
    http.begin("http://bibliohackdaysalsiccia.altervista.org/src.php?search-type=Utente&text=" + String(matricola));
    int httpCode = http.GET();
    String payload = http.getString();
    Serial.println(payload);
    int k, i = 0;
    while (k < 3)
    {
        if (i < payload.length())
        {
            books[k] += int((payload.substring(i, i + 3)).toInt());
            i += 4;
        }
        k++;
    }
    return true;
}

int bookPassCardLoop() {
    int card = 0;
    while (true) {
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
            beep(100);
            for (int i = 0; i < mfrc522.uid.size; i++) {
                card += mfrc522.uid.uidByte[i];
            }
            return card;
        } else {
            digitalWrite(PLV, 1);
            delay(500);
            digitalWrite(PLV, 0);
        }
    }
}

bool booksEmptied() {
    for (int i = 0; i < 3; i++) {
        Serial.println(books[i]);
        if (books[i] != 0) return false;
    }
    return true;
}

bool isIn(int card) {
    bool result = false;
    Serial.println(card);
    for (int i = 0; i < 3; i++) {
        if (books[i] == card) {
            result = true;
            books[i] = 0;
            break;
        }
    }
    return result;
}

bool bookPass() {
  while(!booksEmptied()) {
      if (isIn(bookPassCardLoop())) {
        Serial.println("libri--");
      } else {
            return bookSeqNack();
        }
    }
    return bookSeqAck();
}

bool cleanupBooks() {
    for (int i = 0; i < 3; i++) {
            books[i] = 0;
        }
}
