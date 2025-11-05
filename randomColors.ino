void randomColors() {
  // Questa animazione accende i LED uno alla volta con colori casuali
  // Quando tutti i LED sono accesi, ricomincia da capo
  
  // Controlla se è il momento di aggiornare l'animazione
  // (in base al tempo trascorso dall'ultimo aggiornamento)
  if (millis() - lastAnimationUpdate >= delayval) {
    
    // Accende il LED corrente con un colore casuale
    // random(256) genera un numero casuale tra 0 e 255
    pixels.setPixelColor(
      animationStep,
      pixels.Color(random(256), random(256), random(256))
    );
    
    // Applica le modifiche
    pixels.show();
    
    // Passa al LED successivo
    animationStep++;
    
    // Se ha completato il giro dell'anello, ricomincia
    if (animationStep >= NUM_LEDS) {
      animationStep = 0;
    }
    
    // Aggiorna il timestamp
    lastAnimationUpdate = millis();
  }
}
