#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "ss_oled.h"
#include "hardware/i2c.h"
#include "servo.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "ultrasonic.h"
#include "hardware/timer.h"
#include "hardware/adc.h"


// RPI Pico
#define SDA_PIN 16
#define SCL_PIN 17
#define RESET_PIN -1
#define SDA_PIN_RTC 18
#define SCL_PIN_RTC 19
#define I2C_PORT i2c1
#define DS3231_ADDRESS 0x68
#define PUMP_PIN 28


// Botones
#define UP_PIN 2
#define DOWN_PIN 3
#define LEFT_PIN 4
#define RIGHT_PIN 5
#define OK_PIN 6
#define BACK_PIN 7

// US SENSOR
#define TRIG_PIN 15
#define ECHO_PIN 14

// IR SENSOR
#define IR_SENSOR_PIN 8

// WATER LEVEL SENSOR
#define WATER_SENSOR_PIN 26  // ADC0 is GP26

// Constants for servo control
const int servoPin = 0;
const float minPulseWidth = 1.0f;   // Pulse width for 0 degrees (in ms)
const float maxPulseWidth = 2.0f;   // Pulse width for 180 degrees (in ms)
const float initialPositionDegrees = 25.0f;  // Initial position (0 degrees)
const float finalPositionDegrees = 90.0f;   // Final position (45 degrees)

float clockDiv = 64.0f;
float wrap = 39062.0f;

int rc;
SSOLED oled;
static uint8_t ucBuffer[1024];
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// Variables de estado
enum State { STARTUP, WELCOME, DATE_CONFIG, TIME_CONFIG, MODE_SELECT, MANUAL_MODE, AUTOMATIC_MODE, MONITORING, FOOD_TIME_CONFIG, WATER_TIME_CONFIG } state;
enum State prev_state;

// Variables globales
int current_option = 0;
int selected_section = 0;  // Cambiado de selected_digit
int day = 1, month = 1, year = 2020;  // Valores iniciales para la fecha
int hour = 0, minute = 0, second = 0;  // Valores iniciales para la hora
int hour_2f = 0, minute_2f = 0, second_2f = 0;  // Valores iniciales para la hora
int hour_2w = 0, minute_2w = 0, second_2w = 0;  // Valores iniciales para la hora
int hour_2f_base = 0, minute_2f_base = 0, second_2f_base = 0;  // Valores iniciales para la hora
int hour_2w_base = 0, minute_2w_base = 0, second_2w_base = 0;  // Valores iniciales para la hora

int food_count = 0; int water_count = 0; //Conteo de veces que se alimentó o dió agua a la mascota.

uint8_t rtc_second, rtc_minute, rtc_hour;
uint8_t rtc_day, rtc_month, rtc_year;

// Funciones del menú
void init_oled();
void init_buttons();
void init_rtc();
void ds3231_read_time(uint8_t *rtc_second, uint8_t *rtc_minute, uint8_t *rtc_hour);
void ds3231_set_time(int hour, int minute, int second);
void ds3231_read_date(uint8_t *rtc_day, uint8_t *rtc_month, uint8_t *rtc_year);
void ds3231_set_date(int day, int month, int year);
void display_startup();
void display_welcome();
void configure_date();
void configure_time();
void select_mode();
void manual_mode();
void automatic_mode();
void monitoring();
void handle_input();
void increment_section();
void decrement_section();
void move_left();
void move_right();
void display_date();
void display_time();
void display_real_time();
void move_servo();
float degreesToMillis(float degrees);
void setMillis(int servoPin, float millis);
void setServo(int servoPin, float startMillis);
void configure_food_time();
void configure_water_time();
void init_ussensor();
void fill_food_bowl();
void pump_water();
void init_pump();
void fill_water_bowl();
void init_ir_sensor();
bool is_full();
void init_adc();
uint16_t read_water_level();


