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
  
  MEMORIA PERSISTENTE:
    - L'ultima animazione utilizzata viene salvata nella EEPROM
    - Ad ogni accensione, la lampada riprende dall'ultima animazione
  
  HARDWARE:
  - ESP32
  - Anello NeoPixel 12 LED collegato al pin GPIO 5
  - Sensore touch TB1 collegato al pin D3
  - Sensore touch TB2 collegato al pin D4
  
  LIBRERIE NECESSARIE:
  - Adafruit_NeoPixel (installabile dal Library Manager di Arduino IDE)
  - EEPROM (inclusa nell'ambiente ESP32)
  
  ============================================================================

  Lo sketch "Lampada_Standby_Memory" non utilizza la memoria EEPROM ma la RAM della scheda e:
 
    - Le impostazioni (animazione e luminosità) vengono mantenute in RAM
    - Quando la lampada è "spenta", i LED si spengono ma l'ESP32 resta alimentato
    - All'accensione successiva, riprende dall'ultima configurazione
    - NOTA: Se togli completamente l'alimentazione, le impostazioni si resettano

    Il resto è uguale all'altro sketch
  
