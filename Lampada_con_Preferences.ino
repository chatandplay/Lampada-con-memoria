/*
  ============================================================================
  CONTROLLO ANELLO NEOPIXEL CON DUE SENSORI TOUCH
  ============================================================================
  
  Questo programma controlla un anello di 12 LED NeoPixel utilizzando due
  sensori touch (TB1 e TB2) collegati ad una scheda Arduino ESP32.
  
  FUNZIONALITÀ:
  
  TB1 (Pin D3) - Pulsante di accensione/spegnimento:
    - Premendo TB1 si accende o spegne completamente l'anello
    - Alla prima accensione l'anello riprende l'ultima animazione utilizzata
  
  TB2 (Pin D4) - Controllo animazioni e luminosità:
    - Pressione BREVE (< 500ms): cambia l'animazione corrente
      Sequenza: Bianco fisso → Colori casuali → Rainbow → Bianco fisso...
    
    - Pressione LUNGA (≥ 500ms): varia la luminosità in modo continuo
      * Se luminosità > 50%: diminuisce
      * Se luminosità ≤ 50%: aumenta
      * Ad ogni nuova pressione lunga: inverte la direzione
        (se stava aumentando → diminuisce, se stava diminuendo → aumenta)
  
  MEMORIA PERSISTENTE (usando Preferences):
    - L'ultima animazione utilizzata viene salvata nella memoria flash
    - L'ultima luminosità utilizzata viene salvata nella memoria flash
    - Ad ogni accensione, la lampada riprende dall'ultima configurazione
    - Preferences è più sicuro e moderno rispetto all'uso diretto di EEPROM
  
  HARDWARE:
  - ESP32
  - Anello NeoPixel 12 LED collegato al pin GPIO 5
  - Sensore touch TB1 collegato al pin D3
  - Sensore touch TB2 collegato al pin D4
  
  LIBRERIE NECESSARIE:
  - Adafruit_NeoPixel (installabile dal Library Manager di Arduino IDE)
  - Preferences (inclusa nell'ambiente ESP32)
  
  ============================================================================
*/

#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

// ============================================================================
// OGGETTO PREFERENCES
// ============================================================================

Preferences preferences;

// ============================================================================
// CONFIGURAZIONE PIN E COSTANTI
// ============================================================================

int Touch1 = D3;       // Pin per sensore touch TB1 (accensione/spegnimento)
int Touch2 = D4;       // Pin per sensore touch TB2 (animazioni e luminosità)
int DATA_PIN = 5;      // Pin GPIO 5 per il segnale dati del NeoPixel
#define NUM_LEDS 12    // Numero di LED nell'anello NeoPixel

// ============================================================================
// CALIBRAZIONE COLORE BIANCO
// ============================================================================
// I NeoPixel possono avere il bianco leggermente sbilanciato verso il rosso.
// Modifica questi valori per ottenere il bianco perfetto:
// - Valori più bassi di WHITE_R rendono il bianco meno rossastro
// - Valori più alti di WHITE_B rendono il bianco più bluastro/freddo
// - Prova diversi valori fino a trovare il bianco che preferisci!
//
// Esempi pronti:
// Bianco neutro:        200, 255, 255
// Bianco freddo:        180, 255, 255
// Bianco molto freddo:  150, 230, 255
// Bianco caldo:         220, 255, 240

#define WHITE_R 150   // Componente ROSSO del bianco (0-255)
#define WHITE_G 230   // Componente VERDE del bianco (0-255)
#define WHITE_B 255   // Componente BLU del bianco (0-255)

// ============================================================================
// INIZIALIZZAZIONE OGGETTO NEOPIXEL
// ============================================================================

// Crea l'oggetto NeoPixel con i parametri:
// - NUM_LEDS: numero di LED (12)
// - DATA_PIN: pin di controllo dati (GPIO 5)
// - NEO_GRB: ordine dei colori (Green-Red-Blue)
// - NEO_KHZ800: frequenza di comunicazione (800 KHz)
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

// ============================================================================
// VARIABILI PER LA GESTIONE DEL PULSANTE TB1 (ACCENSIONE/SPEGNIMENTO)
// ============================================================================

