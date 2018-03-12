#include <TimerOne.h>                             // Génère une interruption toutes les TchargeBob ou DsecuBob µs

//**************************************************************
                                                  // MM version Me: Multi-Etincelles, Ub: Ubatterie, Nk: comptage nombre de tours pour le démarrage au kick, C72 : cible de 72°
                                                  // Version dérivée des AEPL & AECP Multi Etincelles Nano. Allumage électronique programmable - Arduino
char ver[] = "version Michel du 12_03_2018";      // MM correction de l'avance à la volée avec sélecteur en A1 (réglage du pas par "PasCorecAv")
                                                  // MM allumage à 0° d'avance au démarrage au kick (fin de cible au PMH), Temps d'avance non calculé jusqu'à Nplancher
                                                  // La cible doit donc couvrir ici pour Triumph T140 : 72° vilebrequin, et pas juste le point de déclenchement  ****** ATTENTION CIBLE 72° ******
                                                  // En option, connexion d'un sélecteur de courbe entre la patte A4 et la masse,
                                                  // pour changer de courbe d'avance en fonction du régime moteur (courbe d'avance mécanique, centrifuge)
                                                  // T > 14ms, correction Christophe.
                                                  // LED de calage statique de l'allumage
                                                  // Flash LED pour calage dynamique de l'allumage (stroboscope à LED)
                                                  // MM avance fixe jusqu'à Nplancher et temps de charge bobine = temps de la cible présente (72°)
                                                  // MM led rouge verte (trois couleurs R + V = Jaune) pour témoin de charge batterie
                                                  // MM remise à 0 de N si le moteur n'a pas démarré dans gestionnaire I bobine pour reprendre démarrage au kick
                                                  // MM modification simplification fonction multi-étincelles
                                                  // MM Temps de charge bobine majoré en cas d'accélération en service
                                                  // MM charge bobine sous Nmulti (1500 tr/mn) à partir du début de cible à 72° soit 8 ms minimum, pour résoudre problème Timer1 et isr gestion I bobine
                                                  
//******************************************************************************

// Si on passe à 1 un de ces Debug on peut lire les valeurs lors du déroulement du programme, mais il est ralenti pas ces lectures donc ne pas laisser à 1 pour le fonctionnement normal (sauf printMM)

#define Debug_CalculT 0        // Lire T dans CalculD
#define Debug_CalculD 0        // Lire D dans CalculD
#define Debug_Tloop 0          // Lire T dans Loop
#define Debug_Davant_TchargeBob 0 // Lire temps avant recharge bobine 
#define Debug_printMM 1        // Sortie série N régime et Av avance (peut servir avec PC, ou Smartphone, ou Tablette, pour lire régime moteur et avance)
#define Debug_DureeAlimBob 0   // Lire durée alimentation des bobines
#define Debug_Fonction 0       // Lire les fonctions activées pendant le déroulement du programme

// Si on ajoute pc(nom_de_la_variable) dans une ligne du programme, il s'arrête en donnant la valeur de cette variable (ex pc(T), pc(D) etc.)  et continue si on met un caractère et "envoyer"
#define pc(v) Serial.print("Ligne_") ; Serial.print(__LINE__) ; Serial.print(" ; ") ; Serial.print(#v) ; Serial.print(" = ") ; Serial.println((v)) ; Serial.println(" un caractère "a" par exemple et 'Envoyer' pour continuer") ; while (Serial.available()==0);{ int k_ = Serial.parseInt() ;}


//************* Ces lignes expliquent la lecture de la dépression ****************
                // Pour la dépression ci-dessous le tableau des mesures relevées sur un banc Souriau
                // Degdep = map((analogRead(A0)),xhigh,xlow,yhigh,ylow);
int xhigh = 0;  // valeur ADC haute pour conversion de la valeur mmHg haute
int xlow = 0;   // valeur ADC basse pour conversion de la valeur mmHg basse
int yhigh = 0;  // valeur de la limite en degré haute x 10 pour avoir les centièmes
int ylow = 0;   // valeur de la limite en degré basse soit 0 degré
                // tableau des valeurs mmHg vs 8 bits
                // {0,707}{25,667}{50,620}{60,601}{75,576}{80,565}{95,538}{100,530}{125,485}{150,438}{175,394}{185,373}{200,350}{210,330}{225,305}{250,260}{275,216}{300,172}
                // relevé en activant la ligne Degdepcal et en ajoutant cette variable à l'édition dans le terminal série
                // on peut en déduire une équation qui donne la valeur en mmHg au départ de la conversion ADC, utilisé sous Processing
                // mmHg = (707 - valeur ADC)/1.78 ou 1.78 est la pente du capteur pour tension VS mmHg

//************** Ces lignes sont à renseigner obligatoirement.****************
          // Ce sont : Na[],Anga[], Ncyl, AngleCapteur, CaptOn, Dwell
          // Les tableau Na[] et Anga[] doivent obligatoirement débuter puis se terminer par 0
          // et contenir des valeurs entières >= 1
          // Le nombre de points est libre. (L'avance est fixe entre Ndem et Nplancher t/mn)
          // Le dernier N fixe la ligne rouge, c'est à dire la coupure de l'allumage
//***************  Courbe Avance régime à renseigner 

// Courbe d'avance type Triumph T140 1980 (750cc)****************************************************************

          // Au régime moteur de (obligatoirement croissant):
int Na[] = {0, 500, 600, 700,  800,  900, 1000, 1500, 2000, 2500, 3000, 3500, 5000, 6800, 7000, 0}; // Courbe a (Triumph T140)
          // degrés d'avance vilebrequin correspondant:
int Anga[] = {0, 5,  5,  5,   14,    13,   12,   18,   25,   29,   32,   33,   35,   35,    5,  0};
          
