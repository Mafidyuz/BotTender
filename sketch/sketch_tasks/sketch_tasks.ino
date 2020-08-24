/*
	BOTTIGLIE: vodka, rum, succo d'arancia, coca-cola, sciroppo alla fragola
	DRINK: cuba libre, screwdriver, malibu, rum e arancia, caipiroska alla fragola
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
#include <Statistics.h>

const int SW = 2;
const int DT = 16;
const int CLK = 17;

const int HX711_dout = 4; 
const int HX711_sck = 3; 

const int dirPin[4] = {7, 9, 11, 13}; 
const int stepPin[4] = {8, 10, 12, 14};

int counter = 0;
int currentStateCLK;
int lastStateCLK;
int quantita = 20;
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
	{1./3, 0, 2./3, 0}, //Cuba libre
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

Task oneStepTask(ONE_STEP_RES, TASK_FOREVER, oneStepCallBack);

void oneStepCallBack() {
	digitalWrite(currentStepPin, stepStatus); 
	stepStatus = !stepStatus;
}

void nSteps(int stepPin, int dirPin, bool dir, int nGiri) {
	currentStepPin = stepPin;
	if(!oneStepTask.isEnabled()){
		runner.deleteTask(oneStepTask);
		//Serial.println("is enabled");
		digitalWrite(dirPin, dir);
		//Serial.print("Step pin: ");
		//Serial.println(stepPin);
		runner.addTask(oneStepTask);
		oneStepTask.setIterations(ONE_STEP * nGiri);
		oneStepTask.enable();
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
	//Serial.println(peso);
}

Task writeLcdTask(TASK_SECOND, TASK_FOREVER, writeLcdCallback);

void waitUntilWeightIsStable() {
	lcd.clear();
	lcdPrintCentered("Stabilizzando", 0);
	lcdPrintCentered("bilancia...", 1);
	Statistics stats(10);
	float p;
	do {
		stats.reset();
		for(int i=0; i<10; i++){
			if(LoadCell.update()) 
				p = LoadCell.getData();
			else
				i--;
			stats.addData(p);
			Serial.print("P: ");
			Serial.println(p);
			delay(100);
		}
	Serial.print("std: ");
	Serial.println(stats.stdDeviation());
	} while(stats.stdDeviation() > 0.5);
}

void serve() {
		int ingredient = 0;
		waitUntilWeightIsStable();
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
			runner.execute();

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
		
			nSteps(stepPin[ingredient], dirPin[ingredient], LOW, 1);
			
			if(peso > pesoIngrediente)  {
				if(ingredient++ < nIngredients)
					firstIterationIngredient = true;
				lastTimeIsDone = millis();
			}
			
		} while(millis() - lastTimeIsDone < 200 || ingredient < nIngredients && ingredient >= 0);
		oneStepTask.disable();
		writeLcdTask.disable();
		runner.deleteTask(oneStepTask);
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
	/*lcd.clear();
	lcdPrintCentered("Servendo",0);
	lcdPrintCentered(drinks[counter] + "...",1);
	Serial.print("Servendo ");
	Serial.println(drinks[counter]);*/
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
}

void loop() {
	runner.execute();
	buttonRoutine();
}

