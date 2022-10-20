CAHUANEXT VER3.0 LW1EHP (HERNAN ROBERTO PORRINI)
	--------------------------------------------------------------------
							OLAVARRIA 12/09/2022
     	--------------------------------------------------------------------
	ESTE DISEÑO ESTA CONSTRUIDO EN BASE AL TRABAJO ORIGINAL DE: @REYNICO   
	LINK DEL PROYECTO: https://github.com/reynico/arduino-dds
	DESCONOZCO CUAL ES LA FUENTE DEL TRABAJO ANTERIOR.


	SOFTWARE DE CONTROL PARA UN DDS EN CAHUANE FR-300 DE 3 CANALES
	BANDAS DE OPERACION: 40-30-20 METROS
	MODOS: USB(bls) y LSB(bli)
	PASOS: 10KHz, 1KHz, 100Hz Y 10 Hz
	INDICACION DE RX-METER (señal AGC) 
	INDICACION DE TX-METER (señal MOD) 
	MODULO DDS: AD9850
	MICROCONTROLADOR: ATMEGA328P
	DISPLAY: LCD16X02 CONEXION PARALELO
        
	COMPENSACION DE FRECUENCIA, EXISTE UN ERROR ENTRE LA FRECUENCIA DESEADA(DISPLAY) Y LA GENERADA.
	SUPER IMPORTANTE BUSCAR LA COMPENSACION EN LA LINEA 254 PARA LSB Y LA LINEA 258 PARA USB.

  */
// https://github.com/arduino-libraries/LiquidCrystal
#include "lib/LiquidCrystal.h"
#include "lib/LiquidCrystal.cpp"

// https://github.com/thijse/Arduino-EEPROMEx
#include "lib/EEPROMex.h"
#include "lib/EEPROMex.cpp"

// https://github.com/buxtronix/arduino/tree/master/libraries/Rotary
#include "lib/Rotary.h"
#include "lib/Rotary.cpp"

// https://github.com/marcobrianza/ClickButton
#include "lib/ClickButton.h"
#include "lib/ClickButton.cpp"

// Pantalla LCD
LiquidCrystal lcd(9, 10, 11, 12, 13, A0);

// Libreria ClickButton
int step_button_status = 0;
ClickButton step_button(4, LOW, CLICKBTN_PULLUP);

#include "common.h"
#include "ad9850.h"
#include "smeter.h"
#include "rotary.h"