int Ncyl = 2;                     // Nombre de cylindres
const int AngleCapteur = 72;      // Position en degrès (vilebrequin) avant le PMH, du capteur (Hall ou autre), coté distribution et allumeur c'est 36° pour toujours avoir AngleCapteur = 72° (vilebrequin) 

const int CaptOn = 1;             // CapteurOn = 1 déclenche sur front montant (capteur saturé) le capteur conduit quand il n'y a pas le disque qui fait écran à l'aimant
                                  // CapteurOn = 0 déclenche sur front descendant (capteur non saturé). le capteur Hall conduit (donc 0 V) quand le disque aimanté passe devant Voir explications en fin du listing
                                  
                                  //***** MULTICOURBES **** IL FAUT TOURNER LE SELECTEUR POUR SELECTIONNER LA COURBE D'AVANCE *******
                                  // A la place de la courbe a, on peut sélectionner la courbe b, c, d ou e
                                  // Un sélecteur rotatif et 4 résistances de 4K7, 18K (ou 22k), 47K et 100K font le job
                                  // avec l'entrée configurée en Input Pull-up, ainsi s'il y a une coupure on reste sur la valeur par défaut
                                  //***************  les autres Courbes Avance régime à renseigner si besoin                                  
                                  //*******//*********Courbe   b
int Nb[] = {0, 500, 600, 700,  800,  900, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 6800, 7000, 0};   // Courbe b
int Angb[] = {0, 5,  10,  14,   14,   10,   14,   22,   24,   26,   30,   30,   30,   28,   5,  0};
                                  //*******//*********Courbe   c
int Nc[] = {0, 500, 600, 700,  800,  900, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 6800, 7000, 0};   // Courbe c
int Angc[] = {0, 5,  12,  16,   16,   12,   16,   24,   26,   28,   32,   32,   32,   28,   5,  0};
                                  //*******//*********Courbe   d
int Nd[] = {0, 500, 600, 700,  800,  900, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 6800, 7000, 0};   // Courbe d
int Angd[] = {0, 5,  14,  18,   18,   14,   18,   26,   28,   30,   34,   34,   34,   28,   5,  0};
                                  //*******//*********Courbe   e
int Ne[] = {0, 500, 600, 700,  800,  900, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 6800, 7000, 0};   // Courbe e
int Ange[] = {0, 5,  16,  20,   20,   16,   20,   28,   30,   32,   36,   36,   36,   28,   5,  0};
           //**********************************************************************************

           //********************FLASH********************
                       // Module flash 3W à Led (type Keyes) connecté sur A3, +5V et la masse
                       // Décommenter dans la fonction Etincelle ()la ligne:
                       // digitalWrite(Flash, 1); delayMicroseconds(tFlash); digitalWrite(Flash, 0); 
int tFlash = 100;      // Durée du flash en µs 100, typique
           //*********************************************

           // Valable pour tout type de capteur soit sur vilo soit dans l'allumeur (ou sur l'arbre à came)
           // La Led(D13) existant sur tout Arduino suit le courant dans la bobine
           // En option, multi-étincelles à bas régime pour dénoyer les bougies
           // En option, correction d'avance "au vol" moteur en fonctionnement en A1
           // pour décaler la courbe de quelques degrés en plus (sauf sous Nplancher), voir PasCorecAv plus bas (ex 2°)
           // En option,multi courbes d'avance "centrifuge", 5 courbes possibles, selectionables par A4
           // En option,multi courbes "dépression", 5 courbes possibles, selectionables par A5
           // En option, connexion d'un module flash à Led en A3
           // Pour N cylindres,2,4,6,8,12,16, 4 temps, on a N cames d'allumeur ou  N/2 cibles sur le vilo
           // Pour les moteurs à 1, 3 ou 5 cylindres, 4 temps, il FAUT un capteur dans l'allumeur (ou sur
           // l'arbre à cames, c'est la même chose)
           // Exception possible pour un monocylindre 4 temps, avec capteur sur vilo et une cible : on peut génèrer
           // une étincelle perdue au Point Mort Bas en utilisant la valeur Ncyl = 2.
           // Avance 0° jusqu'a Nplancher t/mn, anti retour de kick et anti retour pour le démarreur (en particulier moto).
           
           //************ Ces valeurs sont eventuellement modifiables *****************
                                             // Ce sont Nkick Nplancher, TchargeBob , DsecuBob, Ndem, PasCorecAv
const byte Nkick = 3;                        // MM Nkick seuil du nombre de tours moteur au démarrage, utilisé pour le démarrage au kick et forcer l'allumage à 0°  ******* Nombre de tours au kick *******
const int Nplancher = 500;                   // vitesse en t/mn jusqu'à laquelle l'avance est fixe à 0°, doit être supérieur à Ndem, au-dessus l'avance est celle programmée et avec correction
const unsigned long DsecuBob = 5000000;      // Sécurité: bobines coupées après "DsecuBob" en µs, ex 5 sec pour laisser le temps de démarrer au kick après avoir mis le contact
const int TchargeBob = 7500;                 // temps de recharge bobine, ex minimum 7 ms bobines Lucas T140, 5 ms à 14,5 V U batterie pour bobine Bosch crayon (mesuré sur Clio 1.6L 16S)
const unsigned long TsecuKick = 1000000;     // Sécurite: remettre à 0 N tours moteur si le moteur a calé, temps en uS, ex 1 sec
const int NgestBob = 1400;                   // Seuil du régime où isr Gest Bobine entre en action
const unsigned long Tdem = 200000;           // Valeur de T au démarrage pour le premier tour moteur, exemple 200 ms pour 300 tr/mn

int PasCorecAv = 2;                          // MM pas d'avance pour correction, par ex 2°. Quand le sélecteur avance d'une position, l'avance croit de "PasCorecAv"

          //*********************** Majoration temps de charge bobine en accélération ************************************

