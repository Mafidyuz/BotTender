#define ONE_STEP_RES (600L)
#define _TASK_MICRO_RES

#include <HX711_ADC.h>
#include <EEPROM.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <TaskScheduler.h>
#include <Statistics.h>


//States
#define MENU 0
#define CONFIRM 1
#define QUANTITY 2
#define SERVE 3

//Rotatory encoder's pin & variables
const int SW = 2;
const int DT = 16;
const int CLK = 17;
int currentStateCLK;
int lastStateCLK;
int counter = 0;
bool buttonPressed = false;
unsigned long lastButtonPress = 0;

//Stepper motor's pin
const int dirPin[4] = {7, 9, 11, 13}; 
const int stepPin[4] = {8, 10, 12, 14};

//States variables
bool hasChangedState = false;
bool confirm = true; 
int state = MENU;

//Other variables
int ml = 20;
const int nDrinks = 3;
const int nIngredients = 4;
String drinks[nDrinks] = {"Cuba libre", "Rum e arancia", "Rum tonic"};

//Bottles: rum, orange juice, coca-cola, tonic water
float proportions [nDrinks][nIngredients] = {
	{1./3, 0, 2./3, 0}, //Cuba libre
	{1./4, 3./4, 0, 0},	//Rum e arancia
	{1./4, 0, 0, 3./4},	//Rum tonic
};

//LCD
LiquidCrystal_I2C lcd(0x27,20,4);

//LoadCell
const int HX711_sck = 3;
const int HX711_dout = 4; 
HX711_ADC LoadCell(HX711_dout, HX711_sck);
float weight;

//Task scheduler, callback e task
Scheduler runner;

void motorCallback();
void rotatoryEncoderCallback();

Task motorTask(ONE_STEP_RES, TASK_FOREVER , motorCallback);
Task rotatoryEncoderTask(TASK_MILLISECOND, TASK_FOREVER, rotatoryEncoderCallback);

//Stepper motor's functions
int currentStepPin = 8;
bool stepStatus = true;

void motorCallback() {
	digitalWrite(currentStepPin, stepStatus); 
	stepStatus = !stepStatus;
}

void runMotor(int stepPin, int dirPin, bool dir) {
	currentStepPin = stepPin;
	if(!motorTask.isEnabled()){
		Serial.println("is enabled");
		Serial.print("Step pin: ");
		Serial.println(stepPin);
		runner.deleteTask(motorTask);
		digitalWrite(dirPin, dir);
		runner.addTask(motorTask);
		motorTask.enable();
	}
}

//LCD functions
void lcdPrintCentered(String s, int position) {
	int center = (16 - s.length()) / 2;
	lcd.setCursor(center, position);
	lcd.print(s);
}

//Button Interrupt Service Routine
void buttonISR() {
	buttonPressed = true;
}

//Check if button was pressed
void checkButtonPressed() {
	if (buttonPressed && (millis() - lastButtonPress > 500)) {	//debouncing
		state = (state + 1) % 4;
		lastButtonPress = millis();
		hasChangedState = true;
	}
	buttonPressed = false;
}

//Rotatory encoder's and change of state's routines
void menuRoutine() {
	counter = (digitalRead(DT) != currentStateCLK) ? (counter - 1 + nDrinks) % nDrinks : counter = (counter + 1) % nDrinks; //se DT e currentStateCLK sono diversi allora abbiamo girato in senso antiorario, altrimenti in senso orario 
	lcd.clear();
	lcdPrintCentered("Select drink!", 0);
	lcdPrintCentered(drinks[counter], 1);
}

void confirmRoutine() {
	confirm = !confirm;
	lcd.clear();
	lcdPrintCentered(drinks[counter] + "?", 0);
	lcd.setCursor(0, 1);

	Serial.println(confirm);
	if (confirm) 
		lcd.print(" [YES]      NO  ");
	else 
		lcd.print("  YES      [NO] ");
}

void selectQuantityRoutine() {
	if(confirm){
		ml = (digitalRead(DT) != currentStateCLK) ? (ml - 10 + 500) % 500 : ml = (ml + 10) % 500;
		lcd.clear();
		lcdPrintCentered("How many ml?",0);

		lcd.setCursor(0,1);
		lcd.print("ml: ");
		lcd.print(ml);
	} else {
		state = MENU;	
		hasChangedState = true;
	}
}