void setup() {
  lcd.begin(16, 2);

  /* 
     Creamos los caracteres especiales
     para mostrar en el S-meter.
  */
  byte fillbar[8] = {
    B00000,
    B01000,
    B01100,
    B01010,
    B01001,
    B01010,
    B01100,
    B01000
  };
  byte mark[8] = {
    B00000,
    B01010,
    B10001,
    B10101,
    B10001,
    B01010,
    B00000,
    B00000
  };
  lcd.createChar(0, fillbar);
  lcd.createChar(1, mark);

  /*
     Inicializamos un timer para el encoder rotativo
     de esta manera el encoder no bloquea el flujo
     normal de loop();
  */
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();

  /*
     Configuramos 4 salidas para controlar el AD9850.
  */
  pinMode(FQ_UD, OUTPUT);
  pinMode(W_CLK, OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(RESET, OUTPUT);
  pulse_high(RESET);
  pulse_high(W_CLK);
  pulse_high(FQ_UD);

  /*
     Configuramos tres entradas con pull-up para
     el cambio de banda. Como estamos utilizando
     una única llave para las 3 bandas, utilizamos
     3 pines diferentes de manera digital.
  */
  pinMode(A3, INPUT_PULLUP); // banda 40m
  pinMode(A4, INPUT_PULLUP); // banda 30m
  pinMode(A5, INPUT_PULLUP); // banda 20m

  /*
     El código contempla el uso de la memoria EEPROM del
     Arduino. De esta manera podemos almacenar la última
     frecuencia utilizada y recuperarla al encender el
     equipo.
  */
  if (EEPROM.read(0) == true) {
    Sprintln("Leyendo memoria EEPROM...");
    initial_band = EEPROM.read(1);
    initial_rx = EEPROM.readLong(2);
  } else {
    Sprintln("La memoria EEPROM está vacía. Inicializando con valores por defecto...");
  }
  for (int j = 0; j < 8; j++)
    lcd.createChar(j, block[j]);
  lcd.setCursor(0, 0);
  lcd.print("---CAHUA_NEXT---");
  lcd.setCursor(0, 1);
  lcd.print("Ver.:2.00 LW1EHP");
  delay (5000);                // SALUDO INICIAL POR 5 SEGUNDOS
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  if_freq = 2000000;           // FRECUENCIA DEL FILTRO fi
  add_if = false;              
  mode= "LSB";
    //
  //mode = vfo_mode(initial_band);
  lcd.setCursor(0, 0);
  lcd.print(mode);
  increment=1000;
}

void loop() {
  /* El cambio de banda se realiza con una llave
      giratoria que pone a masa una de las tres
      entradas por vez.
  */
  if (digitalRead(A3) == LOW && band != 40) {
    band = 40;
    add_if = false;
    lcd.setCursor(0, 0);
    lcd.print("LSB");
    mode= "LSB";
    //mode = vfo_mode(band);
    /*if (band == initial_band) {
      rx = initial_rx;
    } else {
      rx =7100000;
    }
    */
    rx =7100000;
  }
  if (digitalRead(A4) == LOW && band != 30) {
    band = 30;
    add_if = false;
    lcd.setCursor(0, 0);
    lcd.print("LSB");
    mode= "LSB";
    //mode = vfo_mode(band);
    /*if (band == initial_band) {
      rx = initial_rx;
    } else {
      rx = 10100000;
    }
    */
    rx =10100000;
  }
  if (digitalRead(A5) == LOW && band != 20) {
    band = 20;
    add_if = false;
    lcd.setCursor(0, 0);
    lcd.print("LSB");
    mode= "LSB";
    //mode = vfo_mode(band);
    /*if (band == initial_band) {
      rx = initial_rx;
    } else {
      rx = 14100000;
    }
    */
    rx =14100000;
  }

  /*
     Si la frecuencia mostrada es diferente a la frecuencia que
     obtengo al tocar el encoder, la actualizo tanto en la
     pantalla como en el DDS AD9850.
  */
  if (rx != rx2) {
    show_freq();
    send_frequency(rx);
    rx2 = rx;
  }

  /*
     step_button es el botón del encoder rotativo. Con un click
     corto cicla entre los diferentes saltos de incremento. Con
     dos clicks cortos cicla entre los diferentes saltos de
     decremento. Con un click largo cambia entre los modos USB y LSB  
  */
  step_button.Update();
  if (step_button.clicks != 0) step_button_status = step_button.clicks;
  if (step_button.clicks == 1) {
    set_increment();
  }
  if (step_button.clicks == 2) {
    set_decrement();
  }
  if (step_button.clicks == -1) {
    if (mode == "USB") { 
      mode = "LSB";
      if_freq = 2000000;
      add_if = false;
    }
    else if (mode == "LSB") {
      mode = "USB";
      if_freq = 2000000;
      add_if = true;
    }
    lcd.setCursor(0, 0);
    lcd.print(mode);
  }

  /*
     Luego de sintonizar una frecuencia, si pasaron mas de dos
     segundos la misma se almacena en la memoria EEPROM.
  */
  if (!mem_saved) {
    if (time_passed + 2000 < millis()) {
      store_memory();
    }
  }

  /*
     send_Frequency envía la señal al AD9850.
  */
  //send_frequency(rx);
  if(mode == "LSB")
  {
    send_frequency(rx+180);                     //COMPENSACION DE FRECUENCIA, EXISTE UN ERROR ENTRE LA FRECUENCIA DESEADA(DISPLAY) Y LA GENERADA
  }
  else   if(mode == "USB")
  {
    send_frequency(rx+320);
  }
  /*
     El S-meter funciona tanto para transmisión (modulación)
     como para recepción (RF). La forma de cambio es sensando
     el nivel de señal del AGC (A1), que es la entrada de señal de RF.
     Si la señal de RF del AGC en A1 es menor a 90 entonces estamos en
     transmisión y en el display mostramos la intensidad de la entrada A2 en el S-meter.
     Caso contrario se mostrará la señal de recepción.
  */
  if (millis() < lastT)
    return;
  lastT += T_REFRESH;
  if (analogRead(A1) < 90) {
    smeter_signal = map(sqrt(analogRead(A2) * 16), 0, 128, 0, 80);
    smeter(1, smeter_signal);
    
  } else {
    smeter_signal = map(sqrt(analogRead(A1) * 16), 40, 128, 0, 100);
    smeter(1, smeter_signal);
    
    
  }
  if (analogRead(A2) > 1) 
    {
      lcd.setCursor(4, 0);
      lcd.print("TX");
    }
    else
    {
      lcd.setCursor(4, 0);
      lcd.print("  ");
    }

}

/*
   El incremento de frecuencia se hace de manera
   cíclica por los valores definidos abajo. Cada
   vez que se llama a la función set_increment()
   ésta prepara automáticamente el próximo valor.
*/
void set_increment() {
  switch (increment) {
    case 1:
      increment = 10;
      hertz = "  10 Hz";
      hertz_position = 11;
      break;
    case 10:
      increment = 100;
      hertz = " 100 Hz";
      hertz_position = 10;
      break;
    case 100:
      increment = 1000;
      hertz = "  1 kHz";
      hertz_position = 11;
      break;
    case 1000:
      increment = 10000;
      hertz = " 10 kHz";
      hertz_position = 10;
      break;
  
    
  }

  lcd.setCursor(3, 0);
  lcd.print("             ");
  lcd.setCursor(6, 0);
  lcd.print(hertz);
  delay(1000);
  show_freq();
}

void set_decrement() {
  switch (increment) {
    
    case 100000:
      increment = 10000;
      hertz = " 10 kHz";
      hertz_position = 11;
      break;
    case 10000:
      increment = 1000;
      hertz = "  1 kHz";
      hertz_position = 10;
      break;
    case 1000:
      increment = 100;
      hertz = " 100 Hz";
      hertz_position = 9;
      break;
    case 100:
      increment = 10;
      hertz = "  10 Hz";
      hertz_position = 11;
      break;
    case 10:
      increment = 1;
      hertz = "   1 Hz";
      hertz_position = 11;
      break;
    
  }

  lcd.setCursor(3, 0);
  lcd.print("             ");
  lcd.setCursor(6, 0);
  lcd.print(hertz);
  delay(1000);
  show_freq();
}

/*
   Ésta función se encarga de mostrar la frecuencia
   en pantalla.
*/
void show_freq() {
  lcd.setCursor(3, 0);
  lcd.print("             ");
  if (rx < 10000000) {
    lcd.setCursor(7, 0);
  } else {
    lcd.setCursor(6, 0);
  }
  lcd.print(rx / 1000000);
  lcd.print(".");
  lcd.print((rx / 100000) % 10);
  lcd.print((rx / 10000) % 10);
  lcd.print((rx / 1000) % 10);
  lcd.print(".");
  lcd.print((rx / 100) % 10);
  lcd.print((rx / 10) % 10);
  lcd.print((rx / 1) % 10);


  time_passed = millis();
  mem_saved = false; // Triggerea el guardado en memoria
}

/*
   store_memory() guarda la frecuencia elegida en la memoria
   EEPROM cada vez que se llama.
*/
void store_memory() {
  EEPROM.write(0, true);
  EEPROM.write(1, band);
  EEPROM.writeLong(2, rx);
  mem_saved = true;
 
}

/*
   vfo_mode devuelve el modo en el que se configuró
   el DDS, dependiendo de la banda elegida.

String vfo_mode (int initial_band) {
  if_freq = 2000000;
  lcd.setCursor(0, 0);
  switch (initial_band) {
    case 40:
      add_if = false;
      lcd.print("LSB");
      return "LSB";
      break;
    case 30:
      add_if = false;
      lcd.print("LSB");
      return "LSB";
      break;
    case 20:
      add_if = true;
      lcd.print("LSB");
      return "LSB";
      break;
   
  }
}
*/