const byte MajorTchargBob = 1;               // MM majoration du temps de charge bobine en phase d'accélération 1 en service 0 hors service
const int LimitMajorTchargBob = 10000;       // MM limite du régime maxi de majoration temps de charge bobine ex : 10 ms = 6 000 tr/mn
const float KmajorTchargBob = 1.5;           // MM Coefficient de majoration du temps de charge bobine en phase d'accélération

          //*********************** Surveillance tension batterie ************************************

volatile unsigned int tension = 0;           // valeur de A2 lue, tension batterie de 0 à 1023
volatile float Ubat = 0;                     // U batterie en Volt, de 0 à 15 V
volatile unsigned int fxUbat = 0;            // Pour lecture U batterie avec virgule fixe (optimisation par rapport à virgule flottante)

          //*********************** Multi-étincelles ************************************

                                             // Si multi-étincelles désiré jusqu'à N_multi, modifier ces deux lignes
const int Multi = 1;                         // 1 pour multi-étincelles, 0 pour mettre HS
const int N_multi = 1500;                    // limite supérieure en t/mn du multi-étincelles
const int EtinMulti = 3;                     // Nombre d'étincelles après la principale en mode multi-étincelles
const float CoefTchargeBobMulti = 0.2;       // Coefficient (0 à 1, ex 0.2 Bosch) appliqué au temps de charge lors d'un multi-étincelles, le temps de charge est plus court que pour la première étincelle
int N_Etin = 0;                              // Comptage nombre d'étincelles après la première

          //*********************** Constantes du sketch ************************************
          
const int Bob1 = 4;                 // Sortie D4 vers bobine1
const int Bob2 = 8;                 // Sortie D8 vers bobine2
const int Cible = 2;                // Entrée sur D2 du capteur, R PullUp et interrupt
const int Depression = A0;          // Entrée capteur de dépression, MM : R PullUp
const int SelecAv = A4;             // Entrée analogique sur A4 pour sélecteur de changement de courbes avance centrifuge. R PullUp
const int SelecDep = A5;            // Entrée analogique sur A5 pour sélecteur de changement de courbes d'avance dépression. R PullUp
const int Led = 13;                 // Sortie D13 avec la led built-in (interne sur l'Arduino Nano) pour caller le disque par rapport au capteur
const int Flash = A3;               // Sortie A3 vers module Flash Led 3 w Keyes
const int Vbatterie = A2;           // Entrée mesure tension batterie 12 V
const int LedRouge = 9;             // Sortie led témoin de charge batterie en D9
const int LedVerte = 10;            // Sortie led témoin de charge batterie en D10

          //**************** MM Option sélecteur pour modifier l'avance pendant le fonctionnement moteur ***************           
const int CorAv = A1;               // MM entrée analogique sur A1 pour sélecteur de correction de l'avance (essai d'avance à la volée),  R PullUp             
          //*******************************************************************************************

float modC1 = 0;                    // MM Correctif pour C1[]

unsigned long D = 0;                // D délai en µs et ms à attendre après la cible pour l'étincelle
unsigned long Ddep = 0;             // D dépression
unsigned long Dsum = 0;             // D total
unsigned long Dfixe = 0;            // D fixe entre Ndem et Nplancher

byte AngAvfixe = 0;                 // MM angle d'avance fixe entre Ndem et Nplancher ************* avance entre Ndem et Nplancher ***************

int valSelecAv = 0;                 // 0 à 1023 selon la position du sélecteur en entrée de courbe d'avance "centrifuge"
int valSelecDep = 0;                // 0 à 1023 selon la position du sélecteur en entrée de courbe d'avance à dépression (charge moteur)
int valCorAv = 0;                   // MM 0 à 1023 selon la position du sélecteur en entrée de correction de l'avance en fonctionnement
int milli_delay = 0;
int micro_delay = 0;
float Tplancher = 0;                // Seuil pour régime plancher

unsigned int tcor = 0;              // correction en µs du temps de calcul pour D, lecture de dépression + traitement divers, modifié dans INIT

unsigned long Tseuil = 0;           // T seuil pour correction du TchargeBob à bas régime, ce seuil est fonction de Tcharge Bobine et est calculé dans Init
unsigned long Davant_TchargeBob = 0;// Délai en µs après étincelle pour démarrer la recharge bobine.
unsigned long TchargeBobMajor = 0;  // Délai majoré (accélération) en µs après étincelle pour démarrer la recharge bobine.
unsigned long Tprec = 0;            // Période précédant la T en cours, pour calcul de TchargeBob
unsigned long prec_H = 0;           // Heure du front précédent en µs
unsigned long T = 0;                // Période en cours
int N1 = 0;                         // Couple N,A de début d'un segment
int Ang1 = 0;                       // Car A1 réservé pour entrée analogique!
int N2 = 0;                         // Couple N,A de fin de segment
int Ang2 = 0;
int*  pN = &Na[0];                  // pointeur au tableau des régimes. Na sera la courbe par défaut
int*  pA = &Anga[0];                // pointeur au tableau des avances. Anga sera la courbe par défaut
float k = 0;                        // Constante pour calcul de l'avance courante
float C1[20];                       // Tableaux des constantes de calcul de l'avance courante
float C2[20];                       // Tableaux des constantes de calcul de l'avance courante
float Tc[20];                       // Tableau des Ti correspondants au Ni
                                    // Si nécessaire, augmenter ces 3 valeurs: Ex C1[30],C2[30],Tc[30]
int Tlim  = 0;                      // Période minimale, limite, pour la ligne rouge
int j_lim = 0;                      // index maxi des N, donc aussi Ang
unsigned long NT  = 0;              // Facteur de conversion entre N et T à Ncyl donné
int AngleCibles = 0;                // Angle entre 2 cibles, 180° pour 4 cyl, 120° pour 6 cyl, par exemple
int UneEtin = 1;                    // = 1 pour chaque étincelle, testé par isr_CoupeI et remis à zero

int Mot_OFF = 0;                    // Sera 1 si le moteur est detecté arrété par l'isr_GestionIbob()