void serveRoutine() {
		int ingredient = 0;
		waitUntilWeightIsStable();
		weight = LoadCell.getData();
		lcd.clear();
		lcdPrintCentered("Serving...",0);
		lcd.setCursor(0,1);
		lcd.print("[--------------]");

		float initialWeight;
		float ingredientWeight;
		float finalWeight;
		unsigned long lastTimeIsDone = 0;
		bool firstIteration = true;
		bool firstIterationIngredient = true;
		int percentual = 0;
		do{
			runner.execute();

			if (LoadCell.update())
				weight = LoadCell.getData();

			if (firstIteration){
				initialWeight = weight;
				firstIteration = false;
				Serial.print("Initial weight: ");
				Serial.println(initialWeight);
				finalWeight = initialWeight + ml;
			}

			if(firstIterationIngredient){
				while(proportions[counter][ingredient] == 0 && ingredient < nIngredients)
					ingredient++; 

				ingredientWeight = weight + proportions[counter][ingredient] * ml;
				Serial.print("weight: ");
				Serial.println(weight);

				Serial.print("Ingredient's weight: ");
				Serial.println(ingredientWeight);
				Serial.print("Ingredient: ");
				Serial.println(ingredient);
				Serial.println();

				firstIterationIngredient = false;
			}

			if(round(14 - (finalWeight - weight) / ml * 14) > percentual && percentual < 14 ){
				percentual++;
				lcd.setCursor(percentual, 1);
				lcd.print("X");
			}     

			runMotor(stepPin[ingredient], dirPin[ingredient], LOW);
			
			if(weight > ingredientWeight)  {
				if(ingredient++ < nIngredients)
					firstIterationIngredient = true;
				lastTimeIsDone = millis();
			}
			
		} while(millis() - lastTimeIsDone < 200 || ingredient < nIngredients && ingredient >= 0);
		motorTask.disable();
		runner.deleteTask(motorTask);
		state = MENU;	
		hasChangedState = true;
}

//Load cell stabilizator 
void waitUntilWeightIsStable() {
	lcd.clear();
	lcdPrintCentered("Stabilizing", 0);
	lcdPrintCentered("load cell...", 1);
	Statistics stats(10);
	float w;
	do {
		stats.reset();
		for(int i=0; i<10; i++){
			if(LoadCell.update()) w = LoadCell.getData();
			else i--;
			stats.addData(w);
			Serial.print("w: ");
			Serial.println(w);
			delay(100);
		}
	Serial.print("std: ");
	Serial.println(stats.stdDeviation());
	} while(stats.stdDeviation() > 0.5);
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
			case QUANTITY:
				selectQuantityRoutine();
				break;
			case SERVE:
				serveRoutine();
				break;
			default:
				Serial.println("SHOULD NEVER GET HERE");
				break;
		}
	}
	lastStateCLK = currentStateCLK;
}

void setup() {
	Serial.begin(57600); 

	//Rotatory encoder
	pinMode(CLK,INPUT);
	pinMode(DT,INPUT);
	pinMode(SW, INPUT_PULLUP);
	lastStateCLK = digitalRead(CLK);
	attachInterrupt(digitalPinToInterrupt(SW), buttonISR, FALLING);

	//Stepper motors
	for(int i=0; i<4; i++) {
		pinMode(stepPin[i], OUTPUT);
		pinMode(dirPin[i], OUTPUT);
	}

	//LCD
	lcd.init();
	lcd.backlight();
	lcdPrintCentered("Hi! Place ", 0);
	lcdPrintCentered("your glass!", 1);
	
	//Task scheduler
	runner.init();
	runner.addTask(rotatoryEncoderTask);
	rotatoryEncoderTask.enable();

	//Load cell
	LoadCell.begin();
	float calibrationValue; 
	EEPROM.get(0, calibrationValue); 
	long stabilizingtime = 500;
	boolean _tare = true; 
	LoadCell.start(stabilizingtime, _tare);
	LoadCell.setCalFactor(calibrationValue);
}

void loop() {
	runner.execute();
	checkButtonPressed();
}

