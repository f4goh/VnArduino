

/* Vna
 Anthony LE CREN F4GOH@orange.fr 
 Created 03/03/2015
 Ce programme dialogue avec le logiciel Jnva et blue vna app android
 Possiblilité de fonctionnement en standalone
 
Hardware : dds ad9851 + ad8302
http://ra4nal.lanstek.ru/vna.shtml


Dans le Jnva, menu Calibration frequency
remplacer   10737418  
par
23860477 
afin de calculer un DS_FTW correct

dialogue Jna avec la carte
http://wiki.oz9aec.net/index.php/MiniVNA_ICD

protocole de transfert:
MODE $0D   DDS_FTW $0D    SAMPLES $0D     DDS_STEP $0D   
Mode       StartF         NumberF         StepF

exemples :

acquisition :
 30 0D 32 33 38 36 30 34 38 0D 31 30 30 0D 34 32   0.2386048.100.42
 39 32 34 39 39 38 0D                              924998
stop
 30 0D 30 0D 31 0D 30 0D                             0.0.1.0.

générator
 30 0D 32 33 38 36 30 34 37 37 30 0D 31 0D 30 0D     0.238604770.1.0.

30 0D 32 33 38 36 30 34 37 37 0D 36 32 38 0D 33    0.23860477.628.3
37 39 38 36 0D                                       7986

manque dans le code :
detection deconnection bluevna
faire alors un reset de l'arduino
la gestion d'une carte sd

ad9851 informations :

DDS_CLCK = 180Mhz  (30Mhz*6)

DS_FTW = F_start * 2^32 / DDS_CLCK

F_start = DS_FTW * DDS_CLCK / 2^32

DDS_STEP = F_step * 2^32 / DDS_CLCK

deltaphase = f * 4294967296.0 / calibFreq;

exemple : retrouver la fréquence a partir de DS_FTW

74082466 * 180000000 / 4294967296.0 = 3104760,28 HZ

*/


#include <avr/pgmspace.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AD9850SPI.h>
#include <SPI.h>
#include <EEPROM.h>


#define MinFrq 23860900 // FTW  min freq = 1 Mhz
#define MaxFrq 1431655765 // FTW  max freq = 60 Mhz
#define freqMin  1000000
#define freqMax 60000000


#define PowerDown 0x04  //  Power down AD9851,50  
#define Normal 0x01     //  AD9851 RFCLK multiplier enable x 6 en mode normal

unsigned char Mode;       //mode powerdown ou normal
unsigned long  StartF;    //DS_FTW fréquence de départ
unsigned int NumberF;     //nombre d'échantillons
unsigned long  StepF;     //Fréquence d'incrémentation en FTW (non en HZ)

//char Temp;
unsigned int intTemp;    //variable de boucle pour le balayage
unsigned int adcmag;     //variables des 2 ADC mesures
unsigned int adcphs;
boolean check=0;



#define led  6        //Affectation des broches
#define Rele  5       //relais refexion, transmission 
#define ADC0  A0       //entrée mag
#define ADC1  A1      //entrée phs

#define  dpInEncoderA  2
#define  dpInEncoderB  3
#define  dpInEncoderPress 4


#define adc2Db 60/1024  // pente pleine echelle Db / resolution ADC = 0.0586
#define offsetDb -30  // décallage de -30db, pour ADC=512 -> 0db
#define Adc2Angle 180/1024  // pente pleine echelle Angle / resolution ADC = 0.175
#define D2R 3.14159/180    //degrés to radians


//#define calMag 0.703125      //before calibrate function
//#define calPhs 1.58203125

float calMag;
float calPhs;

struct vector_reflection{
  double Freq;
  float RL;
  float Phi;
  float Rho;
  float Rs;
  float Xs;
  float Swr;
  float Z;
};
vector_reflection Point;

struct vector_transmission{
  double Freq;
  float TL;
  float TP;
};

volatile long freq = 5000000;
byte vnaMode=0;
volatile byte menuSwapp=0;
byte menuChoose=0;
byte bandChoose;
volatile byte bandSwapp=0;
volatile byte bandSwappPrec=11;
double dds_reg;

long freq_prec = 0;               
long freqStep = 0;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);    //4 lines *20 columns lcd char




void(* resetFunc) (void) = 0; //declare reset function @ address 0 

int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}


