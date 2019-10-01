/*
This code was created by James Douglas, Phytochem Consulting, for the Wely (Weighing Lysimeter)
project. It is released under GNU GPLv3 - feel free to modify, share and use the code while keeping it open source.

When using the code to re-flash a Wely device, you may copy and paste most of it into the Particle Build Integrated Development Environment using a web browser.
The only odd part is that you have to add the libraries using the controls inside the IDE, so search for their names as noted below then delete the duplicate like that the IDE will add.

References used to write the code are,

Median filter using qsort:
https://stackoverflow.com/questions/3886446/problem-trying-to-use-the-c-qsort-function
https://community.particle.io/t/using-qsort-to-average-readings/20824

Thermistor code:
https://learn.adafruit.com/thermistor/using-a-thermistor

Bulk update for ThingSpeak:
https://www.mathworks.com/help/thingspeak/bulk-update-a-thingspeak-channel-using-particle-photon-board.html

*/

// This #include statement was automatically added by the Particle IDE.
#include <ThingSpeak.h>

// This #include statement was automatically added by the Particle IDE.
#include <HX711ADC.h>

// This #include statement was automatically added by the Particle IDE.
#include <RTClibrary.h>

#include <math.h> // this is a default C++ library, you don't have to add it through the IDE just include it.

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));//This allows the Photon to remember data during deep sleep.
SYSTEM_MODE(SEMI_AUTOMATIC);//This allows control over the WiFi connectivity.

retained char data_r[500];//Array to store data in during deep sleep
retained long tare;
retained long taretemp; //Store temperature at which the initial tare is made

//Thingspeak channel parameters.
String CHANNEL_ID = "600483";
String WRITE_API_KEY = "T0R08JF34DWWXV0P";
String server = "api.thingspeak.com"; // ThingSpeak Server
TCPClient client;

size_t len;
float massarr[4];//This array is used to collect and filter mass readings before storing them.
float mass;
HX711ADC scale(A1, A0);        // This is for the load cell amplifier. The parameter "gain" is ommitted; the default value 128 is used by the library.

//Thermistor stuff
// which analog pin to connect
#define THERMISTORPIN A2
// Specified nominal resistance of the thermistor at 25 degrees C
#define THERMISTORNOMINAL 10000
// Specified temperature for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// How many samples to take and average for each sampling.
#define NUMSAMPLES 5
// The specified beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// The value of the reference resistor.
#define SERIESRESISTOR 9970

// Create a variable that will store the temperature value
double temperature = 0.0;

// Array for storing temperature values
int samples[NUMSAMPLES];

//end thermistor stuff

//This is the compare function used to sort the array of mass readings to find median (central) value.
//A median filter is a good choice because there are, sometimes, wildly incorrect readings reported.
int compare (const void * a, const void * b)
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
    resistance = SERIESRESISTOR * (4095.0 / average - 1.0);//4095 is the number of levels on the analog to digital converter of the Photon (0 to 1V)

    //This is the core of the thermistor sensor code - the Steinhart and Hart equation.
    float steinhart;
    steinhart = resistance / THERMISTORNOMINAL; // (R/Ro)
    steinhart = log(steinhart); // ln(R/Ro)
    steinhart /= BCOEFFICIENT; // 1/B * ln(R/Ro)
    steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart; // Invert
    steinhart -= 273.15; // convert to degrees C

    Particle.publish("Temperature: ", String(steinhart));
    return steinhart;
}

void setup() {
    delay(2000);

    scale.set_scale(87.f);           // 87 is the unit conversion value for a YZC-1B 50kg capacity bar type load cell. The 10 kg TAL 220 uses more like 212. This value is obtained by calibrating the scale with known weights. It seems similar between the load cells of the same model. It is a quotient: if it goes up the response reported goes down. The HX711 library assumes that the response is linear and mostly static, which is true below about 30% of the nominal load.
    len = strlen(data_r);

    if (len == 0)//If this is first boot after total loss of power then the data_r array will be empty. Add the header and tare the scale.
    {
        Particle.connect();
        waitUntil(Particle.connected);//This is to set the clock. Otherwise the Photon doesn't know the time during the first hour.

        Particle.publish("Wely status", "BOOT!");
        delay(100000);//This gives the user time to screw the housing together after replacing the battery.

        scale.tare();
        taretemp = measuretemp();//store the temperature at tare because we will use it for temperature compensation.

        tare = scale.get_offset();//Store the tare value for later use, after deep sleep

        strcpy(data_r,"write_api_key="+WRITE_API_KEY+"&time_format=absolute&updates=");//Append the header for ThingSpeak.

    }

    scale.set_offset(tare);        //This value is obtained by taring after boot. It must be reloaded after deep sleep.
    ThingSpeak.begin(client);

    pinMode(A2, INPUT);//Not sure if this is really necessary. Some people say pinmode is only for digital pins.
}

void loop() {

    float tcorrectedmass;

    //Collect some mass readings.
    for (int i = 0; i <= 4; i++){
        massarr[i]=scale.get_units(1);//this used to be 10.
        //I think the HX711 is running at 10 samples per second.
    }

    //Sort them by size
    qsort(massarr, 5, sizeof(int), compare);

    //Choose the central value (median).
    mass = massarr[2];

    //Correct for temperature drift.
    tcorrectedmass = mass - (37.9*(measuretemp()-taretemp)); //Wely 16. The values are from calibration. They are different for every Wely. Keep in mind the sign! It varies.

    //Build array to send to ThingSpeak.
    strcat(data_r,String(Time.local())); // append absolute time stamp .
    strcat(data_r,"%2C"); //comma
    strcat(data_r,String(mass)); //append data
    strcat(data_r,"%2C"); //comma
    strcat(data_r,String(tcorrectedmass)); //append data
    strcat(data_r,"%2C"); //comma
    strcat(data_r,String(measuretemp())); //append data
    strcat(data_r,"%7C"); // | between data points


    len = strlen(data_r);

    //This tests to see if it is time to send the data to ThingSpeak, or if should more be collected.
    if (len>=310) //Each point takes about 60 characters. The length of the array is used to track the amount  of data collected.
    {
        senddata(data_r);
    }

    measuretemp();

    scale.power_down();//This puts the clock pin high to the HX711. It must remain high, otherwise the amplifier will turn on and use power. The Photon cannot control pins during deep sleep. That's why there is a pullup resistor on the clock pin.

    System.sleep(SLEEP_MODE_DEEP,600);

    scale.power_up();
}

void senddata(char* Buffer) {

    Particle.connect();

    delay(10000); // Using a Particle feature to see if the connection is complete could save time. Going too fast is a sure way to ruin the update.
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
        client.println();
        client.println(Buffer);
        Particle.publish("data_sent", Buffer);//This Particle event can be retrieved by Losant.
    }

    delay(5000); // again, it is important to not rush cloud operations, otherwise disconnection could happen before the data is transferred.

    Particle.disconnect();

    strcpy(data_r,"write_api_key="+WRITE_API_KEY+"&time_format=absolute&updates="); //This replaces the entire contents of the array with just a header.
}
