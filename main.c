/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include <stdio.h>

#define and &&
#define or ||

#define variable Variables_1.Variable_1

/*** Variables ***/
char ADC_en_ASCII[5];                                   // Variable para transmision de la señal Pletismografica por UART
uint16 vector_periodo_pletismografico[5] = {0,0,0,0,0}; // Vector de tiempo para el calculo de FC

typedef struct Tiempo{                                                     
    uint16 ms_1:10;                                     // Tiempo para el ADC y el UART
    uint16 ms_2:10;                                     // Tiempo para el pulso cardiaco
}Tiempo;
Tiempo tiempo;

typedef struct Medidas{        
    uint32 acumulado_ADC:20;
    uint32 ADC:16;    
    uint32 acumulado_periodo_pletismografico:20;
    uint32 periodo_pletismografico:12;
    uint32 frecuencia_cardiaca:12;
    uint32 BPM:8;
}medidas;
medidas medida;

typedef union Banderas_1{                                                      
    struct Variables1{        
        uint16 contador:5;
        uint16 pulso_detectado:1;
        uint16 i:3;
    }Variable_1;
}banderas_1;
banderas_1 Variables_1;

unsigned char dato;

/*** Funciones ***/
void sensar(void);
void enviar_por_serial(void);
void calcular_FC_y_BPM(void);
void imprimir(void);

/*** Interrupciones ***/
CY_ISR_PROTO(cronometro);
CY_ISR_PROTO(pulso_cardiaco);

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    isr_contador_StartEx(cronometro);
    isr_comparador_StartEx(pulso_cardiaco);
    
    /* Inicializacion de Componentes */
    Contador_Start();
    LCD_Start();
    ADC_Start();
    UART_Start();
    
    Seguidor_1_Start();
    Seguidor_2_Start();
    Comparador_Start();
    PGA_Start();

    for(;;)
    {
        if (tiempo.ms_1%5 == 0) sensar();               // Sensa 200 veces por segundo
        
        if (variable.pulso_detectado == 1){
            variable.pulso_detectado = 0;
            calcular_FC_y_BPM();
            imprimir();
        }
    }
}

void imprimir(void){                                    
    LCD_ClearDisplay();
    
    LCD_Position(0,0);                                  // Variables mostradas en el LCD
    LCD_PrintString("FC: ");
    LCD_PrintNumber(medida.frecuencia_cardiaca);
                                 
    LCD_PrintString(" BPM: ");
    LCD_PrintNumber(medida.BPM);
}

void sensar(void){
    variable.contador++;                                                             
    
    ADC_StartConvert();                                 // Rutina de Conversion
    ADC_IsEndConversion(ADC_WAIT_FOR_RESULT);
    medida.acumulado_ADC += ADC_GetResult16();
    
    if (variable.contador == 10){                       // Rutina de Promedio                                 
        variable.contador = 0;
        
        medida.ADC = medida.acumulado_ADC /= 10;        // Una vez obtenida la lectura promedio del ADC
        enviar_por_serial();                            // se envia al monitor serial para su visualizacion
        
        medida.acumulado_ADC = 0;     
    }
}

void enviar_por_serial(void){
    sprintf(ADC_en_ASCII, "%u", medida.ADC);            // Conversión de número a ASCII: 
                                                        // ADC_en_ASCII <--  str(variable.ADC)
    
    UART_PutString(ADC_en_ASCII);                       // Envia el valor de ADC por Comunicacion Serial
    UART_PutCRLF(' ');    
}

void calcular_FC_y_BPM(void){
    for (uint8 j = 0; j < 5; j++){                      // Se Suman los 5 valores del vector
        medida.acumulado_periodo_pletismografico += vector_periodo_pletismografico[j];
    }
    medida.periodo_pletismografico /= 5;                // Se promedian
    
    if (medida.periodo_pletismografico > 100){          // Sí se supera un umbral de promedios
        medida.frecuencia_cardiaca = medida.periodo_pletismografico; 
        medida.BPM = medida.frecuencia_cardiaca*60;     // se calculan FC y BPM
    }
    else {                                              // de lo contrario no se toma en cuenta
        medida.frecuencia_cardiaca = 0;                 // la medida
        medida.BPM = 0;
    }
}

CY_ISR(pulso_cardiaco){
    // [a _ _ _ _], [a b _ _ _] ... [a b c d e]  Al llenar el vector se reemplazan las muestras en orden    
    // [f b c d e], [f g c d e] ... [f g h i j]  y de esta forma el vector siempre tendra las 5 mas recientes
    
    vector_periodo_pletismografico[variable.i] = tiempo.ms_2;
    tiempo.ms_2 = 0;                                    // Al tener que reiniciar el cronometro
                                                        // debe usarse una variable diferente o 
    variable.i++;                                       // afectara la lectura del ADC
    if (variable.i == 5) variable.i = 0;
    variable.pulso_detectado = 1;                       // Se activa una bandera para calcular FC y BPM
}

CY_ISR(cronometro){                                     
    tiempo.ms_1++;      
    if (tiempo.ms_1 == 1000) tiempo.ms_1 = 0;           // Cronometro del ADC y el UART: ms_1
    
    tiempo.ms_2++;      
    if (tiempo.ms_2 == 1000) tiempo.ms_2 = 0;           // Cronometro del Pulso Cardiaco: ms_2
}
/* [] END OF FILE */
