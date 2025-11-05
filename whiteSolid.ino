void whiteSolid() {
  // Questa funzione mantiene tutti i LED accesi con luce bianca costante
  // Usa i valori calibrati WHITE_R, WHITE_G, WHITE_B per ottenere il bianco desiderato
  
  // Imposta tutti i LED al colore bianco personalizzato
  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, pixels.Color(WHITE_R, WHITE_G, WHITE_B));
  }
  
  // Applica le modifiche ai LED
  pixels.show();
}
