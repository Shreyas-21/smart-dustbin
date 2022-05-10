#include <Servo.h>

const int CONTAINER_OPEN_ANGLE = 90;
const int CONTAINER_CLOSE_ANGLE = 0;

const int LID_OPEN_ANGLE = 90;
const int LID_CLOSE_ANGLE = 0;

const int PLATFORM_TILT_ANGLE = 50;
const int PLATFORM_PAN_ANGLE_METAL = 0;
const int PLATFORM_PAN_ANGLE_WET = 60;
const int PLATFORM_PAN_ANGLE_DRY = 120;

enum WASTE_TYPE { METAL, DRY, WET };

const int MOISTURE_SENSOR_WET_THRESHOLD = 500;

const int moisture_sensor = A0;
const int ir_sensor_metal = 12;
const int ir_sensor_wet = 8;
const int ir_sensor_dry = 7;
const int us_t_pin = 2;
const int us_e_pin = 4;
const int metal_servo = 11;
const int dry_servo = 10;
const int wet_servo = 9;
const int lid_servo = 6;
const int pan_servo = 5;
const int tilt_servo = 3;
const int metal_detector = 13;

class Container {
  int ir_sensor;
  int servo_pin;
  String id;
  Servo servo;

  public:
    Container() {
      ir_sensor = 0;
      servo_pin = 0;
      id = "";
    }
    
    Container(const String& container_name, const int& irs, const int& s) {
      pinMode(ir_sensor, INPUT);
      ir_sensor = irs;
      servo_pin = s;
      id = container_name;
      servo.attach(servo_pin);
    }

    bool is_full() {
      return (digitalRead(ir_sensor) == LOW);
    }

    bool open_lid() {
      if (is_full()) {
        Serial.println(id + " is full");
        return false;
      } else {
        servo.write(CONTAINER_OPEN_ANGLE);
        delay(5000);
        servo.write(CONTAINER_CLOSE_ANGLE);
        return true;
      }
    }
};

class Lid {
  int ultrasonic_sensor_trigger;
  int ultrasonic_sensor_echo;
  int moisture_sensor;
  int servo_pin;
  Servo servo;

  public:
    Lid() {
      ultrasonic_sensor_trigger = 0;
      ultrasonic_sensor_echo = 0;
      moisture_sensor = 0;
      servo_pin = 0;
    }
    
    Lid(const int& us_t, const int& us_e, const int& m, const int& s) {
      pinMode(m, INPUT);
      pinMode(us_e, INPUT);
      ultrasonic_sensor_trigger = us_t;
      ultrasonic_sensor_echo = us_e;
      moisture_sensor = m;
      servo_pin = s;
      servo.attach(servo_pin);
    }
  
    int get_load_distance_in_cm() {
      digitalWrite(ultrasonic_sensor_trigger, LOW);
      delayMicroseconds(2);
      digitalWrite(ultrasonic_sensor_trigger, HIGH);
      delayMicroseconds(10);
      digitalWrite(ultrasonic_sensor_trigger, LOW);
      return pulseIn(ultrasonic_sensor_echo, HIGH) * 0.01732;
    }
  
    bool is_open() {
      return (servo.read() == LID_OPEN_ANGLE);
    }
  
    void open_lid() {
      servo.write(LID_OPEN_ANGLE);
      delay(20);
    }
    
    void close_lid() {
      servo.write(LID_CLOSE_ANGLE);
      delay(20);
    }
  
    void adjust_moisture_sensor() {
      int distance = get_load_distance_in_cm();
      // adjust moisture_sensor so that it touches the load
      // using the distance calculated above
    }
    
    WASTE_TYPE get_waste_type() {
      if (is_open())
        close_lid();
      adjust_moisture_sensor();
      if (analogRead(moisture_sensor) < MOISTURE_SENSOR_WET_THRESHOLD)
        return WET;
      else
        return DRY;
    }
};

class Platform {
  int pan_servo_pin;
  int tilt_servo_pin;
  int metal_detector_pin;
  Servo pan_servo;
  Servo tilt_servo;

