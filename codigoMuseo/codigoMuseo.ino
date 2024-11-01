#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>

// Configuramos el módulo DFPlayer Mini
SoftwareSerial mySerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// Variables de control
const int num_ventanas = 26;          // Número de ventanas
int idiomaSeleccionado = 0;           // 0: idioma 1, 1: idioma 2
bool ventanasAbiertas[num_ventanas];  // Estado de cada ventana
bool audioEnReproduccion[num_ventanas];

// Pines
int pines_ventanas[num_ventanas] = {2, 3, 4, /*... hasta 27*/};
int botonIdioma = 28;
int botonApagado = 29;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  // Iniciar el DFPlayer
  if (!myDFPlayer.begin(mySerial)) {
    Serial.println("Error iniciando DFPlayer");
    while (true);
  }
  myDFPlayer.setTimeOut(500);
  myDFPlayer.volume(30); // Ajusta el volumen entre 0-30

  // Configuración de pines
  pinMode(botonIdioma, INPUT_PULLUP);
  pinMode(botonApagado, INPUT_PULLUP);
  for (int i = 0; i < num_ventanas; i++) {
    pinMode(pines_ventanas[i], INPUT_PULLUP);
    ventanasAbiertas[i] = false;
    audioEnReproduccion[i] = false;
  }
}

void loop() {
  // Leer selección de idioma
  if (digitalRead(botonIdioma) == LOW) {
    idiomaSeleccionado = 1 - idiomaSeleccionado;  // Cambia entre 0 y 1
    delay(200); // Anti-rebote
  }

  // Leer el estado de cada ventana
  for (int i = 0; i < num_ventanas; i++) {
    bool ventanaAbierta = digitalRead(pines_ventanas[i]) == LOW;
    
    if (ventanaAbierta && !ventanasAbiertas[i]) {
      // La ventana se acaba de abrir
      ventanasAbiertas[i] = true;
      reproducirAudio(i);
    } else if (!ventanaAbierta && ventanasAbiertas[i]) {
      // La ventana se acaba de cerrar
      ventanasAbiertas[i] = false;
      detenerAudio(i);
    }
  }

  // Verificar el botón de apagado
  if (digitalRead(botonApagado) == LOW) {
    reiniciarSistema();
    delay(200); // Anti-rebote
  }
}

void reproducirAudio(int ventana) {
  int pista = (idiomaSeleccionado * num_ventanas) + ventana + 1; // Asignar pista según idioma y ventana
  myDFPlayer.play(pista);
  audioEnReproduccion[ventana] = true;
}

void detenerAudio(int ventana) {
  myDFPlayer.stop();
  audioEnReproduccion[ventana] = false;
}

void reiniciarSistema() {
  for (int i = 0; i < num_ventanas; i++) {
    if (audioEnReproduccion[i]) {
      detenerAudio(i);
    }
  }
  idiomaSeleccionado = 0;  // Reiniciar idioma a la opción predeterminada
}
