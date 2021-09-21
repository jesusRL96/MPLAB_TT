/*
 * File:   Codigo_TT2_01.c
 * Author: Jesus
 *
 * Created on 27 de junio de 2019, 09:44 PM
 */
// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF        // Internal External Switchover bit (Internal/External Switchover mode is enabled)
#pragma config FCMEN = OFF       // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is enabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)
// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)
// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.
// Definiciones de constantes en librerias
#define _XTAL_FREQ 8000000
#define RS RD2
#define EN RD3
#define D4 RD4
#define D5 RD5
#define D6 RD6
#define D7 RD7
// Librerias
#include <xc.h>
#include <stdlib.h>
#include "lcd.h"
// Definicion de pines
#define Celda   PORTAbits.RA0       // Celda de carga
#define Boton1  PORTAbits.RA1       // Inicia proceso de recepcion de envaces
#define Boton2  PORTBbits.RB7       // Boton para dispensar barritas   
#define Boton3  PORTDbits.RD0       // Boton para dispensar arroz
#define Boton4  PORTDbits.RD1       // Boton para finalizar proceso
#define capacitivo PORTCbits.RC3    // Entrada del sensor capacitivo
// Definicion de retardos para motores
#define tiempo1 20
#define tiempo2 75
#define tiempo3 45       //100
// Parametros para aceptacion PET
#define pesoMin_p 287
#define pesoMax_p 304 
#define alturaMin_p 30
#define alturaMax_p 80  
// Parametros para aceptacion aluminio
#define pesoMin_a 287
#define pesoMax_a 304 
#define alturaMin_a 90
#define alturaMax_a 194 
// Definicion de variables globales
// __bit pet;           // Bit para reconocimiento de material.
float creditos=0;         // Variable de numero de creditos.
int altura, peso;       // Parametros de reconocimiento
int creditos_temp2;
int creditos_temp = 0;
unsigned char dato_c[20]; 
// Definicion de funciones del proceso
void ConfiguracionInicial(void); // Configuracion de los bits
// void EstadoInicial(void);        // Posicion inicial de los actuadores y configuracion inicial de los pines
void Reconocimiento(void);       // Reconocimiento del material
void DispensarBarritas(void);
void DispensarArroz(void);
// Definicion de funciones
void Servo(int angulo);
void IngresarEnvase();
int MotorPasos(int motor_p,int pasos,int sentido,int tipo, int num, int retardo);
int MotorPasos_sensor(int motor_p, int pasos);
int Medir(void);
int Pesar(void);
void Desactiva(int motor);
void MostrarCreditos(void);
void CreditosInsuf(void);
void Menu1(void);
// Interrupcion para servomotor
int angulo = 0, alto_bajo = 0, parpadeo = 0, alerta = 0, auxiliar = 0;     // variables para el control dl angulo en un servomotor
void __interrupt(high_priority) timer2(void){
    TMR2IF=0;
    if(angulo == 45 && alto_bajo == 1) {
        RC2 = 1;
        PR2 = 5;        //4
        alto_bajo = 0;
    }
    else if(angulo == 45 && alto_bajo == 0) {
        RC2 = 0;
        PR2 = 151;      //152
        alto_bajo = 1;
    }
    else if(angulo == 0 && alto_bajo == 1) {
        RC2 = 1;
        PR2 = 7;
        alto_bajo = 0;
    }
    else{               // (angulo == 0 && alto_bajo == 0) 
        RC2 = 0;
        PR2 = 149;
        alto_bajo = 1;
    }
    parpadeo++;
    if((parpadeo >= 25) && (alerta)){
        RC0 = (~RC0);
        parpadeo = 0;
    }
    if(Boton1)  auxiliar =1;
}
// ********************************** INICIO ****************************************************
void main(void) {
    // Definicion de variables locales
    int motorRecepcion, motorBarritas, motorArroz, motor_temp, pasos; 
    int pet = 1, peso_temp, aceptado;
    // int umbral = 5; //umbral para reconocimiento
    // habilitacion de interrupciones para servomotor
    T2CONbits.T2CKPS = 0b11;
    T2CONbits.TOUTPS = 0b1111;    
    T2CONbits.TMR2ON = 1;
    PIR1bits.TMR2IF = 0;
    PIE1bits.TMR2IE = 1;
    PEIE = 1;
    GIE = 1;
    // configuracion de pines
    ConfiguracionInicial();
    Lcd_Init();
    while(1){
        Inicio:
        // Estado inicial
        motorRecepcion = 0; // motor3 -> motor mecanismo de recepcion
        motorBarritas = 0;  // motor1 -> motor mecanismo de barritas
        motorArroz = 0;     // motor2 -> motor mecanismo de arroz
        angulo = 45;
        Desactiva(1);
        Desactiva(2);
        Desactiva(3);
        RC0 = 1;
        RC1 = 1;
        auxiliar = 0;
        if(creditos == 0) Menu1();
        else {
            // Opciones
            Lcd_Clear();
            Lcd_Set_Cursor(1,1);            
            Lcd_Write_String("Elija una opcion: ");
            Lcd_Set_Cursor(2,2);   
            MostrarCreditos();
            Opciones:
            RC0 = 1;
            RC1 = 1;
            if (Boton1) {
                __delay_ms(25);
                while(Boton1);
                __delay_ms(25);
                goto Proceso;
            }
            else if(Boton2) {
                __delay_ms(25);
                while(Boton2);
                __delay_ms(25);
                // Dispensar barritas
                creditos_temp = (int) creditos;
                if(creditos >= 2) {
                    motor_temp = MotorPasos(motorBarritas, 203, 1, 1, 1, 2);     // 1 vuelta completa (200 pasos)
                    motorBarritas = motor_temp;            
                    Desactiva(1);                                  // motor 1 inactivo
                    creditos -= 2;
                    Lcd_Clear();
                    Lcd_Set_Cursor(1,1); 
                    MostrarCreditos();
                    __delay_ms(2000);
                }
                else CreditosInsuf();
                goto Opciones;
            }
            else if(Boton3) {
                __delay_ms(25);
                while(Boton3);
                __delay_ms(25);
                // Dispensar arroz
                if(creditos >= 1){
                    motor_temp = MotorPasos(motorArroz, 64, 1, 1, 2, 1);
                    motorArroz = motor_temp;            
                    __delay_ms(150);
                    motor_temp = MotorPasos(motorArroz, 64, 0, 1, 2, 1);
                    motorArroz = motor_temp;
                    Desactiva(2);                                            // motor 2 inactivo 
                    creditos -= 1;
                    Lcd_Clear();
                    Lcd_Set_Cursor(1,1); 
                    MostrarCreditos();
                    __delay_ms(2000);
                }            
                else CreditosInsuf();
                goto Opciones;
            }
            else if(Boton4) {
                __delay_ms(25);
                while(Boton4);
                __delay_ms(25);
                //RESET();
                creditos = 0;   
                goto Inicio;
            }
            else goto Opciones;
        } 
        Lcd_Clear();
        Lcd_Set_Cursor(1,1);
        Lcd_Write_String("Espere...");
        Proceso:        
        // RECONOCIMIENTO Y CLASIFICACION
        motor_temp = MotorPasos_sensor(motorRecepcion, 28);     // 45° (lectura de altura)
        motorRecepcion = motor_temp;
        //Desactiva(3);                                           // motor 3 inactivo
        __delay_ms(1500);
        peso = 0;
        peso_temp = 0;
        for(int c = 0; c<= 25; c++) {                          // Lectura de peso maximo
            peso_temp = Pesar();                                // Valor en binario
            if(peso_temp >= peso) peso = peso_temp;
            //__delay_ms(10);
        }
        // Movimiento de servomotor para clasificacion
        if (capacitivo && (altura > 88)) {   
            aceptado = ((peso > pesoMin_a) && (peso < pesoMax_a)) && ((altura > alturaMin_a) && (altura < alturaMax_a));  // Exprecion logica de aceptacion
            angulo = 45;                        // Servo a 45°    
            pet = 0;
        }
        else if(capacitivo && (altura >= 76) && (altura < 88)) {
            aceptado = ((peso > pesoMin_p) && (peso < pesoMax_p)) && ((altura > alturaMin_p) && (altura < alturaMax_p));  // Exprecion logica de aceptacion
            if (aceptado) angulo = 0;                         // Servo a 0°  
            pet = 1;
        }
        else {
            aceptado = ((peso > pesoMin_p) && (peso < pesoMax_p)) && ((altura > alturaMin_p) && (altura < alturaMax_p));  // Exprecion logica de aceptacion
            if(aceptado) angulo = 0;                         // Servo a 0°  
            pet = 1;
        } 
        // Pruebas
//        Lcd_Clear();
//        Lcd_Set_Cursor(1,1);
//        itoa(dato_c,altura,10);
//        Lcd_Write_String("altura: ");
//        Lcd_Write_String(dato_c);
//        Lcd_Set_Cursor(2,2);
//        itoa(dato_c,peso,10);
//        Lcd_Write_String("peso: ");
//        Lcd_Write_String(dato_c);
//        __delay_ms(1000);
        // ACEPTAR O RECHAZAR 
        
        //aceptado = (pesoMin < peso < pesoMax) && (alturaMin < altura < alturaMax); 
        if(aceptado) {
            // Calculo de creditos     
            if(pet) creditos += 0.2;            // .2
            else    creditos += 0.335;          // .335
            Lcd_Clear();
            Lcd_Set_Cursor(1,1);
            MostrarCreditos();
            // Ingresar a la maquina
            RC1 = 0;
            motor_temp = MotorPasos(motorRecepcion, 50, 0, 1, 3, 3);
            motorRecepcion = motor_temp;
            //Desactiva(3);                                            // motor 3 inactivo
            __delay_ms(700);
            angulo = 45;
            motor_temp = MotorPasos(motorRecepcion, 78, 1, 1, 3, 3);
            motorRecepcion = motor_temp;
            Desactiva(3);                                               // motor 3 inactivo
            RC1 = 1;
        }
        else {
            Lcd_Clear();
            Lcd_Set_Cursor(1,1);            
            Lcd_Write_String("rechazado: ");
            alerta = 1;
            motor_temp = MotorPasos(motorRecepcion, 28, 1, 1, 3, 2);
            motorRecepcion = motor_temp;
            Desactiva(3);                                       // motor 3 inactivo
            __delay_ms(1000);
            alerta = 0;
            RC0 = 1;
        }   
        angulo = 45; 
        
    }    
    return;
}
// *************************** Implementacion de funciones ***********************
void ConfiguracionInicial(){
    OSCCONbits.IRCF = 0b111;  // Oscilador interno 8MHz
    TRISA = 0x0F;   // Pin RA0,RA1, RA2 y RA3 como entradas (Celda de carga/ Boton inicio)
    TRISB = 0x40;   // Trigger = RB5, echo = RB6 (01000000)
    TRISC = 0x08;   // Salidas (motores a pasos)
    TRISD = 0x03;   // Pin RD0 y RD1 como entradas (Boton arroz/boton finalizar)
    TRISE = 0x00;   // Todos los pines del puerto configurados como salidas
    ANSEL = 0x01;   // Pin RA0 como entrada analogica (celda)
    ANSELH = 0x00;
    PORTA = 0x00;
    PORTB = 0x00;
    PORTC = 0x00;
    PORTE = 0x00;
    ADCON1 = 0b10010000;    // Referencia positiva en RA3
    ADCON0 = 0b11000001;
    ADCON0bits.CHS = 0; 
    return;
}