// Main
int main() {
    stdio_init_all();
    init_oled();
    init_buttons();
    init_rtc();
    init_ussensor();
    init_pump();
    init_ir_sensor();
    init_adc();

    state = STARTUP;
    prev_state = STARTUP;

    while (1) {
        switch (state) {
            case STARTUP:
                display_startup();
                state = WELCOME;
                break;
            case WELCOME:
                display_welcome();
                state = MODE_SELECT;
                break;
            case DATE_CONFIG:
                configure_date();
                break;
            case TIME_CONFIG:
                configure_time();
                break;
            case MODE_SELECT:
                select_mode();
                break;
            case MANUAL_MODE:
                manual_mode();
                break;
            case AUTOMATIC_MODE:
                automatic_mode();
                break;
            case MONITORING:
                monitoring();
                break;
            case FOOD_TIME_CONFIG:
                configure_food_time();
                break;
            case WATER_TIME_CONFIG:
                configure_water_time();
                break;
        }
        if (hour_2f == 0 && minute_2f == 0 && second_2f == 0) {
            //printf("\nNO TIME TO FOOD IS SET\n");
        }
        else {
             //printf("\nA TIME TO FOOD HAS BEEN SET!\n");
             ds3231_read_time(&rtc_second, &rtc_minute, &rtc_hour);
             if(rtc_hour == hour_2f_base+hour_2f && rtc_minute == minute_2f_base+minute_2f && rtc_second == second_2f_base+second_2f){
                printf("\nTIME FOR FOOD!\n");
                fill_food_bowl();
                food_count++;
                sleep_ms(200); // comentar si es necesario ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
             }
        }

        if (hour_2w == 0 && minute_2w == 0 && second_2w == 0) {
            //printf("\nNO TIME TO WATER IS SET\n");
        }
        else {
             //printf("\nA TIME TO WATER HAS BEEN SET!\n");
             ds3231_read_time(&rtc_second, &rtc_minute, &rtc_hour);
             if(rtc_hour == hour_2w_base+hour_2w && rtc_minute == minute_2w_base+minute_2w && rtc_second == second_2w_base+second_2w){
                printf("\nTIME FOR WATER!\n");
                fill_water_bowl();
                water_count++;
                sleep_ms(200); // comentar si es necesario ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
             }
        }
    }
    return 0;
}

void init_oled() {
    rc = oledInit(&oled, OLED_128x64, 0x3c, 0, 0, 1, SDA_PIN, SCL_PIN, RESET_PIN, 1000000L);
    if (rc == OLED_NOT_FOUND) {
        printf("OLED not found\n");
        while (1);
    }
    oledFill(&oled, 0, 1);
    oledSetContrast(&oled, 127);
}

void init_rtc() {
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(SDA_PIN_RTC, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN_RTC, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN_RTC);
    gpio_pull_up(SCL_PIN_RTC);
}

void init_buttons() {
    gpio_init(UP_PIN);
    gpio_set_dir(UP_PIN, GPIO_IN);
    gpio_pull_up(UP_PIN);

    gpio_init(DOWN_PIN);
    gpio_set_dir(DOWN_PIN, GPIO_IN);
    gpio_pull_up(DOWN_PIN);

    gpio_init(LEFT_PIN);
    gpio_set_dir(LEFT_PIN, GPIO_IN);
    gpio_pull_up(LEFT_PIN);

    gpio_init(RIGHT_PIN);
    gpio_set_dir(RIGHT_PIN, GPIO_IN);
    gpio_pull_up(RIGHT_PIN);

    gpio_init(OK_PIN);
    gpio_set_dir(OK_PIN, GPIO_IN);
    gpio_pull_up(OK_PIN);

    gpio_init(BACK_PIN);
    gpio_set_dir(BACK_PIN, GPIO_IN);
    gpio_pull_up(BACK_PIN);
}

void display_startup() {
    oledFill(&oled, 0, 1);
    oledWriteString(&oled, 0, 0, 2, (char *)"FEEDOG", FONT_16x16, 0, 1);
    oledWriteString(&oled, 0, 0, 5, (char *)" /l__/l", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 6, (char *)"(^ w ^ )  ", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 7, (char *)" ( u u )_S ", FONT_6x8, 0, 1);
    sleep_ms(3000);
}

void display_welcome() {
    oledFill(&oled, 0, 1);
    oledWriteString(&oled, 0, 0, 1, (char *)"Bienvenido al sistema", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 2, (char *)"de alimentacion", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 3, (char *)"automatico", FONT_6x8, 0, 1);
    sleep_ms(2000);
}

void configure_date() {
    handle_input();
    oledFill(&oled, 0, 1);
    oledWriteString(&oled, 0, 0, 0, (char *)"Configurar fecha", FONT_6x8, 0, 1);
    display_date();
    display_real_time();
    sleep_ms(17);
}