unsigned long T_multi  = 0;         // Période minimale pour multi-étincelles
unsigned long TgestBob = 0;         // Période seuil pour gestion isr gest bobine

float uspardegre = 0;
int Dep = 0;
float Degdep = 0;

float Vitesse = 0;              // vitesse en cours
unsigned int Regime = 0;        // régime moteur affiché (partie entière de vitesse)

byte N = 0;                     // MM comptage des tours moteurs pour le démarrage au kick

unsigned long Delaideg = 0;     // µs/deg pour la dépression
                                    
unsigned long Stop_temps_Etincelle = 0;    // MM ajout de = 0
unsigned long Tempsecoule = 0;
float AvanceMoteur = 0;
int fxAvanceMoteur = 0;
unsigned int fxAvanceMoteur2 = 0;

                                    // Define various ADC prescaler
const unsigned char PS_16 = (1 << ADPS2);
const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
const unsigned char PS_64 = (1 << ADPS2) | (1 << ADPS1);
const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);


                  //********************* INITIALISATION *********************
                  

void setup()                //////////////////while (1); delay(1000);//////////////////////////
{
  Serial.begin(115200);         // Ligne suivante, 3 Macros du langage C
  Serial.println(__FILE__); Serial.println(__DATE__); Serial.println(__TIME__);
  Serial.println(ver);
  
  if (Ncyl < 2)Ncyl = 2;           // On assimile le mono cylindre au bi, avec une étincelle perdue

  pinMode(Vbatterie, INPUT);       // Entrée sur A2 mesure tension batterie 12V 1/3 
  pinMode(Cible, INPUT_PULLUP);    // Entrée interruption sur D2, front descendant (résistance pullup)
  pinMode(Bob1, OUTPUT);           // Sortie sur D4 controle du courant dans la bobine1
  pinMode(Bob2, OUTPUT);           // Sortie sur D8 controle du courant dans la bobine2
                                   // Nota: on peut connecter une Led de controle sur D4 avec R=330 ohms vers la masse
  pinMode(Depression, INPUT_PULLUP);  // Entrée capteur de dépression                               
  pinMode(SelecAv, INPUT_PULLUP);  // Entrée sélecteur rotatif avec résistances, optionnel ! Sélection courbe d'avance
  pinMode(SelecDep, INPUT_PULLUP); // Entrée sélecteur rotatif avec résistances, optionnel ! Sélection courbe de dépression
  pinMode(CorAv, INPUT_PULLUP);    // MM Entrée pour sélecteur rotatif avec résistances, optionnel ! Sélection correction d'avance
  pinMode(Led, OUTPUT);            // pour signaler le calage statique du capteur lors de la mise au point
  pinMode(Flash, OUTPUT);          // Sortie vers LED stroboscope, en A3
  pinMode(LedRouge, OUTPUT);       // Sortie led témoin de charge
  pinMode(LedVerte, OUTPUT);       // Sortie led témoin de charge  
  
                                // set up the ADC
  ADCSRA &= ~PS_128;            // remove bits set by Arduino library
                                // you can choose a prescaler from above.
                                // PS_16, PS_32, PS_64 or PS_128
  ADCSRA |= PS_64;    
                                // set our own prescaler to 64 
                                
  Init();                       // Executée une fois au démarrage et à chaque changement de courbe
}


void  Init ()           //////////////////// Calcul des tableaux d'avance /////////////////////////////
                               // Calcul de 3 tableaux,C1,C2 et Tc qui serviront à calculer D, temps d'attente
                               // entre la détection d'une cible par le capteur  et la génération de l'étincelle.
                               // Le couple C1,C2 est determiné par la période T entre deux cibles, correspondant au
                               // bon segment de la courbe d'avance entrée par l'utilisateur: T est comparée à Tc
{ 
  AngleCibles = 720 / Ncyl;                    // Ex : 720° pour 1 ou 2 cylindres (à étincelle perdue), 180° pour 4 cylindres, et 120° pour 6 cylindres
  NT = 120000000 / Ncyl;                       // Facteur de conversion Nt/mn en T µs

  T_multi = NT / N_multi;                      // Période minimale pour génerer un train d'étincelles
                                               // T temps entre 2 étincelles soit 720°  1°=1/6N
  TgestBob = NT / NgestBob;                    // T gestion Bobine, seuil de calcul du temps de charge bobine par isrGestBob

  Tplancher = 120000000 / Nplancher / Ncyl;    // T à vitesse plancher en t/mn, en dessous, avance centrifuge = 0

  Select_Courbe_Avance();                      // Ajuster éventuellement les pointeurs pN et pA pour la courbe a, b, c, d ou e 
  Select_Courbe_Depression();                  // Sélectionne la courbe a, b, c ,d ou e de dépression
  
  N1  = 0; Ang1 = 0;                           // Toute courbe part de  0
  int i = 0;                                   // locale mais valable hors du FOR
  pN++; pA++;                                  // sauter le premier élément de tableau, toujours =0
  for (i  = 1; *pN != 0; i++)                  // i pour les C1,C2 et Tc. Arrêt quand régime=0.
                                               // pN est une adresse (pointeur) qui pointe au tableau N. Le contenu pointé est *pN
  { N2 = *pN; Ang2 = *pA;                      // recopier les valeurs pointées dans N2 et Ang2
    k = float(Ang2 - Ang1) / float(N2  - N1);
    
    C1[i] = float(AngleCapteur - Ang1 + k * N1) / float(AngleCibles);  // MM suppression Avancestatique  

    C2[i] = -  float(NT * k) / float(AngleCibles);
    Tc[i] = float(NT / N2);
    N1 = N2; Ang1 = Ang2;                      // fin de ce segment, début du suivant
    pN++; pA++;                                // Pointer à l'élément suivant de chaque tableau
  }
  j_lim = i - 1;                               // Revenir au dernier couple entré
  Tlim  = Tc[j_lim];                           // Ligne rouge
  
                                               // Timer1 a deux roles:
                                               // 1....couper le courant dans la bobine en l'absence d'étincelle pendant plus de DsecuBob µs
                                               // 2... après une étincelle, attendre le délai D recharge bobine avant de rétablir le courant dans la bobine
                                               // Ce courant n'est rétabli que TchargeBob ms avant la prochaine étincelle, condition indispensable
                                               // pour une bobine à faible résistance, disons inférieure à 3 ohms. Typiquement TchargeBob = 3ms à 5ms.
//  Timer1.attachInterrupt(isr_GestionIbob);     // IT d'overflow de Timer1 (16 bits), à l'échéance de Timer1 (interruption) gérér alimentation des bobines avant étincelle
                                               // MM Timer1.attachInter. a été mis dans loop sinon problème au premier tour moteur de démarrage
  
  digitalWrite(Bob1, 0);                       // par principe, couper la bobine
  digitalWrite(Bob2, 0);                       // par principe, couper la bobine

  tcor = 390;                                  // Correction temps de calcul (mesure faite à l'oscillo à pleine avance) ******************** tcor *********************
}


                  //********************LES FONCTIONS*************************

