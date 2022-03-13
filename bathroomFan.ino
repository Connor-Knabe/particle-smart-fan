// This #include statement was automatically added by the Particle IDE.
#include <HTU21D.h>
HTU21D htu = HTU21D();

// This #include statement was automatically added by the Particle IDE.
#include <WebServer.h>

#include <OneWire.h>


/* Function prototypes -------------------------------------------------------*/
int tinkerDigitalRead(String pin);
int tinkerDigitalWrite(String command);
int tinkerAnalogRead(String pin);
int tinkerAnalogWrite(String command);
int setHumidityTrigger(String value);
int pauseFan(String command);

int pinZero = A0;
int pinOne = A1;
int onOff = 0;

#define PREFIX ""
WebServer webserver(PREFIX, 80);


void helloCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
    /* this line sends the standard "we're all OK" headers back to the
     browser */
    server.httpSuccess();
    RGB.control(true);

    /* if we're handling a GET or POST, we can output our data here.
     For a HEAD request, we just stop after outputting headers. */
    if (type != WebServer::HEAD)
    {
        /* this defines some HTML text in read-only memory aka PROGMEM.
         * This is needed to avoid having the string copied to our limited
         * amount of RAM. */
        P(helloMsg) = "<h1>bathroom-fan!</h1>";
        
        /* this is a special form of print that outputs from PROGMEM */
        server.printP(helloMsg);
    }
}

long humidity = 0;
bool hasSentError = false;
/* This function is called once at start up ----------------------------------*/
void setup() {
	//Setup the Tinker application here
	//Register all the Tinker functions

    Particle.function("digitalread", tinkerDigitalRead);
	Particle.function("digitalwrite", tinkerDigitalWrite);
	Particle.function("analogread", tinkerAnalogRead);
	Particle.function("analogwrite", tinkerAnalogWrite);
    Particle.function("pauseFan", pauseFan);
    
    
    pinMode(pinZero, OUTPUT);
    pinMode(pinOne, OUTPUT);

    digitalWrite(pinZero, HIGH);    
    digitalWrite(pinOne, HIGH);  
    Time.zone(-6);

    int curHour = Time.hour();
    char str[15];
    sprintf(str, "%d", curHour);
    webserver.setDefaultCommand(&helloCmd);
    
    /* run the same command if you try to load /index.html, a common
    * default page name */
    webserver.addCommand("index.html", &helloCmd);
    // HT21D sensor setup
    while(!htu.begin()){
        Serial.println("Couldnt find HTU21D");
        if(!hasSentError){
            Particle.publish("bathroom-fan-err", NULL, 60, PRIVATE);    
            hasSentError = true;
        }
        delay(1000);
    }
    hasSentError = false;

    Particle.variable("humidity", humidity);

}

unsigned long lastTimeOne = 0;
unsigned long lastTime = 0;
// bool fanNeedsToTurnOn = true;
// bool fanNeedsToTurnOff = false;
bool bathroomFanIsOn = false;
int delayMins = 2;
int delayMinsOne = 1;
unsigned int nextTime = 0;    
bool shouldPauseFan = false;

void loop(){

    char buff[64];
    int len = 64;
    
    /* process incoming connections one at a time forever */
    webserver.processConnection(buff, &len);

    unsigned long now = millis();
    if ((now - lastTime) >=  delayMins * 60 * 1000) {
    	lastTime = now;
        if(shouldPauseFan){
            shouldPauseFan = false;
        }
    }

    unsigned long nowOne = millis();
    if ((nowOne - lastTimeOne) >= delayMinsOne * 60 * 1000) {
		lastTimeOne = nowOne;

        long temperature = htu.readTemperature();
        long tempInF = temperature * 1.8 + 32;
        humidity = htu.readHumidity();
        char tempStr[15];
        sprintf(tempStr, "%ld", tempInF);
        char humidStr[15];
        sprintf(humidStr, "%ld", humidity);

        if (nextTime < millis()) {
            Particle.publish("bathroom-fan-humidity", humidStr, 60, PRIVATE);
            nextTime = millis() + 60*60*1000;
        }
        
        if(humidity<62){
            delayMinsOne = .5;
            if(bathroomFanIsOn){
                Particle.publish("bathroom_fan_off", NULL, 60,  PRIVATE);
                bathroomFanIsOn = false;
            }
        } else if(humidity>75 && !shouldPauseFan) {
            if(!bathroomFanIsOn){
                Particle.publish("bathroom_fan_on", NULL, 60, PRIVATE);
                bathroomFanIsOn = true;
            }
            delayMinsOne = 2;
        }
        
        if(isNight){
            RGB.control(true); 
            RGB.brightness(0);
            // Particle.publish("RGB LED","Is night time turn RGB LED light off");
        } else {
            RGB.brightness(255);
            RGB.control(false); 
            // Particle.publish("RGB LED","Is daytime turn RGB LED light on");
        }
    }
    
}