void configure_time() {
    handle_input();
    oledFill(&oled, 0, 1);
    oledWriteString(&oled, 0, 0, 0, (char *)"Configurar hora", FONT_6x8, 0, 1);
    display_time();
    display_real_time();
    sleep_ms(17);
}

void select_mode() {
    static int mode = 0;
    handle_input();
    oledFill(&oled, 0, 1);
    oledWriteString(&oled, 0, 0, 0, (char *)"Seleccione modo:", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 2, current_option == 0 ? "Manual           <" : "Manual", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 3, current_option == 1 ? "Auto             <" : "Auto", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 4, current_option == 2 ? "Configurar Fecha <" : "Configurar Fecha", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 5, current_option == 3 ? "Configurar Hora  <" : "Configurar Hora", FONT_6x8, 0, 1);
    display_real_time();
    sleep_ms(17);
}

void manual_mode() {
    static int option = 0;
    handle_input();
    oledFill(&oled, 0, 1);
    oledWriteString(&oled, 0, 0, 0, (char *)"Modo Manual", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 2, current_option == 0 ? "Dar alimento <" : "Dar alimento", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 3, current_option == 1 ? "Dar agua     <" : "Dar agua", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 4, current_option == 2 ? "Monitorear   <" : "Monitorear", FONT_6x8, 0, 1);
    handle_input();
    display_real_time();
    sleep_ms(17);
}

void automatic_mode() {
    static int option = 0;
    handle_input();
    oledFill(&oled, 0, 1);
    oledWriteString(&oled, 0, 0, 0, (char *)"Modo Automatico", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 2, current_option == 0 ? "Periodo de comida <" : "Periodo de comida", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 3, current_option == 1 ? "Periodo de agua   <" : "Periodo de agua", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 4, current_option == 2 ? "Monitorear        <" : "Monitorear", FONT_6x8, 0, 1);
    display_real_time();
    sleep_ms(17);
}

void monitoring() {
    handle_input();
    oledFill(&oled, 0, 1);

    // Format food_str
    char food_str[3];
    snprintf(food_str, sizeof(food_str), "%02d", food_count);

    // Concatenate food_str to "Comidas: "
    char food_str_full[13];
    snprintf(food_str_full, sizeof(food_str_full), "%s %s", "Comidas: ", food_str);

    // Format water_str
    char water_str[3];
    snprintf(water_str, sizeof(water_str), "%02d", water_count);

    // Concatenate water_str to "Bebidas: "
    char water_str_full[13];
    snprintf(water_str_full, sizeof(water_str_full), "%s %s", "Bebidas: ", water_str);

    uint16_t water_level = read_water_level();
    // Format wlevel_str
    char wlevel_str[5];
    snprintf(wlevel_str, sizeof(wlevel_str), "%04d", water_level);

    // Concatenate food_str to "Comidas: "
    char wlevel_str_full[18];
    snprintf(wlevel_str_full, sizeof(wlevel_str_full), "%s %s", "Nivel agua: ", wlevel_str);
    
    
    oledWriteString(&oled, 0, 0, 0, (char *)"Monitoreo", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 2, food_str_full, FONT_6x8, 0, 1);
    //oledWriteString(&oled, 0, 0, 2, (char *)"Comidas: 75%", FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 3, water_str_full, FONT_6x8, 0, 1);
    oledWriteString(&oled, 0, 0, 4, wlevel_str_full, FONT_6x8, 0, 1); ////////////////////////// VERIFICAR FUNCIONAMIENTO DE MONITOREO DE NIVEL DE AGUA  ////////////////////////////////////////////////////////////
    //oledWriteString(&oled, 0, 0, 3, (char *)"Bebidas: 60%", FONT_6x8, 0, 1);
    //oledWriteString(&oled, 0, 0, 4, (char *)"Plato vacio: 20%", FONT_6x8, 0, 1);
    //oledWriteString(&oled, 0, 0, 5, (char *)"Plato agua: 30%", FONT_6x8, 0, 1);
    display_real_time();
    sleep_ms(17);
}

void configure_food_time() {
    handle_input();
    oledFill(&oled, 0, 1);
    oledWriteString(&oled, 0, 0, 0, (char *)"Configurar hora", FONT_6x8, 0, 1);
    display_time();
    display_real_time();
    sleep_ms(17);
}

void configure_water_time() {
    handle_input();
    oledFill(&oled, 0, 1);
    oledWriteString(&oled, 0, 0, 0, (char *)"Configurar hora", FONT_6x8, 0, 1);
    display_time();
    display_real_time();
    sleep_ms(17);
}

