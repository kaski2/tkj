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

/* Task */
#define STACKSIZE 2048
Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];
uint8_t uartBuffer[80];




// JTKJ: Teht�v� 3. Tilakoneen esittely
// JTKJ: Exercise 3. Definition of the state machine
enum state { WAITING=1, DATA_READY };
enum state programState = WAITING;
//TODO MAYBE TEMP SENSOR :)
enum motionState {MOTION_SENSOR=1, LIGHT_SENSOR};
enum motionState sensorState = MOTION_SENSOR;

// JTKJ: Teht�v� 3. Valoisuuden globaali muuttuja
// JTKJ: Exercise 3. Global variable for ambient light
double ambientLight = -1000.0;
float ax, ay, az, gx, gy, gz;
int pet, feed, workout;

// JTKJ: Teht�v� 1. Lis�� painonappien RTOS-muuttujat ja alustus
static PIN_Handle buttonHandle;
static PIN_State buttonState;
static PIN_Handle ledHandle;
static PIN_State ledState;
static PIN_Handle hMpuPin;
static PIN_State  MpuPinState;

// JTKJ: Exercise 1. Add pins RTOS-variables and configuration here
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

void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
       uint_t pinValue = PIN_getOutputValue( Board_LED0 );
       pinValue = !pinValue;
       PIN_setOutputValue( ledHandle, Board_LED0, pinValue );


       if(sensorState == MOTION_SENSOR){
           sensorState = LIGHT_SENSOR;

       }else {
           sensorState = MOTION_SENSOR;
       }
    // JTKJ: Teht�v� 1. Vilkuta jompaa kumpaa ledi�
    // JTKJ: Exercise 1. Blink either led of the device
}

/* Task Functions */
//strncmp()
Void uartFxn(UART_Handle handle, void *rxBuf, size_t len){
    for (int n=0 ; n<4 ; n++)
        if (strncmp (str[n],"BEEP",4) == 0)
        {
          //MAKE MOGUS FUNCTION :)
        }
}

Void uartTaskFxn(UArg arg0, UArg arg1) {
    char string[100];
    //char debugString[100];
    // UART alustus

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
          UART_read(uart, uartBuffer, 1);
    while (1) {


        if(sensorState == MOTION_SENSOR){

            if(programState == DATA_READY){
                if(feed == 1){
                    sprintf(string, "id:3255,EAT:1\0");
                    UART_write(uart, string, 14);
                }
                if(pet == 1){
                    sprintf(string, "id:3255,PET:1\0");
                    UART_write(uart, string, 14);
                }
                if(workout == 1){
                    sprintf(string, "id:3255,EXERCISE:1\0");
                    UART_write(uart, string, 19);
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

        // JTKJ: Teht�v� 2. Lue sensorilta dataa ja tulosta se Debug-ikkunaan merkkijonona
        // JTKJ: Exercise 2. Read sensor data and print it to the Debug window as string

        // JTKJ: Teht�v� 3. Tallenna mittausarvo globaaliin muuttujaan
        //       Muista tilamuutos
        // JTKJ: Exercise 3. Save the sensor value into the global variable
        //       Remember to modify state

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
                    //sprintf(string, "%.3lf", ambientLight);
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











        // Just for sanity check for exercise, you can comment this out
       // System_printf("sensorTask\n");
        //System_printf("%s\n",string);
        //System_flush();

        // Once per second, you can modify this
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

    
    // JTKJ: Teht�v� 2. Ota i2c-v�yl� k�ytt��n ohjelmassa
    // JTKJ: Exercise 2. Initialize i2c bus
    Board_initI2C();

    // JTKJ: Teht�v� 4. Ota UART k�ytt��n ohjelmassa
    // JTKJ: Exercise 4. Initialize UART
    Board_initUART();

    // JTKJ: Teht�v� 1. Ota painonappi ja ledi ohjelman k�ytt��n
    //       Muista rekister�id� keskeytyksen k�sittelij� painonapille
    buttonHandle = PIN_open(&buttonState, buttonConfig);
       if(!buttonHandle) {
          System_abort("Error initializing button pins\n");
       }
       ledHandle = PIN_open(&ledState, ledConfig);
       if(!ledHandle) {
          System_abort("Error initializing LED pins\n");
       }
    // JTKJ: Exercise 1. Open the button and led pins
    //       Remember to register the above interrupt handler for button
       if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
             System_abort("Error registering button callback function");
          }

       hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
           if (hMpuPin == NULL) {
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

    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