void  CalcD ()    ////////////////// Calcul du temps D entre top capteur à 72° et étincelle d'allumage 
{                                            // Noter que T1>T2>T3...
 
 for (byte j = 1; j <= j_lim; j++)           // On commence par T la plus longue et on remonte, tant que j < limite on augmente j + 1
  {                                          // Serial.print("Tc = "); Serial.println(Tc[j]);delay(1000);

///////////////////////// Debug_CalculT //////////////////
   if ( Debug_CalculT) {
       Serial.print(" ; T = ");  Serial.println(T);
   }
//////////////////////////////////////////////////////// 

///////////////////////// Debug_Fonction //////////////////
   if ( Debug_Fonction) {
       Serial.println("CalD");
   }
//////////////////////////////////////////////////////// 
  
    if  (T >=  Tc[j]) {                      // Si T période en cours supérieur ou égal à j tableau des Ti et Ni
                                             // on a trouvé le bon segment de la courbe d'avance
                                             
      D =  float(T * ( C1[j] - modC1 ) + C2[j]) ;   // MM D en µs = T x (C1 - correctif C1) + C2

///////////////////////// Debug_CalculD //////////////////
   if ( Debug_CalculD) {
       Serial.print(" ; D = ");  Serial.println(D);
   }
//////////////////////////////////////////////////////// 
             
      Ddep = (Degdep * Delaideg ) ;          // Ddep (degré avance dépression) en µs
      Dsum = D - Ddep - tcor ;               // tcor appliqué ici donne de meilleurs résultats niveau précision
                                             // if (deb > 1) {  pc(C1[j] );  pc(C2[j] );pc(D);}   
  if (Dsum < 6500) {                         // delay maxi à ne pas dépasser 16383 µs, risque de temps totalement incorrect
    delayMicroseconds(Dsum);                 // Attendre Dsum
  }
  else {
  milli_delay = ((Dsum/1000)-2);             // Quand D > 10ms car problèmes avec delayMicroseconds(D) si D>14ms!
  micro_delay = (Dsum-(milli_delay*1000));   // 
  delay(milli_delay);                        // Currently, the largest value that will produce an accurate delay is 16383 µs
  delayMicroseconds(micro_delay); 
  }                                                                                    
      break;                                 // break;  // Sortir, on a D
    }
  }
}

void  Etincelle ()   ////////// Coupure de la bobine pour créer l'étincelle d'allumage
{  

///////////////////////// Debug_Fonction //////////////////
   if ( Debug_Fonction) {
       Serial.println("Etin");
   }
//////////////////////////////////////////////////////// 
   
  digitalWrite(Bob1, 0);                     // Couper le courant, donc étincelle ************* ALLUMAGE ***************
  digitalWrite(Bob2, 0);                     // Couper le courant, donc étincelle 
  Stop_temps_Etincelle = micros();           // MM Calcul des temps  *************** CALCUL TEMPS AVANCE ****************

  digitalWrite(Flash, 1);                    // Flash stroboscope à leds
  delayMicroseconds(tFlash); 
  digitalWrite(Flash, 0);

//  Davant_TchargeBob =  T - TchargeBob;       // MM essai sans tenir compte des variations de régime moteur (peu de différence)

 if (T < (Tprec * 0.98) && T >= LimitMajorTchargBob && MajorTchargBob == 1)   // MM Si T est plus court (2 % au moins) et N inférieur à la limite majoration et majoration temps de charge bobine en service on modifie le temps de charge en phase d'accélération
 {
  TchargeBobMajor = TchargeBob * KmajorTchargBob;  // On augmente Temps charge bobine pour avoir de la réserve en cas d'accélération
 }
 else 
 {
  TchargeBobMajor = TchargeBob;                    // Sinon pas de majoration
 }

  Davant_TchargeBob =  T - TchargeBobMajor;        // MM essai sans tenir compte des variations de régime moteur (peu de différence), avec majoration temps de charge en accélération

///////////////////////// Debug_Davant_TchargeBob //////////////////
   if ( Debug_Davant_TchargeBob) {
       Serial.print(" ; Davant_TchargeBob = ");  Serial.println(Davant_TchargeBob);
   }
////////////////////////////////////////////////////////  

   if (N > Nkick && T < TgestBob)               // MM Si on n'est pas en démarrage kick et au dessus du seuil N gest bobine (1400tr/mn) on initialise Timer1 pour recharger bobine
   {
   Timer1.initialize(Davant_TchargeBob);        // Tempo Timer1 = Davant_TchargeBob calculé avant, attendre Davant_TchargeBob (temps de charge bobine) en µs avant de rétablir le courant dans la bobine
                                                // Timer1 au terme de ce temps va lancer isr-gestionIBob pour alimenter les bobines avant l'allumage (coupure des bobines)
   UneEtin = 1;                                 // Signaler une étincelle à l'isr_GestionIbob().
   }
  if (Multi && (T >= T_multi)) Genere_multi(); // Si on a multi étincelles et T supérieur à temps multi on génère multi éticelles
}

