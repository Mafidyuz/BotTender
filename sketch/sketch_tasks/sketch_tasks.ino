/*
	BOTTIGLIE: vodka, rum, succo d'arancia, coca-cola, sciroppo alla fragola
	DRINK: cuba libre, screwdriver, malibu, rum e arancia, caipiroska alla fragola

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

#define _TASK_MICRO_RES
#define ONE_STEP_RES (600L)
#define ONE_STEP 400

#define MENU 0
#define CONFIRM 1
#define QUANTITA 2
#define SERVING 3

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <TaskScheduler.h>

Scheduler runner;


LiquidCrystal_I2C lcd(0x27,20,4);

const int SW = 2;
const int DT = 16;
const int CLK = 17;

const int dirPin[4] = {5, 7, 9, 11}; 
const int stepPin[4] = {6, 8, 10, 12};

const int triggerPort = 14;
const int echoPort = 15;

int counter = 0;
int currentStateCLK;
int lastStateCLK;
int quantita = 0;
bool counterChanged = false;
bool buttonPressed = false;
bool hasChangedState = false;
bool confirm = true; 
unsigned long lastButtonPress = 0;
int state = MENU;

const int nDrinks = 3;
const int nIngredients = 4;
String drinks[nDrinks] = {"Cuba libre", "Rum e arancia", "Rum tonic"};

//bottiglie: rum, succo d'arancia, coca-cola, acqua tonica
float proporzioni [nDrinks][nIngredients] = {
	{0, 1./3, 2./3, 0}, //Cuba libre
	{1./4, 3./4, 0, 0},	//Rum e arancia
	{1./4, 0, 0, 3./4},	//Rum tonic
};

void buttonISR() {
	buttonPressed = true;
}

void buttonRoutine() {
	if (buttonPressed && (millis() - lastButtonPress > 500)) {
		Serial.print("Button pressed! ");
		Serial.println(millis());
		state = (state + 1) % 4;
		lastButtonPress = millis();
		Serial.print("State: ");
		Serial.println(state);
		hasChangedState = true;
	}
	buttonPressed = false;
}

int currentStepPin = 8;
bool stepStatus = true;

bool oneStepIsRunning = false;

Task oneStepTask(ONE_STEP_RES, ONE_STEP, oneStepCallBack);

void oneStepCallBack() {
	digitalWrite(currentStepPin, stepStatus); 
	stepStatus = !stepStatus;
}

bool blinkStatus = true;
Task blinkTask(TASK_SECOND, ONE_STEP, blinkCallback);

void blinkCallback() {
	digitalWrite(LED_BUILTIN, blinkStatus); 
	blinkStatus = !blinkStatus;
}

void nSteps(int stepPin, int dirPin, bool dir, int nGiri) {
	currentStepPin = stepPin;
	if(!oneStepTask.isEnabled()){
		Serial.println("is enabled");
		digitalWrite(dirPin, dir);
		Serial.print("Step pin: ");
		Serial.println(stepPin);
		runner.addTask(oneStepTask);
		oneStepTask.setIterations(ONE_STEP * nGiri);
		oneStepTask.enable();	
	} else {
		runner.deleteTask(oneStepTask);
	}
}

void lcdPrintCentered(String s, int position) {
	int center = (16 - s.length()) / 2;
	lcd.setCursor(center, position);
	lcd.print(s);
}

void menuRoutine() {
	counter = (digitalRead(DT) != currentStateCLK) ? (counter - 1 + nDrinks) % nDrinks : counter = (counter + 1) % nDrinks; //se DT e currentStateCLK sono diversi allora abbiamo girato in senso antiorario, altrimenti in senso orario 
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

float microsecondsToCentimeters(long microseconds) {
	return round(3.4 * microseconds / 2.) / 100.;
}

void serve() {
		int ingredient = 0;
		long durata;
		float distanza;
		float distanzaIniziale;
		float distanzaIngrediente;
		float distanzaFinale;
		unsigned long lastTimeIsDone = 0;
		bool firstIteration = true;
		bool firstIterationIngredient = true;
		
		do{
			//Prendi distanza
			runner.execute();
			/*digitalWrite( triggerPort, LOW );
			delayMicroseconds(2);
			digitalWrite( triggerPort, HIGH );
			delayMicroseconds( 10 );
			digitalWrite( triggerPort, LOW );
			durata = pulseIn( echoPort, HIGH );
			distanza = microsecondsToCentimeters(durata);			
			*/
			durata = 0;
			distanza = 0;
			if (firstIteration){
				distanzaIniziale = distanza;
				firstIteration = false;
				distanzaFinale = distanzaIniziale - quantita;
				Serial.print("DistanzaIniziale: ");
				Serial.println(distanzaIniziale);
			}

			if(firstIterationIngredient){
				while(proporzioni[counter][ingredient] == 0 && ingredient < nIngredients)
					ingredient++; 

				distanzaIngrediente = distanza - proporzioni[counter][ingredient] * quantita;
				Serial.print("Distanza: ");
				Serial.println(distanza);

				Serial.print("DistanzaIngrediente: ");
				Serial.println(distanzaIngrediente);
				Serial.print("Ingrendiente: ");
				Serial.println(ingredient);
				Serial.println();
				
				firstIterationIngredient = false;
			}

			nSteps(stepPin[ingredient], dirPin[ingredient], LOW, 1);
			/*
			lcd.setCursor(0, 1);
			lcd.print("dist.: ");

			//dopo 38ms è fuori dalla portata del sensore
			if( durata > 38000 ){
				lcd.setCursor(0, 1); 
				lcd.println("Fuori portata   ");
			} 
			else{ 
				lcd.print(distanza); 
				lcd.println(" cm     ");
			}*/

			if(distanza < distanzaIngrediente)  {
				if(ingredient++ < nIngredients)
					firstIterationIngredient = true;
				lastTimeIsDone = millis();
			}
			
		} while(millis() - lastTimeIsDone < 200 || ingredient < nIngredients);
}

