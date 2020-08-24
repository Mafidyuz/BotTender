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

#include <HX711_ADC.h>
#include <EEPROM.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <TaskScheduler.h>

const int SW = 2;
const int DT = 16;
const int CLK = 17;

const int HX711_dout = 4; 
const int HX711_sck = 3; 

const int dirPin[4] = {5, 7, 9, 11}; 
const int stepPin[4] = {6, 8, 10, 12};

int counter = 0;
int currentStateCLK;
int lastStateCLK;
int quantita = 250;
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

Scheduler runner;

LiquidCrystal_I2C lcd(0x27,20,4);

HX711_ADC LoadCell(HX711_dout, HX711_sck);
const int calVal_eepromAdress = 0;

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

float peso;

void writeLcdCallback() {
	lcd.setCursor(0, 1);
	lcd.print("peso.: ");
	lcd.print(peso);
}

Task writeLcdTask(TASK_SECOND, TASK_FOREVER, writeLcdCallback);


void serve() {
		int ingredient = 0;
		peso = LoadCell.getData();
		float pesoIniziale;
		float pesoIngrediente;
		float pesoFinale;
		unsigned long lastTimeIsDone = 0;
		bool firstIteration = true;
		bool firstIterationIngredient = true;
		runner.addTask(writeLcdTask);
		writeLcdTask.enable();
		do{
			//runner.execute();

			if (LoadCell.update())
				peso = LoadCell.getData();

			if (firstIteration){
				pesoIniziale = peso;
				firstIteration = false;
				pesoFinale = pesoIniziale + quantita;
				Serial.print("Peso Iniziale: ");
				Serial.println(pesoIniziale);
			}

			if(firstIterationIngredient){
				while(proporzioni[counter][ingredient] == 0 && ingredient < nIngredients)
					ingredient++; 

				pesoIngrediente = peso + proporzioni[counter][ingredient] * quantita;
				Serial.print("Peso: ");
				Serial.println(peso);

				Serial.print("Peso Ingrediente: ");
				Serial.println(pesoIngrediente);
				Serial.print("Ingrendiente: ");
				Serial.println(ingredient);
				Serial.println();

				firstIterationIngredient = false;
			}

			//nSteps(stepPin[ingredient], dirPin[ingredient], LOW, 1);
			
			
			
			//Serial.println(peso);
			if(peso > pesoIngrediente)  {
				if(ingredient++ < nIngredients)
					firstIterationIngredient = true;
				lastTimeIsDone = millis();
			}
			
		} while(millis() - lastTimeIsDone < 200 || ingredient < nIngredients);
		runner.deleteTask(writeLcdTask);
}

void selectQuantitaRoutine() {
	if(confirm){
		quantita = (digitalRead(DT) != currentStateCLK) ? (quantita - 10 + 500) % 500 : quantita = (quantita + 10) % 500;
		lcd.clear();
		lcdPrintCentered("Quanti ml vuoi?",0);

		lcd.setCursor(0,1);
		lcd.print("ml: ");
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
	Serial.begin(57600); 

	pinMode(CLK,INPUT);
	pinMode(DT,INPUT);
	pinMode(SW, INPUT_PULLUP);

	for(int i=0; i<4; i++) {
		pinMode(stepPin[i], OUTPUT);
		pinMode(dirPin[i], OUTPUT);
	}

	lcd.init();
	lcd.backlight();
	lcdPrintCentered("Ciao! Posiziona ", 0);
	lcdPrintCentered("il bicchiere!", 1);

	lastStateCLK = digitalRead(CLK);

    attachInterrupt(digitalPinToInterrupt(SW), buttonISR, FALLING);

	pinMode(LED_BUILTIN, OUTPUT);
	runner.init();
	runner.addTask(rotatoryEncoderTask);
	rotatoryEncoderTask.enable();

	LoadCell.begin();
	float calibrationValue; 
	EEPROM.get(calVal_eepromAdress, calibrationValue); 
	long stabilizingtime = 500;
	boolean _tare = true; 
	LoadCell.start(stabilizingtime, _tare);
	LoadCell.setCalFactor(calibrationValue);
	if (LoadCell.getTareTimeoutFlag()) {
		Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
	}
	else {
		LoadCell.setCalFactor(calibrationValue); // set calibration factor (float)
		Serial.println("Startup is complete");
	}
}

void loop() {
	runner.execute();
	buttonRoutine();
}