void handle_input() {
    if (!gpio_get(UP_PIN)) {
        if (state == DATE_CONFIG || state == TIME_CONFIG || state == FOOD_TIME_CONFIG || state == WATER_TIME_CONFIG) {
            increment_section();
        } else if (state == MODE_SELECT) {
            if (current_option > 0) {
                current_option--;
            }
        } else if (state == MANUAL_MODE || state == AUTOMATIC_MODE ) {
            if (current_option > 0) {
                current_option--;
            }
        }
        sleep_ms(200);
    }
    if (!gpio_get(DOWN_PIN)) {
        if (state == DATE_CONFIG || state == TIME_CONFIG || state == FOOD_TIME_CONFIG || state == WATER_TIME_CONFIG) {
            decrement_section();
        } else if (state == MODE_SELECT) {
            if (current_option < 3) { // Asume que hay dos opciones: Auto y Manual
                current_option++;
            }
        } else if (state == MANUAL_MODE || state == AUTOMATIC_MODE) {
            if (current_option < 2) { // Asume que hay tres opciones en estos modos
                current_option++;
            }
        }
        sleep_ms(200);
    }
    if (!gpio_get(LEFT_PIN)) {
        if (state == DATE_CONFIG || state == TIME_CONFIG || state == FOOD_TIME_CONFIG || state == WATER_TIME_CONFIG) {
            move_left();
        }
        sleep_ms(200);
    }
    if (!gpio_get(RIGHT_PIN)) {
        if (state == DATE_CONFIG || state == TIME_CONFIG || state == FOOD_TIME_CONFIG || state == WATER_TIME_CONFIG) {
            move_right();
        }
        sleep_ms(200);
    }

    switch (state) {
        case DATE_CONFIG:
            if (!gpio_get(OK_PIN)) {
                state = MODE_SELECT;
                prev_state = DATE_CONFIG;
                ds3231_set_date(day,month,year-2000);
                sleep_ms(200);                
            }
            else if (!gpio_get(BACK_PIN)) {
                state = MODE_SELECT;
                prev_state = DATE_CONFIG;
                sleep_ms(200);
            }
            break;
        case TIME_CONFIG:
            if (!gpio_get(OK_PIN)) {
                state = MODE_SELECT;
                prev_state = TIME_CONFIG;
                //ds3231_read_time(&rtc_second, &rtc_minute, &rtc_hour);
                ds3231_set_time(hour, minute, second);
                sleep_ms(200);
            }
            else if (!gpio_get(BACK_PIN)) {
                state = MODE_SELECT;
                prev_state = TIME_CONFIG;
                sleep_ms(200);
            }
            break;
        case MODE_SELECT:
            if (!gpio_get(OK_PIN)) {
                if (current_option == 0) {
                    state = MANUAL_MODE;
                    prev_state = MODE_SELECT;
                }
                else if (current_option == 1) {
                    state = AUTOMATIC_MODE;
                    prev_state = MODE_SELECT;
                }
                else if (current_option == 2) {
                    state = DATE_CONFIG;
                    prev_state = MODE_SELECT;                   
                }
                else if (current_option == 3) {
                    state = TIME_CONFIG;
                    prev_state = MODE_SELECT;                    
                }
                else {
                    state = MODE_SELECT;
                    prev_state = TIME_CONFIG;
                }
                sleep_ms(200);
            }
            else if (!gpio_get(BACK_PIN)) {
                state = MODE_SELECT;
                prev_state = MODE_SELECT;
                sleep_ms(200);
            }
            break;
        case MANUAL_MODE:
            if (!gpio_get(OK_PIN)) {
                if (current_option == 2) { // Si la opción seleccionada es "Monitorear"
                    prev_state = MANUAL_MODE;
                    state = MONITORING;
                }
                else if (current_option == 0) {// Si la opción seleccionada es "Dar comida"
                    move_servo();
                    prev_state = MANUAL_MODE;
                }
                else if (current_option == 1) {// Si la opción seleccionada es "Dar agua"
                    pump_water();
                    prev_state = MANUAL_MODE;
                }
                else {
                    prev_state = MODE_SELECT;
                }
                sleep_ms(200);
            }
            else if (!gpio_get(BACK_PIN)) {
                state = MODE_SELECT;
                prev_state = MANUAL_MODE;
                sleep_ms(200);
            }

            break;
        case AUTOMATIC_MODE:
            if (!gpio_get(OK_PIN)) {
                if (current_option == 2) { // Si la opción seleccionada es "Monitorear"
                    prev_state = AUTOMATIC_MODE;
                    state = MONITORING;
                }
                else if (current_option == 0) {// Si la opción seleccionada es "Dar comida"
                    state = FOOD_TIME_CONFIG;
                    prev_state = AUTOMATIC_MODE;
                }
                else if (current_option == 1) {// Si la opción seleccionada es "Dar agua"
                    state = WATER_TIME_CONFIG;
                    prev_state = AUTOMATIC_MODE;
                }
                else {
                    prev_state = MODE_SELECT;
                }
                sleep_ms(200);
            }
            else if (!gpio_get(BACK_PIN)) {
                state = MODE_SELECT;
                prev_state = AUTOMATIC_MODE;
                sleep_ms(200);
            }
            break;
        case MONITORING:
            if (!gpio_get(OK_PIN)) {
                state = MONITORING;
                //prev_state = AUTOMATIC_MODE;  
                sleep_ms(200);             
            }
            else if (!gpio_get(BACK_PIN)) {
                state = prev_state;
                prev_state = MODE_SELECT;
                //prev_state = AUTOMATIC_MODE;
                sleep_ms(200);
            }
        break;
        case FOOD_TIME_CONFIG:
            if (!gpio_get(OK_PIN)) {
                state = AUTOMATIC_MODE;
                prev_state = FOOD_TIME_CONFIG;
                //EN LUGAR DE GUARDAR EN EL RTC, debe guardar en el sistema (memoria intera o eeprom o lo que sea)
                hour_2f_base = rtc_hour;
                minute_2f_base = rtc_minute;
                second_2f_base = rtc_second;
                sleep_ms(200);
            }
            else if (!gpio_get(BACK_PIN)) {
                state = AUTOMATIC_MODE;
                prev_state = FOOD_TIME_CONFIG;
                sleep_ms(200);
            }
        break;
        case WATER_TIME_CONFIG:
            if (!gpio_get(OK_PIN)) {
                state = AUTOMATIC_MODE;
                prev_state = WATER_TIME_CONFIG;
                //EN LUGAR DE GUARDAR EN EL RTC, debe guardar en el sistema (memoria intera o eeprom o lo que sea)
                hour_2w_base = rtc_hour;
                minute_2w_base = rtc_minute;
                second_2w_base = rtc_second;
                sleep_ms(200);
            }
            else if (!gpio_get(BACK_PIN)) {
                state = AUTOMATIC_MODE;
                prev_state = WATER_TIME_CONFIG;
                sleep_ms(200);
            }
        break;
    }
}