// Funciones Auxiliares

int MotorPasos(int motor_p,int pasos,int sentido,int tipo, int num, int retardo) {
    int paso, n;
    n = motor_p;
    for(int i = 0; i < pasos;i++){                
        if (sentido == 1){
            if(n >= 4) n = 1;
            else n += 1;
        }
        else{
            if(n <= 1) n = 4;
            else n -= 1;
        }        
        switch(n){
            case 1:
                if (tipo == 1) paso = 0b1010;
                else paso = 0b0001;
                break;
            case 2:
                if (tipo == 1) paso = 0b1001;
                else paso = 0b0010;
                break;
            case 3:
                if (tipo == 1) paso = 0b0101;
                else paso = 0b0100;
                break;
            default:
                if (tipo == 1) paso = 0b0110;
                else paso = 0b1000;                                      
        }
        if(num == 1) {
            //PORTA &= (0 << 4) & (0 << 5) & (0 << 6) & (0 << 7);
            PORTA &= 0x0F;
            PORTA |= (paso << 4);
        }
        else if(num == 2) {
            //PORTB &= (0 << 0) & (0 << 1) & (0 << 2) & (0 << 3);
            PORTB &= 0xF0;
            PORTB |= paso;
        }
        else {
            //PORTC &= (0 << 4) & (0 << 5) & (0 << 6) & (0 << 7);
            PORTC &= 0x0F;
            PORTC |= (paso << 4);
        }
        // PORTA = paso;
        if(retardo == 1) {
            __delay_ms(tiempo1); 
        }
        else if(retardo == 2) {
            __delay_ms(tiempo2); 
        }
        else {
            __delay_ms(tiempo3); 
        }       
    }
    return n;
}
int MotorPasos_sensor(int motor_p,int pasos) {
    int paso, n, medida = 400, medida_temp = 0, sentido, tipo;
    n = motor_p;
    sentido = 0;
    tipo = 1;
    for(int i = 0; i < pasos;i++){  
        if (sentido == 1){
            if(n >= 4) n = 1;
            else n += 1;
        }
        else{
            if(n <= 1) n = 4;
            else n -= 1;
        }        
        switch(n){
            case 1:
                if (tipo == 1) paso = 0b1010;
                else paso = 0b0001;
                break;
            case 2:
                if (tipo == 1) paso = 0b1001;
                else paso = 0b0010;
                break;
            case 3:
                if (tipo == 1) paso = 0b0101;
                else paso = 0b0100;
                break;
            default:
                if (tipo == 1) paso = 0b0110;
                else paso = 0b1000;                                      
        }
//        PORTA &= (0 << 4) & (0 << 5) & (0 << 6) & (0 << 7);
//        PORTA |= (paso << 4);
        //PORTC &= (0 << 4) & (0 << 5) & (0 << 6) & (0 << 7);
        PORTC &= 0x0F;
        PORTC |= (paso << 4);
        //RC1 = 1;
        //RC0 = 1;
        // Mapeo
        medida_temp = Medir();
        if(medida_temp < medida) medida = medida_temp;
        __delay_ms(tiempo3);        
    }
    altura = medida;
    return n;
}
int Medir() {
    int medida = 0; 
    char dato_c[5];
    RB5 = 0;
    RB6 = 0;
    T1CON = 0x10;       // Inicializar el modulo de Timer 1
    TMR1H = 0;                  
    TMR1L = 0;
    RB5 = 1;
    __delay_us(10);
    RB5 = 0;
    while(!RB6);
    TMR1ON = 1;               
    while(RB6);
    TMR1ON = 0;
    medida = (TMR1L | (TMR1H<<8));
    medida = medida/5.882;   // Medida en mm
    //medida += 10;            // Calibracion  
    medida = (int) medida;
    // itoa(dato_c, medida, 10);
    return medida;
}
int Pesar() {
    int adc;
    float volts;
    // Inicio de conversion
    ADCON0bits.GO_nDONE = 1;         
    while(ADCON0bits.GO_nDONE == 1);
    adc = ADRESH;
    adc = adc << 8;
    adc = adc | ADRESL;
    // volts = (adc * 5.0) /1023.0;
    return adc;
}