bool TB1_pressed = false;  // Stato attuale del pulsante TB1 (true = premuto)
bool TB1_aux = false;      // Stato precedente di TB1 (per rilevare il cambio di stato)
bool lampOn = false;       // Stato della lampada (true = accesa, false = spenta)

// ============================================================================
// VARIABILI PER LA GESTIONE DEL PULSANTE TB2 (ANIMAZIONI E LUMINOSITÀ)
// ============================================================================

bool TB2_pressed = false;           // Stato attuale del pulsante TB2
bool TB2_aux = false;               // Stato precedente di TB2 (per rilevare il cambio)
unsigned long TB2_pressTime = 0;    // Momento in cui TB2 è stato premuto (in millisecondi)
bool TB2_longPressActive = false;   // Flag per sapere se è attiva una pressione lunga

// Tempo minimo (in millisecondi) per considerare una pressione come "lunga"
// Se il pulsante viene rilasciato prima di 500ms → pressione breve (cambia animazione)
// Se il pulsante resta premuto per 500ms o più → pressione lunga (varia luminosità)
#define LONG_PRESS_TIME 500

// ============================================================================
// VARIABILI PER LA GESTIONE DELLE ANIMAZIONI
// ============================================================================

// Identificativo dell'animazione corrente:
// 0 = Bianco fisso
// 1 = Colori casuali
// 2 = Rainbow (arcobaleno)
int currentAnimation = 0;

// Numero totale di animazioni disponibili
#define NUM_ANIMATIONS 3

// ============================================================================
// VARIABILI PER LA GESTIONE DELLA LUMINOSITÀ
// ============================================================================

int brightness = 255;  // Luminosità corrente (0-255, parte alla massima)

// Direzione di variazione della luminosità durante una pressione lunga:
// false = luminosità sta diminuendo
// true = luminosità sta aumentando
bool brightnessIncreasing = false;

// Quanto cambia la luminosità ad ogni step (maggiore = più veloce)
#define BRIGHTNESS_STEP 5

// Intervallo di tempo (in millisecondi) tra un cambio di luminosità e l'altro
// Più basso = variazione più veloce
#define BRIGHTNESS_DELAY 50

// Timestamp dell'ultimo cambio di luminosità
unsigned long lastBrightnessChange = 0;

// ============================================================================
// VARIABILI PER LE ANIMAZIONI
// ============================================================================

int delayval = 100;  // Ritardo tra un LED e l'altro nell'animazione colori casuali

// Timestamp dell'ultimo aggiornamento dell'animazione
unsigned long lastAnimationUpdate = 0;

// Contatore per tenere traccia dello step corrente nell'animazione
int animationStep = 0;

// Variabile per la posizione del ciclo nell'animazione rainbow
uint16_t rainbowJ = 0;

// ============================================================================
// FUNZIONI PER LA GESTIONE DELLA MEMORIA PREFERENCES
// ============================================================================

// Salva l'animazione corrente nelle Preferences
void saveAnimation() {
  preferences.putInt("anim", currentAnimation);
  Serial.print(">>> Animazione salvata: ");
  Serial.println(currentAnimation);
}

// Carica l'ultima animazione salvata dalle Preferences
int loadAnimation() {
  // getInt() legge il valore salvato, se non esiste ritorna il valore di default (secondo parametro)
  int savedAnimation = preferences.getInt("anim", 0);
  
  // Verifica che il valore letto sia valido
  if (savedAnimation >= 0 && savedAnimation < NUM_ANIMATIONS) {
    Serial.print(">>> Animazione caricata: ");
    Serial.println(savedAnimation);
    return savedAnimation;
  } else {
    Serial.println(">>> Valore non valido, uso animazione predefinita (0)");
    return 0;
  }
}

// Salva la luminosità corrente nelle Preferences
void saveBrightness() {
  preferences.putInt("bright", brightness);
  Serial.print(">>> Luminosità salvata: ");
  Serial.print(brightness);
  Serial.print(" (");
  Serial.print((brightness * 100) / 255);
  Serial.println("%)");
}