void increment_section() {
    // Incrementar valor de la sección seleccionada (día, mes, año, hora, minuto, segundo)
    if (state == DATE_CONFIG) {
        if (selected_section == 0) { // Día
            if ((month == 2 && day < 28) || (month != 2 && day < 31)) {
                day++;
            }
        } else if (selected_section == 1) { // Mes
            if (month < 12) {
                month++;
            }
        } else if (selected_section == 2) { // Año
            if (year < 2124) {
                year++;
            }
        }
    } else if (state == TIME_CONFIG) {
        if (selected_section == 0) { // Hora
            if (hour < 23) {
                hour++;
            }
        } else if (selected_section == 1) { // Minuto
            if (minute < 59) {
                minute++;
            }
        } else if (selected_section == 2) { // Segundo
            if (second < 59) {
                second++;
            }
        }
    } else if (state == FOOD_TIME_CONFIG){
        if (selected_section == 0) { // Hora para comida
            if (hour_2f < 23) {
                hour_2f++;
            }
        } else if (selected_section == 1) { // Minuto para comida
            if (minute_2f < 59) {
                minute_2f++;
            }
        } else if (selected_section == 2) { // Segundo para comida
            if (second_2f < 59) {
                second_2f++;
            }
        }
    } else if (state == WATER_TIME_CONFIG) {
        if (selected_section == 0) { // Hora para agua
            if (hour_2w < 23) {
                hour_2w++;
            }
        } else if (selected_section == 1) { // Minuto para agua
            if (minute_2w < 59) {
                minute_2w++;
            }
        } else if (selected_section == 2) { // Segundo para agua
            if (second_2w < 59) {
                second_2w++;
            }
        }
    }
}

