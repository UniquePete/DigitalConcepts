uint8_t singleUint8;
char  singleChar;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);     // Pro Mini FTDI Adaptor can't handle high baud rates
  Serial.print(F("[setup] "));
  Serial.println(F(" Character print test utility"));

  singleUint8 = 0x41;
  singleChar = 'A';
  Serial.print("[loop] uint8_t 'raw' : ");
  Serial.println( singleUint8 );
  Serial.print("[loop] uint8_t 'char' : ");
  Serial.println( (char)singleUint8 );
  
  Serial.print("[loop] Char 'raw' : ");
  Serial.println( singleChar );
  Serial.print("[loop] Char 'int' : ");
  Serial.println( (int)singleChar );
  Serial.print("[loop] Char 'uint8_t' : ");
  Serial.println( (uint8_t)singleChar );
}

void loop() {
  // put your main code here, to run repeatedly:
}
