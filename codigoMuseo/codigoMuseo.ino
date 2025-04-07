#include <DFRobotDFPlayerMini.h> // Biblioteca para el reproductor MP3
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h> // Biblioteca para la tira LED

// Configuración de pines
#define PIN_LED 6
#define CANTIDAD_LEDS 60
#define PIN_BOTON_DETENER 2 // Botón para detener audio
#define PIN_BOTON_ESPANOL 3 // Botón para español
#define PIN_BOTON_YOKOTAN 4 // Botón para yokot'an

// Definición de ventanillas
#define NUM_VENTANAS 26
const int pinesVentanillas[NUM_VENTANAS] = {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
                                    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};

// Configuración del reproductor MP3
SoftwareSerial comunicacionSerial(10, 11); // RX, TX
DFRobotDFPlayerMini reproductorMP3;

Adafruit_NeoPixel tiraLED(CANTIDAD_LEDS, PIN_LED, NEO_GRB + NEO_KHZ800);

// Estados del sistema
enum EstadoSistema {
  ESTADO_INACTIVO,
  ESTADO_REPRODUCIENDO,
  ESTADO_DETENIDO
};

EstadoSistema estadoActual = ESTADO_INACTIVO;

// Variables para gestión de cola
int colaAudios[10]; // Numero de audios
int tamanoCola = 0;
int audioActual = -1; // no se está reproduciendo nada

// Variables de idioma
bool idiomaEspanol = true; // true: español, false: yokot'an

// Grupos de ventanillas y sus colores
const int gruposVentanillas[][6] = {
  {0, 1, 2, 3},   // Lengua Chol
  {4, 5, 6, 7, 8},   // Lengua Yoko'tan
  {9, 10, 11},  // Lengua Nahuatl
  {12, 13, 14, 15},  // Lengua Tzeltales
  {16, 17, 18, 19, 20}, // Lengua Zoque
  {21, 22, 23, 24}  // Lengua Ayapaneco
};

const uint32_t coloresGrupos[] = {
  tiraLED.Color(0, 26, 149),    // Azul fuerte para Chol
  tiraLED.Color(104, 170, 199), // Azul verdoso para Yoko'tan
  tiraLED.Color(51, 37, 159),   // Morado claro para Nahuatl
  tiraLED.Color(153, 44, 55),   // Rojo claro para Tzeltales
  tiraLED.Color(188, 151, 222), // Rosa para Zoque
  tiraLED.Color(177, 141, 115)  // Naranja claro para Ayapaneco
};

void setup() {
  Serial.begin(9600);
  comunicacionSerial.begin(115200);
  
  // Inicializar reproductor MP3
  if (!reproductorMP3.begin(comunicacionSerial)) {
    Serial.println(F("No se pudo inicializar el reproductor MP3:"));
    Serial.println(F("1. Revisa las conexiones"));
    Serial.println(F("2. Inserta la tarjeta SD"));
    while(true);
  }
  Serial.println(F("Reproductor MP3 listo."));
  
  reproductorMP3.volume(20); // Volumen ajustable

  // Pines de ventanillas como entradas
  for (int i = 0; i < NUM_VENTANAS; i++) {
    pinMode(pinesVentanillas[i], INPUT_PULLUP);
  }

  // Configurar pines de botones
  pinMode(PIN_BOTON_DETENER, INPUT_PULLUP);
  pinMode(PIN_BOTON_ESPANOL, INPUT_PULLUP);
  pinMode(PIN_BOTON_YOKOTAN, INPUT_PULLUP);

  // Inicializar tira LED
  tiraLED.begin();
  tiraLED.show(); // Inicializar todos los píxeles apagados
  actualizarLEDsIdioma();

  // Configurar interrupciones para botones
  attachInterrupt(digitalPinToInterrupt(PIN_BOTON_DETENER), detenerAudio, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BOTON_ESPANOL), establecerEspanol, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BOTON_YOKOTAN), establecerYokotan, FALLING);
}

void loop() {
  verificarVentanillas(); // Estado de ventanillas
  gestionarCola();       // Gestionar reproducción de audios en cola
  
  // Actualizar LEDs según estado
  if (estadoActual == ESTADO_REPRODUCIENDO) {
    // Efecto de LEDs durante reproducción
    efectoTeatro(tiraLED.Color(127, 127, 127), 50);
  } else {
    // Mostrar estado de idioma
    actualizarLEDsIdioma();
  }
  
  delay(50); // Pequeña pausa para evitar rebotes
}

void verificarVentanillas() {
  static bool estadoAnteriorVentanillas[NUM_VENTANAS] = {false};
  
  for (int i = 0; i < NUM_VENTANAS; i++) {
    bool estadoActual = digitalRead(pinesVentanillas[i]) == LOW; // LOW significa abierto
    
    if (estadoActual && !estadoAnteriorVentanillas[i]) {
      // Ventanilla acaba de abrirse
      ventanillaAbierta(i);
      resaltarGrupoVentanilla(i);
    } else if (!estadoActual && estadoAnteriorVentanillas[i]) {
      // Ventanilla acaba de cerrarse
      ventanillaCerrada(i);
    }
    
    estadoAnteriorVentanillas[i] = estadoActual;
  }
}