void decrement_section() {
    // Decrementar valor de la sección seleccionada (día, mes, año, hora, minuto, segundo)
    if (state == DATE_CONFIG) {
        if (selected_section == 0) { // Día
            if (day > 1) {
                day--;
            }
        } else if (selected_section == 1) { // Mes
            if (month > 1) {
                month--;
            }
        } else if (selected_section == 2) { // Año
            if (year > 2024) {
                year--;
            }
        }
    } else if (state == TIME_CONFIG) {
        if (selected_section == 0) { // Hora
            if (hour > 0) {
                hour--;
            }
        } else if (selected_section == 1) { // Minuto
            if (minute > 0) {
                minute--;
            }
        } else if (selected_section == 2) { // Segundo
            if (second > 0) {
                second--;
            }
        }
    } else if (state == FOOD_TIME_CONFIG) {
        if (selected_section == 0) { // Hora para comida
            if (hour_2f > 0) {
                hour_2f--;
            }
        } else if (selected_section == 1) { // Minuto para comida
            if (minute_2f > 0) {
                minute_2f--;
            }
        } else if (selected_section == 2) { // Segundo para comida
            if (second_2f > 0) {
                second_2f--;
            }
        }        
    } else if (state == WATER_TIME_CONFIG) {
        if (selected_section == 0) { // Hora para comida
            if (hour_2w > 0) {
                hour_2w--;
            }
        } else if (selected_section == 1) { // Minuto para comida
            if (minute_2w > 0) {
                minute_2w--;
            }
        } else if (selected_section == 2) { // Segundo para comida
            if (second_2w > 0) {
                second_2w--;
            }
        }         
    }
}

void move_left() {
    // Mover selección a la izquierda entre las secciones (día, mes, año, hora, minuto, segundo)
    if (selected_section > 0) {
        selected_section--;
    }
}

void move_right() {
    // Mover selección a la derecha entre las secciones (día, mes, año, hora, minuto, segundo)
    if (state == DATE_CONFIG && selected_section < 2) {
        selected_section++;
    } else if (state == TIME_CONFIG && selected_section < 2) {
        selected_section++;
    } else if (state == FOOD_TIME_CONFIG && selected_section < 2) {
        selected_section++;
    } else if (state == WATER_TIME_CONFIG && selected_section < 2) {
        selected_section++;
    }
}

void display_date() {
    char date_str[11];
    snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d", day, month, year);
    oledWriteString(&oled, 0, 0, 2, date_str, FONT_8x8, 0, 1);

    // Dibujar cursor
    char cursor[4] = "   ";
    cursor[selected_section] = '^';
    oledWriteString(&oled, 0, 0, 4, cursor, FONT_8x8, 0, 1);
}

void display_time() {
    if (state == TIME_CONFIG) {
        char time_str[9];
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour, minute, second);
        oledWriteString(&oled, 0, 0, 2, time_str, FONT_8x8, 0, 1);

        // Dibujar cursor
        char cursor[4] = "   ";
        cursor[selected_section] = '^';
        oledWriteString(&oled, 0, 0, 4, cursor, FONT_8x8, 0, 1);
    } else if (state == FOOD_TIME_CONFIG){
        char time_str[9];
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour_2f, minute_2f, second_2f);
        oledWriteString(&oled, 0, 0, 2, time_str, FONT_8x8, 0, 1);

        // Dibujar cursor
        char cursor[4] = "   ";
        cursor[selected_section] = '^';
        oledWriteString(&oled, 0, 0, 4, cursor, FONT_8x8, 0, 1);
    } else if (state == WATER_TIME_CONFIG){
        char time_str[9];
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour_2w, minute_2w, second_2w);
        oledWriteString(&oled, 0, 0, 2, time_str, FONT_8x8, 0, 1);

        // Dibujar cursor
        char cursor[4] = "   ";
        cursor[selected_section] = '^';
        oledWriteString(&oled, 0, 0, 4, cursor, FONT_8x8, 0, 1);
    }

}

