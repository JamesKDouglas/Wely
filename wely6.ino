// This #include statement was automatically added by the Particle IDE.
#include <ThingSpeak.h>

// This #include statement was automatically added by the Particle IDE.
#include <HX711ADC.h>

// This #include statement was automatically added by the Particle IDE.
#include <RTClibrary.h>

#include <math.h> // this is a default C++ library, you don't have to add it through the IDE just include it.

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));
SYSTEM_MODE(SEMI_AUTOMATIC);

retained char data_r[500];
retained long tare;
retained long taretemp;

//Thingspeak channel parameters.
String CHANNEL_ID = "647261";//wely 6
String WRITE_API_KEY = "0PX8K4BFLB241JJ3";//wely 6
String server = "api.thingspeak.com"; // ThingSpeak Server
TCPClient client;

size_t len;
float massarr[4];
float mass;
HX711ADC scale(A1, A0);        // parameter "gain" is ommited; the default value 128 is used by the library

//thermistor stuff
// which analog pin to connect
#define THERMISTORPIN A2
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 9970

// Create a variable that will store the temperature value
double temperature = 0.0;

int samples[NUMSAMPLES];

//end thermistor stuff
int compare (const void * a, const void * b)//this is the compare function used to sort the array to find median value
{
  float fa = *(const float*) a;
  float fb = *(const float*) b;
  return (fa > fb) - (fa < fb);
}

float measuretemp()
{
    uint8_t i;
    float average;

    // take N samples in a row, with a slight delay
    for (i=0; i< NUMSAMPLES; i++)
    {
        samples[i] = analogRead(THERMISTORPIN);
        delay(10);
    }

    // average all the samples out
    average = 0;
    for (i=0; i< NUMSAMPLES; i++)
    {
        average += samples[i];
    }

    average /= NUMSAMPLES;

    // convert the value to resistance
    float resistance;
    resistance = SERIESRESISTOR * (4095.0 / average - 1.0);//4095 is the number of levels on the adc

    float steinhart;
    steinhart = resistance / THERMISTORNOMINAL; // (R/Ro)
    steinhart = log(steinhart); // ln(R/Ro)
    steinhart /= BCOEFFICIENT; // 1/B * ln(R/Ro)
    steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart; // Invert
    steinhart -= 273.15; // convert to C

    Particle.publish("Temperature: ", String(steinhart));
    return steinhart;
}

void setup() {
    delay(2000);

    scale.set_scale(212.f);           // Low capacity! High capacity is 81. this value is obtained by calibrating the scale with known weights. It seems similar between the tal 220 units. It is a quotient: if it goes up the response reported goes down. It assumes that the response is linear and mostly static, which is true below 3kg.
// I'm taring upon power up now.
    len = strlen(data_r);

    if (len == 0)//if this is first boot add the header and tare the scale.
    {
        Particle.connect();
        waitUntil(Particle.connected);//This is to set the clock, really. Otherwise the Photon doesn't know the time during the first hour
        Particle.publish("Wely status", "BOOT!");
        delay(100000);//This gives me time to screw it together after replacing the battery
        scale.tare();
        taretemp = measuretemp();//store the temperature at tare because I will use it to determine error later.
        tare = scale.get_offset();
        strcpy(data_r,"write_api_key="+WRITE_API_KEY+"&time_format=absolute&updates=");
    }

    scale.set_offset(tare);        //This value is obtained by taring after boot.
    ThingSpeak.begin(client);

    pinMode(A2, INPUT);//not sure if this is really necessary. some people say pinmode is only for digital pins
}

void loop() {

    float tcorrectedmass;

    for (int i = 0; i <= 4; i++){
        massarr[i]=scale.get_units(1);//this used to be 10.
        //I think the HX711 is running at 10 samples per second.
    }

    qsort(massarr, 5, sizeof(int), compare);

    mass = massarr[2];

    tcorrectedmass = mass - (3.72*(measuretemp()-taretemp)); //Wely 6. the values are from calibration. they are different for every Wely. Keep in mind the sign! It varies.

    strcat(data_r,String(Time.local())); // append absolute time stamp .
    strcat(data_r,"%2C"); //comma
    strcat(data_r,String(mass)); //append data
    strcat(data_r,"%2C"); //comma
    strcat(data_r,String(tcorrectedmass)); //append data
    strcat(data_r,"%2C"); //comma
    strcat(data_r,String(measuretemp())); //append data
    strcat(data_r,"%7C"); // | between data points

    len = strlen(data_r);
    if (len>=310)
    {
        senddata(data_r);
    }

    measuretemp();

    System.sleep(SLEEP_MODE_DEEP,600);

}

void senddata(char* Buffer) {

    Particle.connect();

    delay(10000);
    Particle.publish("data", Buffer);
    Particle.publish("data length", String(len));


    String data_length = String(strlen(Buffer));
    // Close any connection before sending a new request
    client.stop();
    // POST data to ThingSpeak
    if (client.connect(server, 80)) {
        Particle.publish("connected to Thingspeak", "");
        client.println("POST /channels/"+CHANNEL_ID+"/bulk_update HTTP/1.1");
        client.println("Host: "+server);
        client.println("User-Agent: mw.doc.bulk-update (Particle Photon)");
        client.println("Connection: close");
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.println("Content-Length: "+data_length);
        client.println(); // hard return?
        client.println(Buffer);
        Particle.publish("data sent was", Buffer);
    }

    delay(5000);

    Particle.disconnect();


    strcpy(data_r,"write_api_key="+WRITE_API_KEY+"&time_format=absolute&updates=");

}