void setup(){

lcd.begin(4, 20);  //4 lines *20 columns lcd char
lcd.setBacklight(HIGH);    
Serial.begin(115200);
pinMode(led, OUTPUT);
pinMode(Rele, OUTPUT);
DDS.begin(13,8,7);  // dans l'ordre broches W_CLK, FQ_UD et RESET
//DDS.calibrate(180000000);    //pas utile dans l'immédiat mais on ne sait jamais
analogReference(EXTERNAL);  //tension de référence extérieure 1.8V de l'ad8302


//A véfifier si utile
dds_reg=((double)freq) * 4294967296.0 / 180000000.0;
DDS.vna(dds_reg,Normal);
delay(1);
DDS.vna(dds_reg,PowerDown);  


  pinMode(dpInEncoderA, INPUT);
  digitalWrite(dpInEncoderA, HIGH);
  pinMode(dpInEncoderB, INPUT);
  digitalWrite(dpInEncoderB, HIGH);
  pinMode(dpInEncoderPress, INPUT);
  digitalWrite(dpInEncoderPress, HIGH);
  




adcmag=EEPROM.read(2)*256+EEPROM.read(1);
adcphs=EEPROM.read(4)*256+EEPROM.read(3);
calMag=((float)adcmag*adc2Db)+offsetDb;
calPhs=((float)adcphs*Adc2Angle);
//Serial.println(calMag);              //verif calibration
//Serial.println(calPhs);




  lcd.clear();
  lcd.print(F("     VNA v1.0"));    //intro
  lcd.setCursor(0, 2);
  lcd.print(F("    F4GOH 2015"));
  delay(4000);
  lcd.clear();

attachInterrupt(0, doEncoder, CHANGE);  // encoder pin on interrupt 0 - pin 2
boot_menu();


menuChoose=EEPROM.read(0);

switch(menuChoose)
{
case 0 :  menuJvna(0);   break;          //pc
case 1 :  menuJvna(1);  check=0; break;  //bluetooth
case 2 :  bandSelect(); break;           //standalone
}

}




//main loop

void loop(){

switch(menuChoose)
{
case 0 :  Jnva();   break;      //jvna
case 1 :  Jnva();   break;      //bluetooth
case 2 :  sweep();  break;      //standalone
}
}


/********************************************************
 * Jvna
 ********************************************************/

void Jnva()
{
         char Temp;
         Serial.flush();
     
         Temp = DecodeCom(); //acuisition et décodage d'une demande de mesure en provenance du logiciel IG_miniVNA
       if (menuChoose==1) {
                         StartF=(StartF/9)*20;
                         StepF=(StepF/9)*20;
                         }
         affiche_freqs();    //pour debug
         
                         
 if ((Temp==0) && (NumberF!=0)) // si le décodage est bon et que le nombre d'échantillons n'est pas nul alors mesure
          {
            digitalWrite(led, HIGH);      //vérification visuelle de la mesure
          if (Mode == 0) digitalWrite(Rele, LOW);   // commande relais tansmission 0 ou refection 1
                else digitalWrite(Rele, HIGH);
                
          for (intTemp=0; intTemp < NumberF; intTemp++)    //boucle des mesures
            {
           if ((StartF >= MinFrq) && (StartF <= MaxFrq)) {
                                   Mode = Normal;   // pour faire une mesure  F > 0,5 mhz 
                                  }
                                  else {
                                        Mode = PowerDown;  // F < 0,5 mhz, sinon Power Down ajouter test si F>fmax alors power down
                                       }
            DDS.vna(StartF,Mode);       //fonction dédiée au vna pour le dds 9851 meme si la lib est 9850
                                        // si le 9850 est utilisé, Normal doit etre à 0
            //delay(1);
            magPhsADC();
            Serial.write((byte)adcphs);     // LSB
            Serial.write((byte)(adcphs>>8));     // MSB
            Serial.write((byte)adcmag);     // LSB
            Serial.write((byte)(adcmag>>8));     // MSB
            StartF = StartF+StepF;
            }
          }
 digitalWrite(led, LOW);        //fin de mesure
 
}

void magPhsADC()  //average 20 ech
{
unsigned int amp;
unsigned int phs;  
            amp=0;
            phs=0;
            for (int n=0;n<20;n++){
                              amp = amp +  analogRead(ADC0);   //mesure 10 bits
                              phs = phs + analogRead(ADC1);
            }
            adcmag=amp/20;
            adcphs=phs/20;
}



// a virer plus tard
void debug_serial(void)
{
         Serial.print(F("Mode :"));
         Serial.println(Mode);
         Serial.print(F("Start F :"));
         Serial.println(StartF);
         Serial.print(F("Number F :"));
         Serial.println(NumberF);
         Serial.print(F("StepF :"));
         Serial.println(StepF);
}

// traitement chaine série