void ventanillaAbierta(int idVentanilla) {
  // Numero de audio según idioma
  int numeroAudio = idiomaEspanol ? (idVentanilla + 1) : (idVentanilla + 101);
  
  if (estadoActual == ESTADO_INACTIVO) {
    // Reproducir inmediatamente si no hay nada en cola
    reproducirAudio(numeroAudio);
  } else {
    // Agregar a la cola
    agregarACola(numeroAudio);
  }
}

void ventanillaCerrada(int idVentanilla) {
  // Determinar el número de audio según idioma
  int numeroAudio = idiomaEspanol ? (idVentanilla + 1) : (idVentanilla + 101);
  
  // Eliminar de la cola si está presente
  eliminarDeCola(numeroAudio);
}

void reproducirAudio(int numeroAudio) {
  reproductorMP3.play(numeroAudio);
  audioActual = numeroAudio;
  estadoActual = ESTADO_REPRODUCIENDO;
}

void detenerAudio() {
  reproductorMP3.stop();
  estadoActual = ESTADO_DETENIDO;
  audioActual = -1;
  limpiarCola();
  actualizarLEDsIdioma();
}

void establecerEspanol() {
  if (!idiomaEspanol) {
    idiomaEspanol = true;
    actualizarLEDsIdioma();
    
    // Si hay algo reproduciéndose, detenerlo y limpiar cola
    if (estadoActual == ESTADO_REPRODUCIENDO) {
      detenerAudio();
    }
  }
}

void establecerYokotan() {
  if (idiomaEspanol) {
    idiomaEspanol = false;
    actualizarLEDsIdioma();
    
    // Si hay algo reproduciéndose, detenerlo y limpiar cola
    if (estadoActual == ESTADO_REPRODUCIENDO) {
      detenerAudio();
    }
  }
}

void agregarACola(int numeroAudio) {
  if (tamanoCola < 10) {
    colaAudios[tamanoCola] = numeroAudio;
    tamanoCola++;
  }
}

void eliminarDeCola(int numeroAudio) {
  for (int i = 0; i < tamanoCola; i++) {
    if (colaAudios[i] == numeroAudio) {
      // Desplazar elementos restantes
      for (int j = i; j < tamanoCola - 1; j++) {
        colaAudios[j] = colaAudios[j + 1];
      }
      tamanoCola--;
      break;
    }
  }
}

void limpiarCola() {
  tamanoCola = 0;
}

void gestionarCola() {
  if (estadoActual == ESTADO_REPRODUCIENDO) {
    // Verificar si el audio actual ha terminado
    if (reproductorMP3.available()) {
      if (reproductorMP3.readType() == DFPlayerPlayFinished) {
        if (tamanoCola > 0) {
          // Reproducir siguiente en cola
          reproducirAudio(colaAudios[0]);
          // Desplazar cola
          for (int i = 0; i < tamanoCola - 1; i++) {
            colaAudios[i] = colaAudios[i + 1];
          }
          tamanoCola--;
        } else {
          // No hay más audios en cola
          estadoActual = ESTADO_INACTIVO;
          audioActual = -1;
        }
      }
    }
  } else if (estadoActual == ESTADO_INACTIVO && tamanoCola > 0) {
    // Reproducir siguiente en cola si el sistema estaba inactivo
    reproducirAudio(colaAudios[0]);
    // Desplazar cola
    for (int i = 0; i < tamanoCola - 1; i++) {
      colaAudios[i] = colaAudios[i + 1];
    }
    tamanoCola--;
  }
}

void actualizarLEDsIdioma() {
  uint32_t color = idiomaEspanol ? tiraLED.Color(0, 0, 255) : tiraLED.Color(255, 165, 0); // Azul para español, Naranja para yokot'an
  
  for (int i = 0; i < CANTIDAD_LEDS; i++) {
    tiraLED.setPixelColor(i, color);
  }
  tiraLED.show();
}

void resaltarGrupoVentanilla(int idVentanilla) {
  // Determinar a qué grupo pertenece la ventanilla
  int grupo = -1;
  for (int g = 0; g < 5; g++) {
    for (int v = 0; v < 5; v++) {
      if (gruposVentanillas[g][v] == idVentanilla) {
        grupo = g;
        break;
      }
    }
    if (grupo != -1) break;
  }
  
  if (grupo != -1) {
    // Iluminar LEDs con el color del grupo
    for (int i = 0; i < CANTIDAD_LEDS; i++) {
      tiraLED.setPixelColor(i, coloresGrupos[grupo]);
    }
    tiraLED.show();
    delay(500); // Mostrar color por medio segundo
    actualizarLEDsIdioma(); // Volver al color de idioma
  }
}

// Efecto de teatro para LEDs durante reproducción
void efectoTeatro(uint32_t color, int espera) {
  for (int q=0; q < 3; q++) {
    for (int i=0; i < tiraLED.numPixels(); i=i+3) {
      tiraLED.setPixelColor(i+q, color); // Enciende cada tercer píxel
    }
    tiraLED.show();
    delay(espera);
    
    for (int i=0; i < tiraLED.numPixels(); i=i+3) {
      tiraLED.setPixelColor(i+q, 0); // Apaga los píxeles
    }
  }
}