  public:
    Platform() {
      pan_servo_pin = 0;
      tilt_servo_pin = 0;
      metal_detector_pin = 0;
    }
    
    Platform(const int& ps, const int& ts, const int& m) {
      pinMode(ps, OUTPUT);
      pinMode(ts, OUTPUT);
      pinMode(m, INPUT);

      pan_servo_pin = ps;
      tilt_servo_pin = ts;
      metal_detector_pin = m;

      pan_servo.attach(pan_servo_pin);
      tilt_servo.attach(tilt_servo_pin);
    }

    WASTE_TYPE get_waste_type() {
      if (digitalRead(metal_detector_pin) == HIGH)
        return METAL;
      else
        return DRY;
    }

    void pan_and_tilt(WASTE_TYPE w) {
      if (w == METAL) {
        pan_servo.write(PLATFORM_PAN_ANGLE_METAL);
        delay(15);
      } else if (w == DRY) {
        pan_servo.write(PLATFORM_PAN_ANGLE_DRY);
        delay(15);
      } else {
        pan_servo.write(PLATFORM_PAN_ANGLE_WET);
        delay(15);
      }
      tilt_servo.write(PLATFORM_TILT_ANGLE);
      delay(15);
    }
};

class SmartBin {
  Container metal_container;
  Container dry_container;
  Container wet_container;

  Lid lid;

  Platform platform;

  public:
    SmartBin() {
      metal_container = Container();
      dry_container = Container();
      wet_container = Container();

      lid = Lid();

      platform = Platform();
    }
    
    SmartBin(const int& metal_ir_pin, const int& metal_servo_pin,
            const int& dry_ir_pin, const int& dry_servo_pin,
            const int& wet_ir_pin, const int& wet_servo_pin,
            const int& us_t_pin, const int& us_e_pin, const int& moisture_pin,
            const int& lid_servo_pin, const int& ps_pin, const int& ts_pin,
            const int& metal_detector_pin) {
    metal_container = Container("METAL CONTAINER", metal_ir_pin, metal_servo_pin);
    dry_container = Container("DRY CONTAINER", dry_ir_pin, dry_servo_pin);
    wet_container = Container("WET CONTAINER", wet_ir_pin, wet_servo_pin);

    lid = Lid(us_t_pin, us_e_pin, moisture_pin, lid_servo_pin);

    platform = Platform(ps_pin, ts_pin, metal_detector_pin);
  }

  bool is_open() {
    return lid.is_open();
  }

  void open_bin() {
    lid.open_lid();
  }

  void close_bin() {
    lid.close_lid();
  }

  void handle_waste() {
    if (!is_open())
      open_bin();

    Serial.println("Empty waste into the bin");
    if (lid.get_waste_type() == WET) {
      if (wet_container.is_full()) {
        Serial.println("Wet Container is full");
        lid.open_lid();
        Serial.println("Remove the waste");
      } else {
        platform.pan_and_tilt(WET);
        wet_container.open_lid();
      }
    } else if (platform.get_waste_type() == METAL) {
      if (metal_container.is_full()) {
        Serial.println("Metal Container is full");
        lid.open_lid();
        Serial.println("Remove the waste");
      } else {
        platform.pan_and_tilt(METAL);
        metal_container.open_lid();
      }
    } else {
      if (dry_container.is_full()) {
        Serial.println("Dry Container is full");
        lid.open_lid();
        Serial.println("Remove the waste");
      } else {
        platform.pan_and_tilt(DRY);
        dry_container.open_lid();
      }
    }
  }
};

SmartBin mySmartBin;

void setup() {
  mySmartBin = SmartBin(ir_sensor_metal, metal_servo, ir_sensor_dry,
                        dry_servo, ir_sensor_wet, wet_servo,
                        us_t_pin, us_e_pin, moisture_sensor, lid_servo,
                        pan_servo, tilt_servo, metal_detector);
}

void loop() {
  mySmartBin.handle_waste();
  delay(10000);
}