void  Genere_multi()                       ////////// Gérération de multi étincelles après allumage normal
{                                          // L'étincelle principale a juste été générée
  delayMicroseconds(1000);                 // Attendre fin d'étincelle T en µs
  N_Etin = 0;                              // Remise à 0 compteur étincelles multiples
  
  while (N_Etin < EtinMulti)               // Tant que le seuil du nombre d'étincelles secondaires n'est pas atteint :
  {
  digitalWrite(Bob1, 1);                   // Rétablir le courant
  digitalWrite(Bob2, 1);                   // Rétablir le courant  
  delayMicroseconds(TchargeBob * CoefTchargeBobMulti);  // Recharger en un temps diminué = temps normal de charge bobine x coefficient, en µs
  digitalWrite(Bob1, 0);                   // Etincelle secondaire
  digitalWrite(Bob2, 0);                   // Etincelle secondaire 
  delayMicroseconds(200);                  // Attendre fin d'étincelle T en µs
  N_Etin ++;                               // Augmenter le comptage du nombre d'étincelles
  }
}

void  isr_GestionIbob()         ////////// Gestion du moment d'alimentation de la bobine
{

///////////////////////// Debug_Fonction //////////////////
   if ( Debug_Fonction) {
       Serial.println("IsrB");
   }
//////////////////////////////////////////////////////// 
  
  Timer1.stop();             // Arrêter le décompte du timer1
  if (UneEtin == 1 && T > Tlim)  // Si il y a eu une étincelle (fonction Etincelle) MM essai: ET régime moteur inférieur à régime maxi
  {    
  AlimBobine();              // Fonction alimentation des bobines
  }
  else                       // Sinon 
  {
  digitalWrite(Bob1, 0);     // Préserver la bobine, couper le courant
  digitalWrite(Bob2, 0);     // Préserver la bobine, couper le courant  
  }
}

void AlimBobine()              /////////////// Alimentation des bobines pour prochain allumage
{

///////////////////////// Debug_Fonction //////////////////
   if ( Debug_Fonction) {
       Serial.println("AlimB");
   }
//////////////////////////////////////////////////////// 
  
  Timer1.stop();                // Arrêter le décompte du timer1 
  digitalWrite(Bob1, 1);        // Rétablir le courant dans bobine
  digitalWrite(Bob2, 1);        // Rétablir le courant dans bobine
  UneEtin = 0;                  // Remet le détecteur d'étincelle à 0
  Timer1.initialize(DsecuBob);  // Au cas où le moteur s'arrête, couper l'alimentation de la bobine, après DsecuBob µs, pour ne pas la surchauffer  
}

void  Select_Courbe_Avance()       //////////// Sélection courbe d'avance "centrifuge" en début de programme
{                                               // Par défaut, la courbe "a" est déja sélectionnée
 valSelecAv = analogRead(SelecAv);
  Serial.print("Selecteur AV = "); Serial.print(valSelecAv); Serial.print(" ,Centrifuge = ");
  if (valSelecAv < 80 || valSelecAv > 880) {   // Shunt 0 ohm donne 15 ou pas de shunt ou pas de résistance donne 1015
    Serial.println("Courbe a");
  }
  if (valSelecAv > 80 && valSelecAv < 230) {   // Résistance de 4K7 donne 130 
    pN = &Nb[0];                               // pointer à la courbe b
    pA = &Angb[0];
    Serial.println("Courbe b");
  }
  if (valSelecAv > 230 && valSelecAv < 450) {  // Résistance de 18K (ou 22k) donne 340    
    pN = &Nc[0];                               // pointer à la courbe c
    pA = &Angc[0];
    Serial.println("Courbe c");
  }
  if (valSelecAv > 450 && valSelecAv < 650) {  // Résistance de 47K donne 565    
    pN = &Nd[0];                               // pointer à la courbe d
    pA = &Angd[0];
    Serial.println("Courbe d");
  }
  if (valSelecAv > 650 && valSelecAv < 880) {  // Résistance de 100K donne 735    
    pN = &Ne[0];                               // pointer à la courbe e
    pA = &Ange[0];
    Serial.println("Courbe e");
  }
}


void  Select_Courbe_Depression()       //////////// Sélection courbe d'avance par dépression fonction de la charge en début de programme
{
 valSelecDep = analogRead(SelecDep);
  Serial.print("Selecteur Dep = "); Serial.print(valSelecDep); Serial.print(" ,Depression = ");
  if (valSelecDep < 80 || valSelecDep > 880) {  // Shunt 0 ohm donne 15 ou pas de shunt ou pas de résistance donne 1015
    xhigh = 330;                         // soit 210 mmHg
    xlow = 565;                          // soit 80 mmHg
    yhigh = 150;                         // soit 15°
    ylow = 0;                            // soit 0°
    Serial.println("Courbe a");
  }
  if (valSelecDep > 80 && valSelecDep < 230) {  // Résistance de 4K7 donne 130 
    xhigh = 350;                         // soit 200 mmHg
    xlow = 565;                          // soit 80 mmHg
    yhigh = 140;                         // soit 14°
    ylow = 0;                            // soit 0°
    Serial.println("Courbe b");
  }
  if (valSelecDep > 230 && valSelecDep < 450) {  // Résistance de 18K (ou 22k) donne 340    
    xhigh = 350;                         // soit 200 mmHg
    xlow = 601;                          // soit 60 mmHg
    yhigh = 160;                         // soit 16°
    ylow = 0;                            // soit 0°
    Serial.println("Courbe c");
  }
  if (valSelecDep > 450 && valSelecDep < 650) {  // Résistance de 47K donne 565    
    xhigh = 350;                         // soit 200 mmHg
    xlow = 565;                          // soit 80 mmHg
    yhigh = 160;                         // soit 16°
    ylow = 0;                            // soit 0°
    Serial.println("Courbe d");
  }
  if (valSelecDep > 650 && valSelecDep < 880) {  // Résistance de 100K donne 735    
    xhigh = 350;                         // soit 200 mmHg
    xlow = 601;                          // soit 60 mmHg
    yhigh = 140;                         // soit 14°
    ylow = 0;                            // soit 0°
    Serial.println("Courbe e");
  }
}


