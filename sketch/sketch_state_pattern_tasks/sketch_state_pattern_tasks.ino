/*
	BOTTIGLIE: vodka, rum, succo d'arancia, coca-cola, sciroppo alla fragola
	DRINK: cuba libre, screwdriver, malibu, rum e arancia, caipiroska alla fragola
*/

#define _TASK_MICRO_RES
#define ONE_STEP_RES (600L)
#define ONE_STEP 400

#include <HX711_ADC.h>
#include <EEPROM.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <TaskScheduler.h>
#include <Statistics.h>

class Context;

class State {
    protected:
        int x;
    public:
        void doAction(Context &context);
};


class Context {
    protected:
        State state;

        int counter = 0;
        const int SW = 2;
        const int DT = 16;
        const int CLK = 17;

        const int HX711_dout = 4; 
        const int HX711_sck = 3; 

        const int dirPin[4] = {5, 7, 9, 11}; 
        const int stepPin[4] = {6, 8, 10, 12};

        int currentStateCLK;
        int lastStateCLK;
        int quantita = 250;
        bool counterChanged = false;
        bool buttonPressed = false;
        bool hasChangedState = false;
        bool confirm = true; 
        unsigned long lastButtonPress = 0;

        static const int nDrinks = 3;
        static const int nIngredients = 4;
        String drinks[nDrinks] = {"Cuba libre", "Rum e arancia", "Rum tonic"};

        //bottiglie: rum, succo d'arancia, coca-cola, acqua tonica
        float proporzioni [nDrinks][nIngredients] = {
            {0, 1./3, 2./3, 0}, //Cuba libre
            {1./4, 3./4, 0, 0},	//Rum e arancia
            {1./4, 0, 0, 3./4},	//Rum tonic
        };

        Scheduler runner;

        LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27,20,4);

        HX711_ADC LoadCell = HX711_ADC(HX711_dout, HX711_sck);
        const int calVal_eepromAdress = 0;

    public:

        void setState(State s) {
            state = s;
        }

        State getState() {
            return state;
        }

        void request() {
            currentStateCLK = digitalRead(CLK);
            if (hasChangedState || currentStateCLK != lastStateCLK  && currentStateCLK == 1){
		        hasChangedState = false;
                state.doAction(this);
            }
            lastStateCLK = currentStateCLK;
        }

        void changeCounter() {
            counter = (digitalRead(DT) != currentStateCLK) ? (counter - 1 + nDrinks) % nDrinks : counter = (counter + 1) % nDrinks; //se DT e currentStateCLK sono diversi allora abbiamo girato in senso antiorario, altrimenti in senso orario 
            lcd.clear();
            lcd.print("Seleziona drink!");
            lcdPrintCentered(drinks[counter], 1);
        }
};

class Menu: public State {
    public: 
        void doAction(Context &context) {
            context.changeCounter();
        }
};

void setup()
{
    
}

void loop()
{
    
}