void selectQuantitaRoutine() {
	if(confirm){
		quantita = (digitalRead(DT) != currentStateCLK) ? (quantita - 1 + 15) % 15 : quantita = (quantita + 1) % 15;
		lcd.clear();
		lcdPrintCentered("Quanti cm vuoi?",0);

		lcd.setCursor(0,1);
		lcd.print("cm: ");
		lcd.print(quantita);
	} else {
		state = MENU;	
		hasChangedState = true;
	}
}

void serveRoutine() {
	lcd.clear();
	lcdPrintCentered("Servendo",0);
	lcdPrintCentered(drinks[counter] + "...",1);
	Serial.print("Servendo ");
	Serial.println(drinks[counter]);
	serve();
	state = MENU;	
	hasChangedState = true;
}

void rotatoryEncoderCallback() {
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
			case QUANTITA:
				selectQuantitaRoutine();
				break;
			case SERVING:
				serveRoutine();
				break;
			default:
				Serial.println("ERRORE");
				break;
		}
	}
	lastStateCLK = currentStateCLK;
}

Task rotatoryEncoderTask(TASK_MILLISECOND, TASK_FOREVER, rotatoryEncoderCallback);

void setup() {
	pinMode(CLK,INPUT);
	pinMode(DT,INPUT);
	pinMode(SW, INPUT_PULLUP);

	for(int i=0; i<4; i++) {
		pinMode(stepPin[i], OUTPUT);
		pinMode(dirPin[i], OUTPUT);
	}

	pinMode(triggerPort, OUTPUT);
	pinMode(echoPort, INPUT);

	Serial.begin(9600);

		lcd.init();
	lcd.backlight();
	lcdPrintCentered("Ciao! Posiziona ", 0);
	lcdPrintCentered("il bicchiere!", 1);

	lastStateCLK = digitalRead(CLK);

    attachInterrupt(digitalPinToInterrupt(SW), buttonISR, FALLING);

	pinMode(LED_BUILTIN, OUTPUT);
	runner.init();
	runner.addTask(rotatoryEncoderTask);
	runner.addTask(blinkTask);
	blinkTask.enable();
	rotatoryEncoderTask.enable();
}

void loop() {
	runner.execute();
	buttonRoutine();
}