void ds3231_read_time(uint8_t *rtc_second, uint8_t *rtc_minute, uint8_t *rtc_hour) {
    uint8_t buffer[3];
    uint8_t reg = 0x00;

    i2c_write_blocking(I2C_PORT, DS3231_ADDRESS, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, DS3231_ADDRESS, buffer, 3, false);

    *rtc_second = ((buffer[0] >> 4) & 0x07) * 10 + (buffer[0] & 0x0F);
    *rtc_minute = ((buffer[1] >> 4) & 0x07) * 10 + (buffer[1] & 0x0F);
    *rtc_hour = ((buffer[2] >> 4) & 0x03) * 10 + (buffer[2] & 0x0F);
}
void ds3231_set_time(int hour, int minute, int second) {
    uint8_t buffer[4];

    buffer[0] = 0x00; // Start at address 0x00 (seconds)
    buffer[1] = ((second / 10) << 4) | (second % 10); // Seconds
    buffer[2] = ((minute / 10) << 4) | (minute % 10); // Minutes
    buffer[3] = ((hour / 10) << 4) | (hour % 10); // Hours

    i2c_write_blocking(I2C_PORT, DS3231_ADDRESS, buffer, 4, false);
}



void ds3231_set_date(int day, int month, int year) {
    uint8_t buffer[4];

    buffer[0] = 0x04; // Start at address 0x04 (day)
    buffer[1] = ((day / 10) << 4) | (day % 10); // Day
    buffer[2] = ((month / 10) << 4) | (month % 10); // Month

    // Convert year to BCD format
    int tens = year / 10;
    int units = year % 10;
    buffer[3] = ((tens << 4) & 0xF0) | (units & 0x0F); // Year

    i2c_write_blocking(I2C_PORT, DS3231_ADDRESS, buffer, 4, false);
}


void ds3231_read_date(uint8_t *rtc_day, uint8_t *rtc_month, uint8_t *rtc_year) {
    uint8_t buffer[4];

    buffer[0] = 0x04; // Start at address 0x04 (day)

    i2c_write_blocking(I2C_PORT, DS3231_ADDRESS, buffer, 1, true);
    i2c_read_blocking(I2C_PORT, DS3231_ADDRESS, buffer, 4, false);

    *rtc_day = ((buffer[0] >> 4) * 10) + (buffer[0] & 0x0F);
    *rtc_month = ((buffer[1] >> 4) * 10) + (buffer[1] & 0x0F);
    *rtc_year = ((buffer[2] >> 4) * 10) + (buffer[2] & 0x0F);
}


void display_real_time() {
    // Read the time from your RTC module
    ds3231_read_time(&rtc_second, &rtc_minute, &rtc_hour);
    ds3231_read_date(&rtc_day, &rtc_month, &rtc_year);

    // Format the time string
    char rtc_time_str[9];
    snprintf(rtc_time_str, sizeof(rtc_time_str), "%02d:%02d:%02d", rtc_hour, rtc_minute, rtc_second);

    // Format the date string
    char rtc_date_str[9];
    snprintf(rtc_date_str, sizeof(rtc_date_str), "%02d/%02d/%02d", rtc_day, rtc_month, rtc_year);

    // Concatenate time and date strings
    char rtc_datetime_str[18];
    snprintf(rtc_datetime_str, sizeof(rtc_datetime_str), "%s %s", rtc_time_str, rtc_date_str);

    // Display the time and date on the OLED display
    oledWriteString(&oled, 0, 0, 7, rtc_datetime_str, FONT_6x8, 0, 1);
}

// Function to convert degrees to pulse width in milliseconds
float degreesToMillis(float degrees)
{
    return minPulseWidth + (degrees / 180.0f) * (maxPulseWidth - minPulseWidth);
}

void setMillis(int servoPin, float millis)
{
    pwm_set_gpio_level(servoPin, (millis / 20.0f) * wrap);  // 20 ms period
    return;
}