void  Correction_Avance()      //////////////////// MM Correction d'avance en fonctionnement par sélecteur                                                                                               
{                                                // MM Par défaut, correction d'avance à 0° est sélectionnée
  valCorAv = analogRead(CorAv);                  // MM Lire valeur analogique de la tension du sélecteur de correction d'avance en entrée

    Serial.print("Cor ");
  if (valCorAv < 80 || valCorAv > 880) {                     // Shunt 0 ohm donne 15 ou pas de shunt ou pas de résistance donne 1015 et plus
    modC1 = 0;                                               // MM Shunt 0 ohm donne 15 correction = 0 
    Serial.print("+ 0");                                     // correction 0
  }
  if (valCorAv > 80 && valCorAv < 230) {                     // Résistance de 4K7 donne 130 
    modC1 = float (PasCorecAv) / float(AngleCibles);         // correction 1 fois le Pas de correction d'avance
    Serial.print("+ ");
    Serial.print(PasCorecAv);                             
  }
  if (valCorAv > 230 && valCorAv < 450) {                    // Résistance de 18K (ou 22k) donne 340    
    modC1 = 2 * float (PasCorecAv) / float(AngleCibles);     // correction 2 fois le Pas de correction d'avance
    Serial.print("+ ");
    Serial.print((PasCorecAv)*2);                        
  }
  if (valCorAv > 450 && valCorAv < 650) {                    // Résistance de 47K donne 565    
    modC1 = 3 * float (PasCorecAv) / float(AngleCibles);     // correction 3 fois le Pas de correction d'avance                            
    Serial.print("+ ");
    Serial.print((PasCorecAv)*3);                         
  }
  if (valCorAv > 650 && valCorAv < 880) {                    // Résistance de 100K donne 735    
    modC1 = 4 * float (PasCorecAv) / float(AngleCibles);     // correction 4 fois le Pas de correction d'avance
    Serial.print("+ ");
    Serial.print((PasCorecAv)*4);                         
  }
 }


void  LectUbat()       /////////////////////// Lecture de la tension batterie
{
   tension = analogRead(Vbatterie);    // calcul durée alimentation bobine, demande environ 120 µs supplémentaire
//   pc(tension)
   float Ubat = tension / 68.286;      // tension de 0 à 1023 vers U batterie de 0 à 15 V, utilisé aussi pour serial print U bat

///////////////// Lecture tension batterie sur port série
    fxUbat = Ubat * 10;        // U batterie est multiplié par 10 pour visualiser les chiffres après la virgule
}