char DecodeCom (void)
{
char data, i, err=0,  Param[11];

data = getRX();                           // récupère un caractère réflextion , transmission
switch (data)
  {
  case '0': Mode = 0; break;
  case '1': Mode = 1; break;
  default: err = 1;
  }

data = getRX();                           // ok c'est un retour chariot 
if  (data !=0x0D) err = 1;
if  (err != 0)  return (err);               // ou erreur

for (i = 0; i <= 10; i++)                  // lire l'info sur le mot DS_FTW de start
  {
  data = getRX();
  if (data == 0x0D) break;                  // sort de la boucle si retour chariot 
  if (isdigit(data) ==1) Param[i] = data;    // vérification si c'est un un chiffre
     else  err = 1;                         // sinon erreur
  }
                         
if  ((i==0) | (i>10))  err = 1;              // erreur si DS_FTW start nul ou trop long
Param[i] = 0;                                // char nul avant conversion
if  (err != 0)  return (err);                //  return si erreur

StartF = atol (Param);                       // conversion en unsigned long

for (i = 0; i <= 5; i++)                   // meme principe avec le  nombre d'échantillons
  {
  data = getRX();
  if (data == 0x0D) break;                  
  if (isdigit(data) ==1) Param[i] = data;   
     else  err = 1;                         
  }
if  ((i==0) | (i>5))  err = 1;              
Param[i] = 0;                               
if  (err != 0)  return (err);               

NumberF = atoi (Param);                     

for (i = 0; i <= 10; i++)                  // meme principe avec le  DS_FTW  step
  {
  data = getRX();
  if (data == 0x0D) break;                  
  if (isdigit(data) ==1) Param[i] = data;   
     else  err = 1;                         
  }
if  ((i==0) || (i>10))  err = 1;            
Param[i] = 0;                               
if  (err != 0)  return (err);               

StepF = atol (Param);                       
return (0);
}

/*
mode bluetooth 115200 bauds

<\r><\n>CONNECT,BCCFCC36B9BC<\r> <\n>0<\r>0<\r>1<\r>0<\r>

0<\r>1073804<\r>1000<\r>1931773<\r>

<\r><\n>DISCONNECT<\r><\n>
*/

char getRX(void)
{
char data=0;
do
{
 if (Serial.available()) {
                         data=Serial.read();
                         if(check==0) {
                                     if (data==0x0D) {
                                                   data=0;
                                                    while (data!=0x0d)                                
                                                    {
                                                    if (Serial.available()) data=Serial.read();  
                                                    }
                                                    while (data!='0')
                                                    {
                                                    if (Serial.available()) data=Serial.read();  
                                                    }
                                                 }
                                     }
                                     check=1;
                       }
}
while((data != 0x0D)&&(isdigit(data) !=1));
return data;
}



void affiche_freqs(void)
{
  if ((StartF>=MinFrq) && (StartF<=MaxFrq)) { 
     delete_char(1,8,19); 
     lcd.setCursor(8,1);
     lcd.print((unsigned long)ticksToFreq(StartF));
     lcd.print(" Hz");
     delete_char(2,8,19); 
     lcd.setCursor(8,2);
     lcd.print((unsigned long)ticksToFreq(StepF));
     lcd.print(" Hz");
     delete_char(3,8,19); 
     lcd.setCursor(8,3);
     lcd.print(NumberF);
   }
}

double ticksToFreq(long f)
{
 return ((double)f) * 180000000.0 / 4294967296.0;  
}


void menuJvna(byte PB)
{
  lcd.clear();
 if (PB==0)   lcd.print(F("JVna PC"));  else  lcd.print(F("Blue Vna Android"));
  lcd.setCursor(0, 1);
  lcd.print(F("Fstart:")); 
  lcd.setCursor(0, 2);
  lcd.print(F("Fstep:")); 
  lcd.setCursor(0, 3);
  lcd.print(F("Samples:"));   
  
}

/********************************************************
 * Bluetooth
 ********************************************************/

// géré dans le JvnaPC



/********************************************************
 * StandAlone
 ********************************************************/

void sweep()
{
char tab[10];
int freqLcd;
byte bpState=0;
if (freq!=freq_prec){
                   double dds_reg=((double)freq) * 4294967296.0 / 180000000.0;
                   DDS.vna(dds_reg,Normal);  
                   freq_prec=freq;
                   BCD(freq,tab);
                   lcd.setCursor(5, 0);
                   lcd.write(tab[7]);
                   lcd.write(tab[6]);                   
                   lcd.write(46);
                   lcd.write(tab[5]);                   
                   lcd.write(tab[4]);                                      
                   mesure();
                   }

if (digitalRead(dpInEncoderPress)==0) bpState|=1;
if (bpState==1) bandSelect();
bpState=(bpState<<1)&3;
}