// Carica l'ultima luminosità salvata dalle Preferences
int loadBrightness() {
  // getInt() legge il valore salvato, se non esiste ritorna il valore di default (secondo parametro)
  int savedBrightness = preferences.getInt("bright", 255);
  
  // Verifica che il valore letto sia valido (tra 10 e 255)
  if (savedBrightness >= 10 && savedBrightness <= 255) {
    Serial.print(">>> Luminosità caricata: ");
    Serial.print(savedBrightness);
    Serial.print(" (");
    Serial.print((savedBrightness * 100) / 255);
    Serial.println("%)");
    return savedBrightness;
  } else {
    Serial.println(">>> Valore non valido, uso luminosità predefinita (255)");
    return 255;
  }
}

// ============================================================================
// FUNZIONE SETUP - ESEGUITA UNA SOLA VOLTA ALL'AVVIO
// ============================================================================

void setup() {
  // Inizializza la comunicazione seriale a 115200 baud per il debug
  Serial.begin(115200);
  delay(2000);  // Attende 2 secondi per stabilizzare la comunicazione
  Serial.println("\n\nLampada_con_Preferences");
  
  // Inizializza Preferences con namespace "lampada"
  // Il secondo parametro false = modalità lettura/scrittura
  // Il namespace è come il nome di un "cassetto" dove salvare i dati
  preferences.begin("lampada", false);
  Serial.println("Preferences inizializzate");
  
  // Carica l'ultima animazione salvata
  currentAnimation = loadAnimation();
  
  // Carica l'ultima luminosità salvata
  brightness = loadBrightness();
  
  // Inizializza l'anello NeoPixel
  pixels.begin();                    // Prepara i LED per la comunicazione
  pixels.setBrightness(brightness);  // Imposta la luminosità caricata
  pixels.clear();                    // Spegne tutti i LED
  pixels.show();                     // Applica le modifiche (LED spenti)
  
  // Configura i pin come INPUT o OUTPUT
  pinMode(LED_BUILTIN, OUTPUT);  // LED integrato della scheda come output
  pinMode(Touch1, INPUT);        // Sensore touch TB1 come input
  pinMode(Touch2, INPUT);        // Sensore touch TB2 come input
  
  // Stampa messaggi di debug sulla porta seriale
  Serial.println("\n\n=== SISTEMA AVVIATO ===");
  Serial.println("Test sensori touch:");
  Serial.print("TB1 (D3): ");
  Serial.println(digitalRead(Touch1));  // Mostra lo stato iniziale di TB1
  Serial.print("TB2 (D4): ");
  Serial.println(digitalRead(Touch2));  // Mostra lo stato iniziale di TB2

  Serial.println("\n>>> PREMI TB1 PER ACCENDERE <<<");
  Serial.println("Monitoraggio stato ogni 500ms...\n");
}

// ============================================================================
// FUNZIONE LOOP - ESEGUITA CONTINUAMENTE
// ============================================================================

void loop() {
  // Gestisce il pulsante TB1 (accensione/spegnimento)
  handleTB1();
  
  // Se la lampada è accesa, gestisce TB2 ed esegue le animazioni
  if (lampOn) {
    handleTB2();           // Gestisce pulsante TB2 (animazioni e luminosità)
    runCurrentAnimation(); // Esegue l'animazione corrente
  } else {
    // Se la lampada è spenta, spegne tutti i LED
    pixels.clear();
    pixels.show();
  }
  
  // Piccolo ritardo per evitare letture troppo frequenti
  delay(10);
}

// ============================================================================
// GESTIONE PULSANTE TB1 - ACCENSIONE/SPEGNIMENTO
// ============================================================================