void Desactiva(int motor){    
    if(motor == 1)      PORTA &= 0x0F;
    else if(motor == 2) PORTB &= 0xF0;
    else                PORTC &= 0x0F;         
}

void MostrarCreditos(void) {
    Lcd_Write_String("creditos: ");         // Imprimir creditos
    creditos_temp = (int) creditos;
    itoa(dato_c, creditos_temp, 10);
    Lcd_Write_String(dato_c);
    creditos_temp = creditos * 100;
    creditos_temp = (int) creditos_temp;            
    if(creditos_temp >= 100) {
        creditos_temp2 = (int) creditos;
        creditos_temp2 *= 100;
        creditos_temp -= creditos_temp2;
    }
    Lcd_Write_String(".");
    if(creditos_temp < 10) Lcd_Write_String("0");        
    itoa(dato_c, creditos_temp, 10);
    
    Lcd_Write_String(dato_c);   
    return;
}

void CreditosInsuf(void){
    alerta = 1;
    Lcd_Clear();
    Lcd_Set_Cursor(1,4);            
    Lcd_Write_String("Creditos");
    Lcd_Set_Cursor(2,1); 
    Lcd_Write_String("insuficientes");
    __delay_ms(2000);
    alerta = 0;
    RC0 = 0;
    Lcd_Clear();
    Lcd_Set_Cursor(1,2);            
    Lcd_Write_String("Ingrese mas");
    Lcd_Set_Cursor(2,4); 
    Lcd_Write_String("envases");
    __delay_ms(2000);
}

void Menu1() {
    while(auxiliar == 0) {     // RA1 como boton de ingreso de recipiente 
            if(auxiliar == 0){
                Lcd_Clear();
                Lcd_Set_Cursor(1,3);
                Lcd_Write_String("Bienvenido");
                __delay_ms(1000);
                Lcd_Clear();
                Lcd_Set_Cursor(1,3);
                Lcd_Write_String("Ingrese Un");
                Lcd_Set_Cursor(2,5);
                Lcd_Write_String("Envase");
                __delay_ms(1000);
            }
            else {
                Lcd_Clear();
                Lcd_Set_Cursor(1,1);
                // Lcd_Write_String("Creditos: ");
                MostrarCreditos();
                Lcd_Set_Cursor(2,1);
                Lcd_Write_String("Elija una opción: ");
            }            
        }
    __delay_ms(25);
    while(Boton1);
    __delay_ms(25); 
    return ;
}