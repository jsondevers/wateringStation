
const int DELAY_SECONDS = 1; 
const int AIR_VALUE = 1000;  
const int WATER_VALUE = 200; 
const int INTERVAL = 200;    
const int DEBUG = 2;         
                             
void setup() {
  Serial.begin(9600);
}


void get_moisture_level(int moisture_val, String &moisture_level) {
  switch (moisture_val) {
    
    case (WATER_VALUE - 100) ... WATER_VALUE:
      moisture_level = "Humidity: 100%";
      break;
    case (WATER_VALUE + 1) ... (WATER_VALUE + INTERVAL):
      moisture_level = "Humidity: >75%";
      break;
    case (WATER_VALUE + INTERVAL + 1) ... (WATER_VALUE + 2 * INTERVAL):
      moisture_level = "Humidity: >50%";
      break;
    case (WATER_VALUE + 2 * INTERVAL + 1) ... (WATER_VALUE + 3 * INTERVAL):
      moisture_level = "Humidity: >25%";
      break;
    case (WATER_VALUE + 3 * INTERVAL + 1) ... AIR_VALUE:
      moisture_level = "Humidity: >0%";
      break;
    default: 
      moisture_level = "Humidity: 0%";
      break;
  }
}


int moisture_percentage(int moisture_val) {
  int moisture_range = AIR_VALUE - WATER_VALUE;
  return ((moisture_val - WATER_VALUE) * 100) / moisture_range;
}


void get_moisture_percentage(int moisture_val, String &moisture_level) {
  int moisture_pcent_val = moisture_percentage(moisture_val);
  moisture_level = "Humididty: " + String(moisture_pcent_val) + "%";
}

/**
 * print_sensor_value() --> Print the exact sensor value. Takes a single
 * int as input.
 */
void print_sensor_value(int moisture_val) {
  Serial.print("Moisture Sensor Value:\t"); Serial.println(moisture_val);
}


 */
void loop() {
  int moisture_val;
  moisture_val = analogRead(0); 

  print_sensor_value(moisture_val);
  if (DEBUG == 1) {
    String moisture_level;
    get_moisture_level(moisture_val, moisture_level);
    Serial.println(moisture_level);
  } else if (DEBUG > 1) {
    String moisture_level;       
    get_moisture_percentage(moisture_val, moisture_level);
    Serial.println(moisture_level);
  }

  delay(DELAY_SECONDS * 1000);
}