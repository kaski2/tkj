// Christian R. Miko Nikula, Aapo Kinnunen


/* C Standard library */
#include <stdio.h>
#include <string.h>
#include <math.h>

/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/i2c/I2CCC26XX.h>

/* Board Header files */
#include "Board.h"
#include "sensors/opt3001.h"
#include "sensors/mpu9250.h"

#include "motion.h"
#include "pitches.h"
#include "buzzer.h"


/* Task */
#define STACKSIZE 2048
Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];
uint8_t uartBuffer[80];


enum state { WAITING=1, DATA_READY};
enum state programState = WAITING;
enum motionState {MOTION_SENSOR=1, LIGHT_SENSOR};
enum motionState sensorState = MOTION_SENSOR;


double ambientLight = -1000.0;
float ax, ay, az, gx, gy, gz;
int pet, feed, workout;
int beepDetected = 0;


static PIN_Handle buttonHandle;
static PIN_State buttonState;
static PIN_Handle ledHandle;
static PIN_State ledState;
static PIN_Handle hMpuPin;
static PIN_State  MpuPinState;


PIN_Config buttonConfig[] = {
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE};

PIN_Config ledConfig[] = {
     Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
     PIN_TERMINATE};

static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};

static PIN_Handle hBuzzer;
static PIN_State sBuzzer;
PIN_Config cBuzzer[] = {
  Board_BUZZER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
  PIN_TERMINATE
};



void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
       uint_t pinValue = PIN_getOutputValue( Board_LED0 );
       pinValue = !pinValue;
       PIN_setOutputValue( ledHandle, Board_LED0, pinValue );


       if(sensorState == MOTION_SENSOR){
           sensorState = LIGHT_SENSOR;

       }else {
           sensorState = MOTION_SENSOR;
       }
}

/* Task Functions */

Void uartFxn(UART_Handle handle, void *rxBuf, size_t len){


    if (strncmp (rxBuf,"3255,BEEP:I",11) == 0){
        beepDetected = 1;

    }else if(strncmp(rxBuf,"3255,BEEP:Running", 17) == 0){
        beepDetected = 1;
    }else if(strncmp(rxBuf,"3255,BEEP:Severe", 16) == 0){
        beepDetected = 1;
    }
    UART_read(handle, rxBuf, 80);
}

Void uartTaskFxn(UArg arg0, UArg arg1) {
    char string[100];

    UART_Handle uart;
    UART_Params uartParams;

    UART_Params_init(&uartParams);
       uartParams.writeDataMode = UART_DATA_TEXT;
       uartParams.readDataMode = UART_DATA_TEXT;
       uartParams.readEcho = UART_ECHO_OFF;
       uartParams.readMode=UART_MODE_CALLBACK;
       uartParams.readCallback= &uartFxn;
       uartParams.baudRate = 9600;
       uartParams.dataLength = UART_LEN_8;
       uartParams.parityType = UART_PAR_NONE;
       uartParams.stopBits = UART_STOP_ONE;

       uart = UART_open(Board_UART0, &uartParams);
          if (uart == NULL) {
             System_abort("Error opening the UART");
          }
          UART_read(uart, uartBuffer, 80);
    while (1) {

        if(beepDetected == 1){
            int i;
            buzzerOpen(hBuzzer);
            for(i=0; i<36; i++){
                if(notes[i] == 0){
                    buzzerSetFrequency(0);
                    Task_sleep(pauses[i]*1000 / Clock_tickPeriod);
                }else {
                    buzzerSetFrequency(notes[i]);
                    Task_sleep(pauses[i]*1000 / Clock_tickPeriod);
                    buzzerSetFrequency(0);

                }
            }
           buzzerClose();
           Task_sleep(950000 / Clock_tickPeriod);
           beepDetected = 0;
        }

        if(sensorState == MOTION_SENSOR){

            if(programState == DATA_READY){
                if(feed == 1){
                    sprintf(string, "id:3255,EAT:1\0");
                    UART_write(uart, string, 14);
                    buzzerOpen(hBuzzer);
                    buzzerSetFrequency(2000);
                    Task_sleep(50000/Clock_tickPeriod);
                    buzzerClose();
                }
                if(pet == 1){
                    sprintf(string, "id:3255,PET:1\0");
                    UART_write(uart, string, 14);
                    buzzerOpen(hBuzzer);
                    buzzerSetFrequency(5000);
                    Task_sleep(50000/Clock_tickPeriod);
                    buzzerClose();

                }
                if(workout == 1){
                    sprintf(string, "id:3255,EXERCISE:1\0");
                    UART_write(uart, string, 19);
                    buzzerOpen(hBuzzer);
                    buzzerSetFrequency(7000);
                    Task_sleep(50000/Clock_tickPeriod);
                    buzzerClose();
                }
                programState = WAITING;

            }
        }else {
            if(programState == DATA_READY){
                if(ambientLight < 500){
                    sprintf(string, "id:3255,MSG1:It's dark\0");
                    UART_write(uart, string, 23);
                }else {
                    sprintf(string, "id:3255,MSG1:It's bright\0");
                    UART_write(uart, string, 25);
                }
                programState = WAITING;

            }

        }


        System_flush();
        Task_sleep(500000 / Clock_tickPeriod);
    }
}