bool isNight(){
    return Time.hour()>=20 && Time.hour()<=7;
}

int pauseFan(String p){
    delayMins = 30;
    if(bathroomFanIsOn){
        Particle.publish("bathroom_fan_off", NULL, 60,  PRIVATE);
        bathroomFanIsOn = false;
    }
    
    if(!shouldPauseFan){
        shouldPauseFan = true;
    } else {
        shouldPauseFan = false;
    }
    return 0;
}

/*******************************************************************************
 * Function Name  : tinkerDigitalRead
 * Description    : Reads the digital value of a given pin
 * Input          : Pin
 * Output         : None.
 * Return         : Value of the pin (0 or 1) in INT type
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerDigitalRead(String pin)
{
	//convert ascii to integer
	int pinNumber = pin.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	if(pin.startsWith("D"))
	{
		pinMode(pinNumber, INPUT_PULLDOWN);
		return digitalRead(pinNumber);
	}
	else if (pin.startsWith("A"))
	{
		pinMode(pinNumber+10, INPUT_PULLDOWN);
		return digitalRead(pinNumber+10);
	}
	return -2;
}

/*******************************************************************************
 * Function Name  : tinkerDigitalWrite
 * Description    : Sets the specified pin HIGH or LOW
 * Input          : Pin and value
 * Output         : None.
 * Return         : 1 on success and a negative number on failure
 *******************************************************************************/
int tinkerDigitalWrite(String command)
{
	bool value = 0;
	//convert ascii to integer
	int pinNumber = command.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	if(command.substring(3,7) == "HIGH") value = 1;
	else if(command.substring(3,6) == "LOW") value = 0;
	else return -2;

	if(command.startsWith("D"))
	{
		pinMode(pinNumber, OUTPUT);
		digitalWrite(pinNumber, value);
		return 1;
	}
	else if(command.startsWith("A"))
	{
		pinMode(pinNumber+10, OUTPUT);
		digitalWrite(pinNumber+10, value);
		return 1;
	}
	else return -3;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogRead
 * Description    : Reads the analog value of a pin
 * Input          : Pin
 * Output         : None.
 * Return         : Returns the analog value in INT type (0 to 4095)
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerAnalogRead(String pin)
{
	//convert ascii to integer
	int pinNumber = pin.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	if(pin.startsWith("D"))
	{
		return -3;
	}
	else if (pin.startsWith("A"))
	{
		return analogRead(pinNumber+10);
	}
	return -2;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogWrite
 * Description    : Writes an analog value (PWM) to the specified pin
 * Input          : Pin and Value (0 to 255)
 * Output         : None.
 * Return         : 1 on success and a negative number on failure
 *******************************************************************************/
int tinkerAnalogWrite(String command)
{
	//convert ascii to integer
	int pinNumber = command.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	String value = command.substring(3);

	if(command.startsWith("D"))
	{
		pinMode(pinNumber, OUTPUT);
		analogWrite(pinNumber, value.toInt());
		return 1;
	}
	else if(command.startsWith("A"))
	{
		pinMode(pinNumber+10, OUTPUT);
		analogWrite(pinNumber+10, value.toInt());
		return 1;
	}
	else return -2;
}