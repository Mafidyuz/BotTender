
const int dirPin[4] = {7, 9, 11, 13}; 
const int stepPin[4] = {8, 10, 12, 14};

void oneStep(int stepPin) {
	for(int i = 0; i < 200; i++) {
		digitalWrite(stepPin,HIGH); 
		delayMicroseconds(600); 
		digitalWrite(stepPin,LOW); 
		delayMicroseconds(600); 
	}
}

void nSteps(int stepPin, int dirPin, bool dir, int nGiri) {
	digitalWrite(dirPin,dir);
	for (int i = 0; i < nGiri; i++)
		oneStep(stepPin);
}

void setup() {
    for(int i=0; i<4; i++) {
		pinMode(stepPin[i], OUTPUT);
		pinMode(dirPin[i], OUTPUT);
	}

}

void loop() {
    for(int i=0; i<4; i++)
        nSteps(stepPin[i], dirPin[i], LOW, 5);
}