#include <LiquidCrystal.h>
#include <Servo.h>

// Pines del display LCD
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
Servo servoMotor;

const int ledLCD = 1;

// Pines del motor y buzzer
const int motorPin = 9;
const int motorPin2 = 6;
const int buzzerPin = 10;

// Pines del sensor HC-SR04
const int trigPin = 7;
const int echoPin = 8;

// Variables para el temporizador
unsigned long startTime = 0;
unsigned long elapsedTime = 0;
unsigned long interval = 5000; // 5 segundos

// Variables para la programación del usuario
int targetHour = 0;
int targetMinute = 0;
int repetitions = 0;
unsigned long intervalBetweenRepetitions = 0;

// Estados del menú
enum MenuState {
  CONFIRMATION,
  HOUR_SELECTION,
  MINUTE_SELECTION,
  RUNNING
};

MenuState menuState = CONFIRMATION;
bool isPetNear = false; // Estado del sensor de distancia

bool isMotorActive = false; // Estado del motor
unsigned long motorActivationTime = 0; // Tiempo de activación del motor
const unsigned long motorActivationDuration = 5000; // Duración de activación del motor (5 segundos)

void setup() {
  pinMode(motorPin, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);

  Serial.begin(9600);

  servoMotor.attach(motorPin);
  lcd.begin(16, 2);

  lcd.setCursor(3, 0);
  lcd.print("PET-FEEDER");
  lcd.setCursor(5, 1);
  lcd.print("(0w0)");
  digitalWrite(ledLCD, HIGH);

  for (int i = 0; i < 4; i++) {
    digitalWrite(ledLCD, HIGH);
    delay(100);
    digitalWrite(ledLCD, LOW);
    delay(100);
  }
  digitalWrite(ledLCD, HIGH);

  delay(2000);

  lcd.setCursor(1, 0);
  lcd.print("Presione START");
  lcd.setCursor(5, 1);
  lcd.print("(0w0)");
}

void loop() {
  switch (menuState) {
    case CONFIRMATION:
      menuConfirmation();
      break;
    case HOUR_SELECTION:
      menuHourSelection();
      break;
    case MINUTE_SELECTION:
      menuMinuteSelection();
      break;
    case RUNNING:
      runProgram();
      break;
  }
}

void menuConfirmation() {
  const int startButtonPin = A2;

  if (digitalRead(startButtonPin) == LOW) {
    delay(100);
    lcd.clear();
    lcd.print("Hora: 00:00");
    lcd.setCursor(0, 1);
    lcd.print("Reps: 0");
    menuState = HOUR_SELECTION;
  }

  // Verificar si la distancia es menor o igual a 60 cm
  if (distanceToObstacle() <= 60) {
    isPetNear = true;
  } else {
    isPetNear = false;
  }

  // Verificar si la mascota está cerca y si el motor no está activo
  if (isPetNear && !isMotorActive) {
    activateMotor();
  } else if (!isPetNear && isMotorActive) {
    if (millis() - motorActivationTime >= motorActivationDuration) {
      deactivateMotor();
    }
  }
}

void menuHourSelection() {
  const int hourButtonPin = A0;
  const int confirmButtonPin = A1;

  if (digitalRead(hourButtonPin) == LOW) {
    delay(100);
    targetHour = (targetHour + 1) % 24;
    lcd.setCursor(6, 0);
    printDigits(targetHour);
  }

  if (digitalRead(confirmButtonPin) == LOW) {
    delay(100);
    lcd.setCursor(0, 1);
    lcd.print("Min: 00");
    menuState = MINUTE_SELECTION;
  }
}

void menuMinuteSelection() {
  const int minuteButtonPin = A0;
  const int confirmButtonPin = A1;

  if (digitalRead(minuteButtonPin) == LOW) {
    delay(100);
    targetMinute = (targetMinute + 1) % 60;
    lcd.setCursor(9, 0);
    printDigits(targetMinute);
  }

  if (digitalRead(confirmButtonPin) == LOW) {
    delay(100);
    lcd.clear();
    lcd.print("Hora: ");
    printDigits(targetHour);
    lcd.print(":");
    printDigits(targetMinute);
    lcd.setCursor(0, 1);
    lcd.print("Reps: 0");

    intervalBetweenRepetitions = (48 * 60 * 60 * 1000) / 6; // 48 horas divididas en 6 repeticiones

    unsigned long currentTime = millis();
    unsigned long targetTime = (targetHour * 60 + targetMinute) * 60 * 1000;
    unsigned long remainingTime = targetTime - (currentTime % (24 * 60 * 60 * 1000));
    startTime = millis() + remainingTime;
    elapsedTime = 0;
    repetitions = 0;

    menuState = RUNNING;
    lcd.clear();
    lcd.print("Temporizador:");
  }
}

void runProgram() {
  const int startButtonPin = A2;

  if (digitalRead(startButtonPin) == LOW) {
    delay(100);
    lcd.clear();
    lcd.print("Presione Start");
    menuState = CONFIRMATION;
  }

  // Utilizar la variable isPetNear en lugar de verificar el sensor directamente
  if (isPetNear) {
    activateMotor();
  } else {
    deactivateMotor();
  }

  if (elapsedTime >= interval) {
    lcd.clear();
    lcd.print("!COMIDA SERVIDA!");
    lcd.setCursor(5, 1);
    lcd.print("(0w0)");
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    startTime = millis();
    elapsedTime = 0;
    repetitions++;

    int servoPosition = repetitions * 50; // Calcular la posición del servo en cada repetición
    servoMotor.write(servoPosition); // Mover el servo

    if (repetitions >= 6) {
      lcd.clear();
      lcd.print("Completado!");
      menuState = CONFIRMATION;
    } else {
      lcd.clear();
      unsigned long remainingTime = intervalBetweenRepetitions - elapsedTime;
      unsigned long remainingMinutes = remainingTime / (60 * 1000);
      unsigned long remainingSeconds = (remainingTime / 1000) % 60;
      lcd.setCursor(0, 1);
      lcd.print("Temp: ");
      printDigits(remainingMinutes);
      lcd.print(":");
      printDigits(remainingSeconds);
    }
  } else {
    unsigned long remainingTime = (interval - elapsedTime) / 1000; // Convertir a segundos
    unsigned long remainingMinutes = remainingTime / 60;
    unsigned long remainingSeconds = remainingTime % 60;
    lcd.setCursor(0, 1);
    lcd.print("Temp: ");
    printDigits(remainingMinutes);
    lcd.print(":");
    printDigits(remainingSeconds);
  }

  elapsedTime = millis() - startTime;
}

float distanceToObstacle() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long pulseDuration = pulseIn(echoPin, HIGH);
  float distance = pulseDuration * 0.034 / 2; // Conversión a centímetros

  return distance;
}

void printDigits(int digits) {
  if (digits < 10) {
    lcd.print("0");
  }
  lcd.print(digits);
}

void activateMotor() {
  digitalWrite(motorPin2, HIGH); // Activar el motor
  isMotorActive = true;
  motorActivationTime = millis();
}

void deactivateMotor() {
  if (millis() - motorActivationTime >= motorActivationDuration) {
    digitalWrite(motorPin2, LOW); // Desactivar el motor
    isMotorActive = false;
  }
}