void setServo(int servoPin, float startMillis)
{
    gpio_set_function(servoPin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(servoPin);

    pwm_config config = pwm_get_default_config();

    uint64_t clockspeed = clock_get_hz(clk_sys);
    clockDiv = 64.0f;
    wrap = 39062.0f;

    while (clockspeed / clockDiv / 50 > 65535 && clockDiv < 256) 
        clockDiv += 64; 
    
    wrap = clockspeed / clockDiv / 50;

    pwm_config_set_clkdiv(&config, clockDiv);
    pwm_config_set_wrap(&config, wrap);

    pwm_init(slice_num, &config, true);

    setMillis(servoPin, startMillis);
    return;
}

void move_servo() {
    // Convert degrees to pulse widths
    float initialPositionMillis = degreesToMillis(initialPositionDegrees);
    float finalPositionMillis = degreesToMillis(finalPositionDegrees);

    setServo(servoPin, initialPositionMillis);
    setMillis(servoPin, finalPositionMillis);
    sleep_ms(500); 

    // Move the servo back to the initial position
    setMillis(servoPin, initialPositionMillis);
    return;
}

void fill_food_bowl() {
    printf("\nFILLING:   ...   \n");
    //declarar contador de dosis en 0.
    int count = 0;
    //mientras que sensor IR NO detecte la minima distancia o contador de dosis máximo en 4:
    while (count < 4) {
        /*if(is_full()){break;}
        else{
            move_servo();
            sleep_ms(3000); 	///////////////////////////////////////////////////////////////////////// REDUCIR TIEMPO DE SER NECESARIO ///////////////////////////////////////////////////
        }*/
        move_servo();
        sleep_ms(3000); 	///////////////////////////////////////////////////////////////////////// REDUCIR TIEMPO DE SER NECESARIO ///////////////////////////////////////////////////
        count++;
    }
    count = 0;
    sleep_ms(1000);
    ds3231_read_time(&rtc_second, &rtc_minute, &rtc_hour);
    hour_2f_base = rtc_hour;
    minute_2f_base = rtc_minute;
    second_2f_base = rtc_second;
    return;
}

void init_ussensor() {
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_put(TRIG_PIN, 0);

    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    return;
}

void init_pump() {
    // Initialize the GPIO for the pump
    gpio_init(PUMP_PIN);
    gpio_set_dir(PUMP_PIN, GPIO_OUT);
}

void pump_water() {
    // Turn on the pump
    gpio_put(PUMP_PIN, 1);
    // Wait for 2 seconds
    sleep_ms(2000);

    // Turn off the pump
    gpio_put(PUMP_PIN, 0);
    // Wait for 3 seconds
    sleep_ms(200);
}

void fill_water_bowl() {
    /*
    printf("\nPUMPING:   ...   \n");
    //declarar contador de dosis en 0.
    int count = 0;
    //leer sensor de ultrasonido
    float distance = measure_distance(TRIG_PIN, ECHO_PIN);
    printf("Distance: %.2f cm\n", distance);
    //mientras que sensor de ultrasonido NO detecte la minima distancia (maximo tamaño de monticulo, minima cercania al sensor) o contador de sosis máximo en 5:
    while (count < 4 ) {
        //ejecutar move_servo()
        float distance = measure_distance(TRIG_PIN, ECHO_PIN);
        pump_water();
        sleep_ms(500);
        //leer sensor de ultrasonido
        
        printf("Distance: %.2f cm\n", distance);
        if (distance <= 10.0){break;}
        //sleep_ms(1000);
        count++;
    }
    count = 0;
    ds3231_read_time(&rtc_second, &rtc_minute, &rtc_hour);
    hour_2w_base = rtc_hour;
    minute_2w_base = rtc_minute;
    second_2w_base = rtc_second;
    return; 
    */
    printf("\nPUMPING:   ...   \n");
    //declarar contador de dosis en 0.
    int count2 = 0;
    //leer sensor de ultrasonido
    uint16_t water_level = read_water_level();
    printf("water level: %.2f \n", water_level);
    //mientras que sensor de nivel NO detecte el máximo configurado  o contador de dosis máximo en 3:
    while (count2 < 3 ) {
        if (water_level > 1200){break;}
        else{
            pump_water();
            sleep_ms(500);           
        }
        count2++;
    }
    count2 = 0;
    sleep_ms(1000);
    ds3231_read_time(&rtc_second, &rtc_minute, &rtc_hour);
    hour_2w_base = rtc_hour;
    minute_2w_base = rtc_minute;
    second_2w_base = rtc_second;
    return; 
}

// Initialize the IR sensor pin
void init_ir_sensor() {
    gpio_init(IR_SENSOR_PIN);
    gpio_set_dir(IR_SENSOR_PIN, GPIO_IN);
}

// Function to check if food bowl is full
bool is_full() {
    return gpio_get(IR_SENSOR_PIN);
}

void init_adc() {
    adc_init();
    adc_gpio_init(WATER_SENSOR_PIN);
    adc_select_input(0);  // ADC input 0 is connected to GP26
}

uint16_t read_water_level() {
    return adc_read();
}