Void sensorTaskFxn(UArg arg0, UArg arg1) {


    Char string[30];

    I2C_Handle      i2c;
    I2C_Params      i2cParams;
    I2C_Params      i2cMotionParams;


    I2C_Params_init(&i2cParams);
    I2C_Params_init(&i2cMotionParams);
    i2cParams.bitRate = I2C_400kHz;
    i2cMotionParams.bitRate = I2C_400kHz;
    i2cMotionParams.custom = (uintptr_t)&i2cMPUCfg;

    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);



         i2c = I2C_open(Board_I2C_TMP, &i2cMotionParams);
         mpu9250_setup(&i2c);
         if (i2c == NULL) {
                 System_abort("Error Initializing I2C\n");
         }
         Task_sleep(100000/Clock_tickPeriod);





    while (1) {

        if(sensorState == LIGHT_SENSOR){
            I2C_close(i2c);
            i2c = I2C_open(Board_I2C_TMP, &i2cParams);
            opt3001_setup(&i2c);

            if (i2c == NULL) {
               System_abort("Error Initializing I2C\n");
            }
            while(sensorState == LIGHT_SENSOR){

                if(programState == WAITING){
                    ambientLight = opt3001_get_data(&i2c);
                    programState = DATA_READY;
                }
                Task_sleep(100000/Clock_tickPeriod);

            }
        }
        if(sensorState == MOTION_SENSOR){
            I2C_close(i2c);
            i2c = I2C_open(Board_I2C_TMP, &i2cMotionParams);
            mpu9250_setup(&i2c);
            if(i2c == NULL){
               System_abort("Error Initializing I2C\n");
            }
            Task_sleep(100000/Clock_tickPeriod);
            while(sensorState ==  MOTION_SENSOR){
                    mpu9250_get_data(&i2c, &ax, &ay, &az, &gx, &gy, &gz);

                    if(programState == WAITING){
                        pet = detectLift(ax, ay, az, gx);
                        workout = detectSlide(ax, ay, az, gx, gy, gz);
                        feed = detectTurn(ax, ay, az, gx);
                        programState = DATA_READY;
                    }

                    if(pet == 1){
                        sprintf(string, "Lift Up\n");
                        System_printf(string);
                        System_flush();
                    }


                    if(workout == 1){
                        sprintf(string, "Slide\n");
                        System_printf(string);
                        System_flush();
                    }


                    if(feed == 1){
                        sprintf(string, "Turn\n");
                        System_printf(string);
                        System_flush();
                    }
                    Task_sleep(100000/Clock_tickPeriod);
            }
        }
        Task_sleep(100000 / Clock_tickPeriod);
    }
}



Int main(void) {

    // Task variables
    Task_Handle sensorTaskHandle;
    Task_Params sensorTaskParams;
    Task_Handle uartTaskHandle;
    Task_Params uartTaskParams;

    // Initialize board
    Board_initGeneral();
    Board_initI2C();
    Board_initUART();

    buttonHandle = PIN_open(&buttonState, buttonConfig);
       if(!buttonHandle) {
          System_abort("Error initializing button pins\n");
       }
       ledHandle = PIN_open(&ledState, ledConfig);
       if(!ledHandle) {
          System_abort("Error initializing LED pins\n");
       }

       if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
             System_abort("Error registering button callback function");
          }

       hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
           if (hMpuPin == NULL) {
               System_abort("Pin open failed!");
           }

        hBuzzer = PIN_open(&sBuzzer, cBuzzer);
        if (hBuzzer == NULL) {
        System_abort("Pin open failed!");
        }

    /* Task */
    Task_Params_init(&sensorTaskParams);
    sensorTaskParams.stackSize = STACKSIZE;
    sensorTaskParams.stack = &sensorTaskStack;
    sensorTaskParams.priority=2;
    sensorTaskHandle = Task_create(sensorTaskFxn, &sensorTaskParams, NULL);
    if (sensorTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority=2;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
