void rainbowCycle() {
  // Questa animazione crea un effetto arcobaleno che ruota continuamente
  // attorno all'anello
  
  // Controlla se è il momento di aggiornare l'animazione (20ms tra un frame e l'altro)
  if (millis() - lastAnimationUpdate >= 20) {
    
    // Per ogni LED dell'anello
    for (int i = 0; i < NUM_LEDS; i++) {
      // Calcola il colore in base alla posizione del LED e allo step corrente
      // La formula crea un arcobaleno che si distribuisce sull'anello e ruota
      pixels.setPixelColor(i, Wheel(((i * 256 / NUM_LEDS) + rainbowJ) & 255));
    }
    
    // Applica le modifiche
    pixels.show();
    
    // Incrementa il contatore per far ruotare l'arcobaleno
    rainbowJ++;
    
    // Reset del contatore quando raggiunge 256*5 (5 giri completi)
    if (rainbowJ >= 256 * 5) {
      rainbowJ = 0;
    }
    
    // Aggiorna il timestamp
    lastAnimationUpdate = millis();
  }
}
