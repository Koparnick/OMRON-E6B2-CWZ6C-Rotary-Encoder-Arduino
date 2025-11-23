/*
 * Project: Omron E6B2-CWZ6C Rotary Encoder Reading System
 * Author: Gemini (User Partner)
 * Date: 23.11.2025
 * Description: 
 * This code reads an NPN Open Collector output encoder using Arduino.
 * It operates on an Interrupt-based logic and provides angle/rotation data via serial port.
 */

// --- CONSTANTS AND SETTINGS ---
const int PIN_ENCODER_A = 2;       // Phase A (Black Wire) -> Interrupt Pin
const int PIN_ENCODER_B = 3;       // Phase B (White Wire)
const int ENCODER_RESOLUTION = 1024; // Encoder P/R (Pulse per Revolution) value. Check the label and update!
const long SERIAL_BAUD_RATE = 9600; // Serial communication speed

// --- CLASS DEFINITION: RotaryEncoderMonitor ---
class RotaryEncoderMonitor {
  private:
    int _pinA;
    int _pinB;
    volatile long _encoderPosition; // Must be volatile as it changes inside an interrupt (ISR)

  public:
    /**
     * Constructor: Defines the pins used by the encoder.
     * @param pinA Pin connected to Phase A
     * @param pinB Pin connected to Phase B
     */
    RotaryEncoderMonitor(int pinA, int pinB) {
      _pinA = pinA;
      _pinB = pinB;
      _encoderPosition = 0;
    }

    /**
     * Initialization: Sets pin modes.
     * Activates internal Pull-Up resistors which are critical for NPN Open Collector outputs.
     */
    void begin() {
      pinMode(_pinA, INPUT_PULLUP); 
      pinMode(_pinB, INPUT_PULLUP);
    }

    /**
     * Core Logic: Determines direction on signal change and updates the counter.
     * This function is intended to be called by the Interrupt Service Routine (ISR).
     */
    void handleInterruptLogic() {
      // When Pin A goes HIGH, check Pin B state to determine direction
      if (digitalRead(_pinA) == HIGH) {
        if (digitalRead(_pinB) == LOW) {
          _encoderPosition++; // Clockwise (CW)
        } else {
          _encoderPosition--; // Counter-Clockwise (CCW)
        }
      } else {
        // Optional: Logic for when Pin A goes LOW (Falling Edge).
        // Included for stability when using CHANGE interrupt mode.
        if (digitalRead(_pinB) == HIGH) {
           _encoderPosition++;
        } else {
           _encoderPosition--;
        }
      }
    }

    /**
     * Returns the raw position counter safely.
     * Uses a critical section (disabling interrupts) to ensure atomic reading.
     * @return Current raw step count
     */
    long getRawPosition() {
      long position;
      noInterrupts(); // Disable interrupts to prevent data corruption during read
      position = _encoderPosition;
      interrupts();   // Re-enable interrupts
      return position;
    }

    /**
     * Calculates the total number of full rotations completed.
     * @return Number of full turns
     */
    long getFullRotations() {
      return getRawPosition() / ENCODER_RESOLUTION;
    }

    /**
     * Calculates the current angle within the specific revolution (0-360 degrees).
     * Handles negative values correctly to always return 0-360 range.
     * @return Angle in degrees (float)
     */
    float getCurrentAngle() {
      long rawPos = getRawPosition();
      long posInRevolution = rawPos % ENCODER_RESOLUTION;
      
      // Convert negative values to positive (e.g., -10 degrees becomes 350 degrees)
      if (posInRevolution < 0) {
        posInRevolution += ENCODER_RESOLUTION;
      }
      
      return (posInRevolution * 360.0) / ENCODER_RESOLUTION;
    }
};

// --- GLOBAL OBJECTS ---
RotaryEncoderMonitor motorEncoder(PIN_ENCODER_A, PIN_ENCODER_B);

// --- INTERRUPT SERVICE ROUTINE (WRAPPER) ---
// Global function wrapper because a class method cannot be directly passed to attachInterrupt.
void encoderISR() {
  motorEncoder.handleInterruptLogic();
}

// --- SETUP ---
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  
  motorEncoder.begin();

  // Trigger interrupt on every state change (CHANGE) on Pin A.
  // This captures both Rising and Falling edges for better resolution/stability.
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), encoderISR, CHANGE);

  Serial.println("--- Encoder Reading Started ---");
  Serial.print("Resolution: ");
  Serial.println(ENCODER_RESOLUTION);
}

// --- MAIN LOOP ---
void loop() {
  // Retrieve data in a human-readable format
  long totalSteps = motorEncoder.getRawPosition();
  long turns = motorEncoder.getFullRotations();
  float angle = motorEncoder.getCurrentAngle();

  // Print to Serial Monitor
  Serial.print("Step: ");
  Serial.print(totalSteps);
  Serial.print(" | Turn: ");
  Serial.print(turns);
  Serial.print(" | Angle: ");
  Serial.println(angle);

  delay(100); // Small delay to avoid flooding the Serial Monitor
}