void handleTB1() {
  // Legge lo stato corrente del pulsante TB1
  // digitalRead ritorna HIGH quando il sensore touch rileva un tocco
  TB1_pressed = digitalRead(Touch1);
  
  // Rileva il "fronte di salita" (passaggio da non premuto a premuto)
  // Questo avviene solo una volta per ogni pressione, evitando ripetizioni
  // mentre si tiene premuto il pulsante
  if (TB1_pressed && !TB1_aux) {
    // Inverte lo stato della lampada (accesa → spenta, spenta → accesa)
    lampOn = !lampOn;
    
    if (lampOn) {
      // La lampada è stata appena accesa
      
      // L'animazione è già impostata a quella salvata nelle Preferences
      // (caricata durante il setup)
      
      // La luminosità è già impostata a quella salvata nelle Preferences
      // (caricata durante il setup)
      Serial.print("DEBUG: Luminosità ripristinata: ");
      Serial.println(brightness);
      
      pixels.setBrightness(brightness);
      Serial.print("DEBUG: getBrightness() ritorna ");
      Serial.println(pixels.getBrightness());
      
      // Accende immediatamente i LED con l'animazione salvata
      Serial.print(">>> Lampada ACCESA - Animazione: ");
      switch(currentAnimation) {
        case 0: 
          Serial.println("Bianco fisso");
          // Accende tutti i LED in bianco
          for (int i = 0; i < NUM_LEDS; i++) {
            pixels.setPixelColor(i, pixels.Color(WHITE_R, WHITE_G, WHITE_B));
          }
          pixels.show();
          break;
        case 1: 
          Serial.println("Colori casuali");
          animationStep = 0;  // Reset per far partire subito l'animazione
          break;
        case 2: 
          Serial.println("Rainbow");
          rainbowJ = 0;  // Reset per far partire subito l'animazione
          break;
      }
      
    } else {
      // La lampada è stata appena spenta
      
      // Salva la luminosità corrente nelle Preferences
      saveBrightness();
      
      Serial.println(">>> Lampada SPENTA");
    }
  }
  
  // Memorizza lo stato attuale per il prossimo ciclo
  // Questo permette di rilevare quando lo stato cambia
  TB1_aux = TB1_pressed;
}

// ============================================================================
// GESTIONE PULSANTE TB2 - ANIMAZIONI E LUMINOSITÀ
// ============================================================================