void BCD (unsigned long b, char* o)
{
   for (int i=10; i; --i)
   {
      *o = (b % 10)+48;
      b /= 10;
      o++;
   }
}




void mesure()
{

magPhsADC();
calculDut(adcmag,adcphs);
vna_print();
delete_char(1,3,9); 
lcd.setCursor(3, 1); 
lcd.print((int)Point.RL);
lcd.print(F("dB"));
delete_char(2,4,9); 
lcd.setCursor(4, 2);
lcd.print((int)Point.Phi);
lcd.write(0xdf);
delete_char(1,13,19); 
lcd.setCursor(13, 1);
lcd.print((int)Point.Rs);
lcd.write(0xf4);
delete_char(2,13,19); 
lcd.setCursor(13, 2);
lcd.print((int)Point.Xs);
lcd.write(0xf4);
delete_char(3,2,9); 
lcd.setCursor(2, 3);
lcd.print((int)Point.Z);
lcd.write(0xf4);
delete_char(3,14,19); 
lcd.setCursor(14, 3);
if (Point.Swr>=1)
{
lcd.print((int)Point.Swr);
lcd.write(46);
byte tempSwr=(int)((Point.Swr-(int)(Point.Swr))*100);
if (tempSwr/10==0) lcd.write(48);
lcd.print(tempSwr);
}
}

void delete_char(byte line, byte start, byte end)
{
while(start<=end) {
                  lcd.setCursor(start, line);
                  lcd.write(32);
                  start++;
                  }
}




/********************************************************
 * Compute Vna datas
 ********************************************************/
 
 /*
//formulas
RL=-20log(Rho)
Rho=10^(Rl/-20)
Z=(ZL-ZO)/(ZL+Z0) avec Z0=50ohms
Z=a+jb avec 
a=Rho*cos(phi)
b=Rho*sin(phi)
ZL=(1+Z)/(1-Z)*Z0
ZL=RS+jXS avec
RS=abs(1-a²-b²)/((1-a)²-b²)
XS=abs(2b/((1-a)²-b²))
|Z|=sqrt(RS²+XS²)
SWR=(1+Rho)/(1-Rho)
*/

void calculDut(int adcMag, int adcPhs)
{
Point.Freq=freq;  
Point.RL=((float)adcMag*adc2Db)+offsetDb-calMag;
Point.Phi=((float)adcPhs*Adc2Angle)-calPhs;
Point.Rho=pow(10.0,Point.RL/-20.0);
float re=Point.Rho*cos(Point.Phi * D2R);
float im=Point.Rho*sin(Point.Phi * D2R);
float denominator=((1-re)*(1-re)+(im*im));
Point.Rs=fabs((1-(re*re)-(im*im))/denominator)*50.0;
Point.Xs=fabs(2.0*im)/denominator*50.0;
Point.Z=sqrt(Point.Rs*Point.Rs+Point.Xs*Point.Xs);
Point.Swr=fabs(1.0+Point.Rho)/(1.001-Point.Rho);
Point.RL*=-1;
}

void vna_print()
{
Serial.print(freq);
Serial.write(9);
Serial.print(adcmag);
Serial.write(9);
Serial.print(adcphs);
Serial.write(9);
Serial.print(Point.RL);
Serial.write(9);
Serial.print(Point.Phi);
Serial.write(9);
Serial.print(Point.Rho);
Serial.write(9);
Serial.print(Point.Rs);
Serial.write(9);
Serial.print(Point.Xs);
Serial.write(9);
Serial.print(Point.Z);
Serial.write(9);
Serial.println(Point.Swr);
}


/********************************************************
 * Encoder 
 ********************************************************/


void doEncoder() {
  if (digitalRead(dpInEncoderA) == digitalRead(dpInEncoderB)) {
    menuSwapp=(menuSwapp+1)%4;
    bandSwappPrec=bandSwapp;
    bandSwapp=(bandSwapp+1)%12;
    freq+=freqStep;
    if (freq>freqMax) freq=freqMax; 
  } else {
    freq-=freqStep;
    if (freq<freqMin) freq=freqMin; 
  }
}

/********************************************************
 * Lcd menus
 ********************************************************/ 

