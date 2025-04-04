#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

// Configuración de pines
#define LED_PIN 6
#define LED_COUNT 60
#define STOP_BUTTON_PIN 2 // Botón para detener audio
#define SPANISH_BUTTON_PIN 3 // Botón para español
#define YOKOTAN_BUTTON_PIN 4 // Botón para yokot'an

// Definición de ventanillas
#define NUM_WINDOWS 22
const int windowPins[NUM_WINDOWS] = {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
                                    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43};

// Configuración del reproductor MP3
SoftwareSerial mySoftwareSerial(10, 11);
DFRobotDFPlayerMini myDFPlayer;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

enum SystemState {
  STATE_IDLE,
  STATE_PLAYING,
  STATE_STOPPED
};

SystemState currentState = STATE_IDLE;

int audioQueue[10];
int queueSize = 0;
int currentPlaying = -1;
bool spanishLanguage = true; // true: español, false: yokot'an

const int windowGroups[][5] = {
  {0, 1, 2, 3, 4},   // Grupo 1
  {5, 6, 7, 8, 9},   // Grupo 2
  {10, 11, 12, 13},  // Grupo 3
  {14, 15, 16, 17},  // Grupo 4
  {18, 19, 20, 21}   // Grupo 5
};

const uint32_t groupColors[] = {
  strip.Color(255, 0, 0),
  strip.Color(0, 255, 0),
  strip.Color(0, 0, 255),
  strip.Color(255, 255, 0),
  strip.Color(255, 0, 255)
};

void setup() {
  Serial.begin(9600);
  mySoftwareSerial.begin(9600);
  
  if (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("Error al inicializar DFPlayer"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini listo."));
  
  myDFPlayer.volume(20);

  for (int i = 0; i < NUM_WINDOWS; i++) {
    pinMode(windowPins[i], INPUT_PULLUP);
  }

  pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SPANISH_BUTTON_PIN, INPUT_PULLUP);
  pinMode(YOKOTAN_BUTTON_PIN, INPUT_PULLUP);

  strip.begin();
  strip.show();
  updateLanguageLEDs();

  attachInterrupt(digitalPinToInterrupt(STOP_BUTTON_PIN), stopAudio, FALLING);
  attachInterrupt(digitalPinToInterrupt(SPANISH_BUTTON_PIN), setSpanish, FALLING);
  attachInterrupt(digitalPinToInterrupt(YOKOTAN_BUTTON_PIN), setYokotan, FALLING);
}

void loop() {
  checkWindows();
  manageQueue();
  
  if (currentState == STATE_PLAYING) {
    theaterChase(strip.Color(127, 127, 127), 50);
  } else {
    updateLanguageLEDs();
  }
  
  delay(50);
}

// Funciones modificadas para manejar dos botones de idioma
void setSpanish() {
  if (!spanishLanguage) {
    spanishLanguage = true;
    updateLanguageLEDs();
    if (currentState == STATE_PLAYING) {
      stopAudio();
    }
  }
}

void setYokotan() {
  if (spanishLanguage) {
    spanishLanguage = false;
    updateLanguageLEDs();
    if (currentState == STATE_PLAYING) {
      stopAudio();
    }
  }
}

// Resto de las funciones permanecen igual que en el código anterior
void checkWindows() {
  static bool lastWindowState[NUM_WINDOWS] = {false};
  
  for (int i = 0; i < NUM_WINDOWS; i++) {
    bool currentState = digitalRead(windowPins[i]) == LOW;
    
    if (currentState && !lastWindowState[i]) {
      windowOpened(i);
      highlightWindowGroup(i);
    } else if (!currentState && lastWindowState[i]) {
      windowClosed(i);
    }
    
    lastWindowState[i] = currentState;
  }
}

void windowOpened(int windowId) {
  int audioNumber = spanishLanguage ? (windowId + 1) : (windowId + 101);
  
  if (currentState == STATE_IDLE) {
    playAudio(audioNumber);
  } else {
    addToQueue(audioNumber);
  }
}

void windowClosed(int windowId) {
  int audioNumber = spanishLanguage ? (windowId + 1) : (windowId + 101);
  removeFromQueue(audioNumber);
}

void playAudio(int audioNumber) {
  myDFPlayer.play(audioNumber);
  currentPlaying = audioNumber;
  currentState = STATE_PLAYING;
}

void stopAudio() {
  myDFPlayer.stop();
  currentState = STATE_STOPPED;
  currentPlaying = -1;
  clearQueue();
  updateLanguageLEDs();
}

void addToQueue(int audioNumber) {
  if (queueSize < 10) {
    audioQueue[queueSize] = audioNumber;
    queueSize++;
  }
}

void removeFromQueue(int audioNumber) {
  for (int i = 0; i < queueSize; i++) {
    if (audioQueue[i] == audioNumber) {
      for (int j = i; j < queueSize - 1; j++) {
        audioQueue[j] = audioQueue[j + 1];
      }
      queueSize--;
      break;
    }
  }
}

void clearQueue() {
  queueSize = 0;
}

void manageQueue() {
  if (currentState == STATE_PLAYING) {
    if (myDFPlayer.available()) {
      if (myDFPlayer.readType() == DFPlayerPlayFinished) {
        if (queueSize > 0) {
          playAudio(audioQueue[0]);
          for (int i = 0; i < queueSize - 1; i++) {
            audioQueue[i] = audioQueue[i + 1];
          }
          queueSize--;
        } else {
          currentState = STATE_IDLE;
          currentPlaying = -1;
        }
      }
    }
  } else if (currentState == STATE_IDLE && queueSize > 0) {
    playAudio(audioQueue[0]);
    for (int i = 0; i < queueSize - 1; i++) {
      audioQueue[i] = audioQueue[i + 1];
    }
    queueSize--;
  }
}

void updateLanguageLEDs() {
  uint32_t color = spanishLanguage ? strip.Color(0, 0, 255) : strip.Color(255, 165, 0);
  
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void highlightWindowGroup(int windowId) {
  int group = -1;
  for (int g = 0; g < 5; g++) {
    for (int w = 0; w < 5; w++) {
      if (windowGroups[g][w] == windowId) {
        group = g;
        break;
      }
    }
    if (group != -1) break;
  }
  
  if (group != -1) {
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, groupColors[group]);
    }
    strip.show();
    delay(500);
    updateLanguageLEDs();
  }
}

void theaterChase(uint32_t color, int wait) {
  for (int q=0; q < 3; q++) {
    for (int i=0; i < strip.numPixels(); i=i+3) {
      strip.setPixelColor(i+q, color);
    }
    strip.show();
    delay(wait);
    
    for (int i=0; i < strip.numPixels(); i=i+3) {
      strip.setPixelColor(i+q, 0);
    }
  }
}