void handleTB2() {
  // Legge lo stato corrente del pulsante TB2
  TB2_pressed = digitalRead(Touch2);
  
  // -------------------------------------------------------------------------
  // FRONTE DI SALITA - Inizio pressione
  // -------------------------------------------------------------------------
  if (TB2_pressed && !TB2_aux) {
    // Il pulsante è appena stato premuto
    
    // Memorizza il momento esatto della pressione (in millisecondi dall'avvio)
    TB2_pressTime = millis();
    
    // Reset del flag di pressione lunga
    TB2_longPressActive = false;
    
    Serial.println("TB2 premuto");
  }
  
  // -------------------------------------------------------------------------
  // PULSANTE TENUTO PREMUTO
  // -------------------------------------------------------------------------
  if (TB2_pressed && TB2_aux) {
    // Il pulsante è ancora premuto (non è stato rilasciato)
    
    // Calcola da quanto tempo il pulsante è premuto
    unsigned long pressDuration = millis() - TB2_pressTime;
    
    // Controlla se è passato abbastanza tempo per una "pressione lunga"
    if (pressDuration >= LONG_PRESS_TIME && !TB2_longPressActive && lampOn) {
      // Siamo in modalità pressione lunga!
      
      // Attiva il flag per evitare di ri-entrare in questo blocco
      TB2_longPressActive = true;
      
      // Determina la direzione iniziale della variazione di luminosità
      // in base al valore corrente:
      if (brightness > 127) {
        // Se luminosità è maggiore del 50% (127/255) → diminuisce
        brightnessIncreasing = false;
      } else {
        // Se luminosità è al 50% o meno → aumenta
        brightnessIncreasing = true;
      }
      
      // Stampa la direzione iniziale
      Serial.print("Pressione lunga - Luminosità ");
      Serial.println(brightnessIncreasing ? "AUMENTA" : "DIMINUISCE");
    }
    
    // Continua a variare la luminosità durante tutta la pressione lunga
    if (TB2_longPressActive && lampOn) {
      // Controlla se è passato abbastanza tempo dall'ultimo cambio
      // (per evitare di cambiare troppo velocemente)
      if (millis() - lastBrightnessChange >= BRIGHTNESS_DELAY) {
        
        if (brightnessIncreasing) {
          // Aumenta la luminosità
          brightness += BRIGHTNESS_STEP;
          if (brightness > 255) brightness = 255;  // Limita al massimo
        } else {
          // Diminuisce la luminosità
          brightness -= BRIGHTNESS_STEP;
          if (brightness < 10) brightness = 10;  // Limita al minimo (non spegne)
        }
        
        // Applica la nuova luminosità all'anello
        pixels.setBrightness(brightness);
        
        // Aggiorna il timestamp per il prossimo cambio
        lastBrightnessChange = millis();
        
        // Stampa il valore corrente di luminosità
        Serial.print("Luminosità: ");
        Serial.print(brightness);
        Serial.print(" (");
        Serial.print((brightness * 100) / 255);  // Mostra anche in percentuale
        Serial.println("%)");
      }
    }
  }
  
  // -------------------------------------------------------------------------
  // FRONTE DI DISCESA - Fine pressione (pulsante rilasciato)
  // -------------------------------------------------------------------------
  if (!TB2_pressed && TB2_aux) {
    // Il pulsante è appena stato rilasciato
    
    // Calcola quanto tempo è stato tenuto premuto
    unsigned long pressDuration = millis() - TB2_pressTime;
    
    // Determina se è stata una pressione BREVE o LUNGA
    if (pressDuration < LONG_PRESS_TIME && lampOn) {
      // -----------------------------------------------------------------------
      // PRESSIONE BREVE: Cambia animazione
      // -----------------------------------------------------------------------
      
      // Passa alla prossima animazione (con ritorno ciclico: 0→1→2→0→...)
      currentAnimation = (currentAnimation + 1) % NUM_ANIMATIONS;
      
      // Salva la nuova animazione nelle Preferences
      saveAnimation();
      
      // Reset delle variabili di animazione
      animationStep = 0;  // Reset contatore per animazione colori casuali
      rainbowJ = 0;       // Reset contatore per animazione rainbow
      
      // Stampa quale animazione è stata attivata
      Serial.print(">>> Cambio animazione: ");
      switch(currentAnimation) {
        case 0: Serial.println("Bianco fisso"); break;
        case 1: Serial.println("Colori casuali"); break;
        case 2: Serial.println("Rainbow"); break;
      }
    }
    
    // Se era una pressione lunga, inverte la direzione per la prossima volta
    if (TB2_longPressActive) {
      // Inverte la direzione: se stava aumentando → diminuirà, e viceversa
      brightnessIncreasing = !brightnessIncreasing;
      
      Serial.print("Prossima direzione: ");
      Serial.println(brightnessIncreasing ? "AUMENTA" : "DIMINUISCE");
    }
    
    // Reset del flag di pressione lunga
    TB2_longPressActive = false;
  }
  
  // Memorizza lo stato attuale per il prossimo ciclo
  TB2_aux = TB2_pressed;
}

// ============================================================================
// ESECUZIONE DELL'ANIMAZIONE CORRENTE
// ============================================================================

void runCurrentAnimation() {
  // Esegue l'animazione corrispondente al valore di currentAnimation
  switch (currentAnimation) {
    case 0:
      whiteSolid();      // Bianco fisso
      break;
    case 1:
      randomColors();    // Colori casuali
      break;
    case 2:
      rainbowCycle();    // Rainbow (arcobaleno)
      break;
  }
}

// ============================================================================
// FUNZIONE AUSILIARIA: WHEEL
// ============================================================================
// Converte un valore da 0 a 255 in un colore dell'arcobaleno

uint32_t Wheel(byte WheelPos) {
  // Questa funzione genera colori dell'arcobaleno
  // Input: un valore da 0 a 255
  // Output: un colore che varia da rosso → verde → blu → rosso
  
  WheelPos = 255 - WheelPos;
  
  if (WheelPos < 85) {
    // Prima parte dello spettro: da rosso a verde
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  
  if (WheelPos < 170) {
    // Seconda parte: da verde a blu
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  
  // Terza parte: da blu a rosso
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
