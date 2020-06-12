/*
    “Chi beve solo acqua ha un segreto da nascondere.” - Charles Pierre Baudelaire
	“Ecco il problema di chi beve, pensai versandomene un altro: se succede qualcosa di brutto si beve per dimenticare; se succede qualcosa di bello si beve per festeggiare; se non succede niente si beve per far succedere qualcosa.” - Charles Bukowski
    “Io non mi fido di nessun bastardo che non beva. La gente che non beve ha paura di rivelare se stessa.” - Humphrey Bogart
    “Bevo per rendere le altre persone interessanti.” - George Jean Nathan
    “Penso che penso troppo... ecco perché bevo.” - Janis Joplin
    “Purtroppo è difficile dimenticare qualcuno bevendo un'orzata.” - Hugo Eugenio Pratt
	"I feel sorry for people who don't drink. When they wake up in the morning that's as good as they're going to feel all day." ~ Frank Sinatra
	"Always remember that I have taken more out of alcohol than alcohol has taken out of me." ~ Winston Churchill
	"I drink to make other people more interesting." ~ Ernest Hemingway
	"It takes only one drink to get me drunk.....the trouble is, I can't remember if it's the thirteenth or the fourteenth." ~ George Burns
	"Here’s to alcohol, the cause of — and solution to — all life’s problems." ~ Homer Simpson
	
*/

#define CLK 3
#define DT 4
#define SW 2

#define MENU 0
#define CONFIRM 1
#define SERVING 2

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);

int counter = 0;
int currentStateCLK;
int lastStateCLK;
bool counterChanged = false;
bool buttonPressed = false;
bool hasChangedState = false;
bool confirm = true; 
unsigned long lastButtonPress = 0;
int state = MENU;
String drinks [3] = {"Cuba libre", "Mojito", "Gin tonic"};
int nDrinks = 3;

void buttonISR() {
    buttonPressed = true;
}

void buttonRoutine() {
    if (buttonPressed && (millis() - lastButtonPress > 500)) {
        Serial.print("Button pressed! ");
		Serial.println(millis());
		state = (state + 1) % 3;
		lastButtonPress = millis();
		Serial.print("State: ");
		Serial.println(state);
		hasChangedState = true;
	}
	buttonPressed = false;
}
void lcdPrintCentered(String s, int position) {
	int center = (16 - s.length()) / 2;
	lcd.setCursor(center, position);
	lcd.print(s);
}

void menuRoutine() {
	counter = (digitalRead(DT) != currentStateCLK) ? (counter - 1 + nDrinks) % nDrinks : counter = (counter + 1) % nDrinks;
	lcd.clear();
	lcd.print("Seleziona drink!");
	lcdPrintCentered(drinks[counter], 1);
}

void confirmRoutine() {
	confirm = !confirm;
	lcd.clear();
	lcdPrintCentered(drinks[counter] + "?", 0);
	lcd.setCursor(0, 1);
	Serial.println(confirm);
	if (confirm) 
		lcd.print(" [SI]       NO  ");
	else 
		lcd.print("  SI       [NO] ");
}

void serve() {
	if(confirm) {
		lcd.clear();
		lcdPrintCentered("Servendo",0);
		lcdPrintCentered(drinks[counter] + "...",1);
		Serial.print("Servendo ");
		Serial.println(drinks[counter]);
	}
	state = MENU;	
	hasChangedState = true;
}

void rotatoryEncoderRoutine() {
    currentStateCLK = digitalRead(CLK);
	if (hasChangedState || currentStateCLK != lastStateCLK  && currentStateCLK == 1){
		hasChangedState = false;
		switch(state){
			case MENU:
				menuRoutine();
				break;
			case CONFIRM:
				confirmRoutine();
				break;
			case SERVING:
				serve();
				break;
			default:
				Serial.println("ERRORE");
				break;
		}
	}
	lastStateCLK = currentStateCLK;
}

void setup() {
	pinMode(CLK,INPUT);
	pinMode(DT,INPUT);
	pinMode(SW, INPUT_PULLUP);

	Serial.begin(9600);

	lcd.init();
	lcd.backlight();
	lcdPrintCentered("Ciao! sono il", 0);
	lcdPrintCentered("bottender!", 1);

	lastStateCLK = digitalRead(CLK);

    attachInterrupt(digitalPinToInterrupt(SW), buttonISR, FALLING);
}
void loop() {
	rotatoryEncoderRoutine();
	buttonRoutine();

	delay(1);
}