void lcd_menu_analyse_refection()
{
  lcd.clear();
  lcd.print(F("FREQ:"));
  lcd.setCursor(11, 0);
  lcd.print(F("MHz"));  
  lcd.setCursor(0, 1);
  lcd.print(F("RL:"));  
  lcd.setCursor(10, 1);
  lcd.print(F("RS:"));  
  lcd.setCursor(0, 2);
  lcd.print(F("Phi:"));  
  lcd.setCursor(10, 2);
  lcd.print(F("XS:"));  
  lcd.setCursor(0, 3);
  lcd.print(F("Z:"));  
  lcd.setCursor(10, 3);
  lcd.print(F("SWR:"));  
}


 
void boot_menu()
{
  if (digitalRead(dpInEncoderPress)==1) return;
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(F("JVna PC"));
  lcd.setCursor(1, 1);
  lcd.print(F("Blue Vna Android"));  
  lcd.setCursor(1, 2);
  lcd.print(F("Standalone Reflect."));  
  lcd.setCursor(1, 3);
  lcd.print(F("Calibration    V1.0"));  
  while(digitalRead(dpInEncoderPress)==0) {}  //BP rise down detect
  while(digitalRead(dpInEncoderPress)==1) {
  for (byte n=0;n<=3;n++) {
        lcd.setCursor(0, n); 
        if (menuSwapp==n) lcd.write(42); else lcd.write(32);
        }
  }
  menuChoose=menuSwapp;
  for (byte n=0;n<=3;n++) {
          if (menuChoose!=n) {
                           for (byte m=0;m<20;m++) {
                              lcd.setCursor(m, n);
                              lcd.write(32);
                           }
        }
  }
  delay(1000);
  if (menuChoose<3) EEPROM.write(0,menuChoose); else calibration();
  resetFunc();  //call reset
}


void bandSelect()
{
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(F("160m  80m   60m"));
  lcd.setCursor(1, 1);
  lcd.print(F(" 40m  30m   20m"));
  lcd.setCursor(1, 2);
  lcd.print(F(" 17m  15m   12m"));
  lcd.setCursor(1, 3);
  lcd.print(F(" 10m   6m   Free"));
  delay(100);    //pour le bp
  while(digitalRead(dpInEncoderPress)==0) {}  //BP rise down detect
  while(digitalRead(dpInEncoderPress)==1) {
  byte x,y;
  x=(bandSwappPrec%3)*6;
  y=(bandSwappPrec/3);
  lcd.setCursor(x, y);
  lcd.write(32);
  x=(bandSwapp%3)*6;
  y=(bandSwapp/3);
  lcd.setCursor(x, y);
  lcd.write(42);

  }
  delay(100);    //pour le bp
  bandChoose=bandSwapp;
  switch (bandChoose)
  {
  case 0 : freq =  1800000; break;
  case 1 : freq =  3500000; break;  
  case 2 : freq =  5300000; break;    
  case 3 : freq =  7000000; break;    
  case 4 : freq = 10100000; break;    
  case 5 : freq = 14000000; break;    
  case 6 : freq = 18100000; break;    
  case 7 : freq = 21000000; break;    
  case 8 : freq = 24900000; break;    
  case 9 : freq = 28000000; break;    
  case 10 : freq =50000000; break;    
  default : freq =14000000;
  }
  
  if (bandChoose<11) freqStep=10000; else freqStep=100000;
  lcd_menu_analyse_refection(); 
  freq_prec=0;
  vna_print_unites();
}


void vna_print_unites()
{
  Serial.println(F("Freq\tAdcmag\tAdcphs\tRL\tPhiMag\tRS\tXs\tZ\tSWR"));
}
/********************************************************
 * Calibration
 * one point calibration only
 ********************************************************/ 
 

void calibration()
{
lcd.clear();
lcd.print(F("Leave open DUT"));  
lcd.setCursor(0, 1);
lcd.print(F("and press button")); 
while(digitalRead(dpInEncoderPress)==0) {}  //BP rise down detect
while(digitalRead(dpInEncoderPress)==1) {} 
DDS.vna(dds_reg,Normal);
delay(10);
lcd.clear();
mesure();
DDS.vna(dds_reg,PowerDown);  
EEPROM.write(1,(byte) adcmag);
EEPROM.write(2,(byte) (adcmag>>8));
EEPROM.write(3,(byte) adcphs);
EEPROM.write(4,(byte) (adcphs>>8));
delay(2000);
lcd.clear();
lcd.print(F("Done")); 
lcd.setCursor(0, 1);
lcd.print(F("Press button to")); 
lcd.setCursor(0, 2);
lcd.print(F("return normal mode")); 
while(digitalRead(dpInEncoderPress)==0) {}  //BP rise down detect
while(digitalRead(dpInEncoderPress)==1) {} 
delay(2000);
}