void loop()    //////////////// Attendre les tops capteur pour générer l'allumage
{

 while (digitalRead(Cible) == !CaptOn);         // Attendre début de la cible     ***************** DEBUT CIBLE ******************

 if (N > 0)                                     // MM Si on n'est pas en phase de démarrage, le premier tour moteur, on mesure T et on autorise la gestion du temps charge bobine (sinon allumage intempestif au tour 1)
 {
 Timer1.attachInterrupt(isr_GestionIbob);       // IT d'overflow de Timer1 (16 bits), à l'échéance de Timer1 (interruption) gérér alimentation des bobines avant étincelle
 
 T = micros() - prec_H;                         // front actif arrivé calculer T  ************** CALCUL T ******************
 prec_H = micros();                             // heure du front actuel qui deviendra le front précédent
 }

 else { T = Tdem; }                             // MM sinon on est en phase de démarrage (1er tour) on fixe arbitrairement T à une valeur par exemple de 200 ms pour 300 tr/mn, pour permettre l'allumage aux premiers tours moteur

 LectUbat();                                    // Lecture de la tension batterie
  
 digitalWrite(Led,HIGH);                        // MM allumage LED de calage statique, quand la cible en métal est présente la LED s'allume

 if (T > TsecuKick) {N = 0;}                    // MM si T est supérieur à Temporisation sécurité Kick le moteur a calé il faut remettre N à 0 pour un nouveau démarrage
 N++;                                           // MM on augmente N à chaque passage de la cible
 if (N > Nkick +1) {N = Nkick +1;}              // MM si N est supérieur au seuil Nkick +1, inutile de l'augmenter sinon il repasse à 0 au bout d'un moment

///////////////////////// Debug_Tloop //////////////////
   if ( Debug_Tloop) {
       Serial.print(" ; Tloop = ");  Serial.println(T);
   }
//////////////////////////////////////////////////////// 

///////////////////////// Debug_Fonction //////////////////
   if ( Debug_Fonction) {
       Serial.println("Cibl=1");
   }
//////////////////////////////////////////////////////// 

  Dep = analogRead(Depression);                  // Lecture dépression en A0
  Degdep = map(Dep,xhigh,xlow,yhigh,ylow);       // Mesure la dépression
  Degdep = Degdep/10;
//   int Degdepcal = analogRead(Depression);     // Pour calibrage du capteur MVP5050 
                                                 
  if (Degdep < 0){ Degdep = 0; }                 // Si la dépression est inférieure à 0 alors elle = 0
    else if (Degdep > (yhigh/10)){ Degdep = (yhigh/10); }
    else ;

  if (Tprec >= TgestBob || N <= Nkick)           // MM Si le régime moteur (précédent) est sous le régime N_multi (1500tr/mn) OU N inférieur au seuil Nkick, alimentation bobines pour préparer un allumage
  {
   AlimBobine();                                 // Alimentation des bobines
  } 
    
  if (T <= Tplancher && T > Tlim && N > Nkick)   // Si le régime moteur est au dessus du régime plancher et sous le régime maxi (zone rouge) et N supérieur au seuil Nkick
  { 
   CalcD();                                      // Calcul du temps d'attente entre début de la cible et l'allumage
   Etincelle();                                  // Etincelle d'allumage
  }

  if (T <= Tlim)                                 // Si le régime moteur est égal ou supérieur au régime maxi (zone rouge)
  {
   digitalWrite(Bob1, 0);                        // Préserver la bobine, couper le courant immédiatement au régime maxi sans attendre Dsecu de 1 sec
   digitalWrite(Bob2, 0);                        // Préserver la bobine, couper le courant immédiatement au régime maxi sans attendre Dsecu de 1 sec
  }
   

  while (digitalRead(Cible) == CaptOn);          // Attendre fin de la cible au PMH à 0°  ******************* FIN DE CIBLE à 0° ********************

    ///////////////////////// Debug_Fonction //////////////////
   if ( Debug_Fonction) {
       Serial.println("Cibl=0");
   }
   ////////////////////////////////////////////////////////

  Tprec = T;                                     // Sauve la valeur de T en cas de perte d'info du capteur
 
  if (T > Tplancher || N <= Nkick)               // MM Si le régime moteur est sous le régime plancher OU N inférieur au seuil Nkick
  {
   Etincelle();                                  // Etincelle d'allumage (et multi-étincelles si à 1) pour le démarrage
  }   
  
  digitalWrite(Led,LOW);                         // MM extinction Led de calage statique
  Correction_Avance();                           // MM correction avance en fonctionnement


       //////////// MM allumage led témoin de charge batterie (led connectée au + 5V, allumée si sortie à LOW)
       
   if (tension < 897 || tension > 1003) { digitalWrite(LedRouge,LOW); digitalWrite(LedVerte,HIGH);}        // MM allumage led en rouge si tension batterie est inférieure à 13V ou supérieure à 14,7V
   else if (tension >= 897 && tension <= 956) { digitalWrite(LedRouge,LOW); digitalWrite(LedVerte,LOW); }  // MM led jaune si tension entre 13 et 14 V
   else { digitalWrite(LedVerte,LOW); digitalWrite(LedRouge,HIGH);}                                        // MM sinon led verte (tension entre 14 et 14,7 V)
       
  
       /////////// MM Informations envoyées vers le port série pour lecture du régime moteur, de l'avance à l'allumage.

///////////////////////// Debug_printMM //////////////////
 if (Debug_printMM) 
 {
  Regime = (NT / T) * 1.001;                                // MM calcul du régime moteur, avec correction d'erreur 0.001
  Delaideg = NT / Regime / float(AngleCibles);              // MM calcul du délai en degrés   
  float Tempsecoule = Stop_temps_Etincelle - prec_H;        // MM calcul du temps entre étincelle et le top capteur, type float pour avoir chiffre après la virgule et afficher avance négative sous Ndem
  AvanceMoteur = (AngleCapteur - (Tempsecoule / Delaideg)) * 1.004;   // MM calcul avance totale à l'allumage du moteur, avance moteur doit être du type float, correction x 1.004
  fxAvanceMoteur = 10 * AvanceMoteur;                       // MM pour afficher l'avance avec un chiffre après virgule fixe, optimisation par rapport à virgule flottante
  if (fxAvanceMoteur > 0)
  {fxAvanceMoteur2 = fxAvanceMoteur%10;}
  else {fxAvanceMoteur2 = -(fxAvanceMoteur%10);
  }
  
  Serial.print(" ; Ubat = ");      // MM tension batterie
  Serial.print(fxUbat/10);         // MM tension batterie partie entière avant virgule fixe
  Serial.print(",");               // MM virgule fixe
  Serial.print(fxUbat%10);         // MM modulo partie après la virgule fixe
  Serial.print(" ; N = ");         // MM régime moteur N  
  Serial.print(Regime);            // MM régime moteur N
  Serial.print(" ; Av =  ");       // MM avance
  if (AvanceMoteur < -360 || AvanceMoteur > 360) 
  {Serial.println("All HS");}      // MM si l'avance est hors limite afficher : All HS
  else {
  Serial.print(fxAvanceMoteur/10); // MM partie entière de l'avance
  Serial.print(",");               // MM virgule fixe
  Serial.println(fxAvanceMoteur2); // MM modulo partie après la virgule fixe
  }    
 }
///////////////////////////////////////////////////////// 
}


//////////////// Informations diverses //////////////////

// Régimes moteur et fréquences correspondantes

          // Hertz = Nt/mn / 30 , pour moteur 4 cylindres
          // N 1000,1500,2000,2500,3000,3500,4000,4500,5000,5500,6000,6500,7000,7500,8000,8500,9000,9500,10000
          // Hz 33,  50,  66,  83  100  117  133  150  166  183  200  216  233  250  266  283  300  316   333

          // Hertz = Nt/mn / 60 , pour moteur 2 cylindres
          // N 1000,1500,2000,2500,3000,3500,4000,4500,5000,5500,6000,6500,7000,7500,8000,8500,9000,9500,10000
          // Hz 16,  25,  33,  41   50   58   66   75   83   91  100  108  116  125  133  141  150  158   166
          
// Type de capteur Hall possible :

          // Capteur Honeywell 1GT101DC, contient un aimant sur le coté, type non saturé, sortie haute à vide,
          // et basse avec une cible en métal. Il faut  CapteurOn = 0, déclenche sur front descendant.
          
          // Le capteur à fourche SR 17-J6 contient un aimant en face, type saturé, sortie basse à vide,
          // et haute avec une cible métallique. Il faut  CapteurOn = 1, déclenche sur front montant.

