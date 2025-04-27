#include "main.h"
#include <stm32f0xx_hal.h>
#include <stdlib.h>
#include <math.h>

// create a buffer to store the characters received over UART
const uint8_t buffer_size = 6;
char buffer[6];
uint8_t buffer_index = 0;

// angle measured are fixed-point numbers with one decimal place, e.g. 450 = 45.0 degrees

// Bottom servo: 10 us per degree
uint32_t servoACenter = 1370;
uint32_t servoAMin = 0;
uint32_t servoAMax = 900;

// Top servo: 16 us per degree
uint32_t servoBMax = 800;
uint32_t servoBMin = 0;
uint32_t yAngleDivisor = 5;

void commandServoA(uint32_t angle)
{

  // transform the input from pixels to degrees. Adjust these numbers to calibrate for different locations
  angle = 150 + angle * 4;


  if (angle <= servoAMax && angle >= servoAMin)
  {
    // convert from degrees to a pulse-width in microseconds
    uint32_t pulseWidth = servoACenter + (angle - 450);
    // update timer3's compare capture register with the new value
    TIM3->CCR1 = pulseWidth;
  }
}

void commandServoB(uint32_t angle)
{
  // transform the input from pixels to degrees. Adjust these numbers to calibrate for different locations
  angle = 0 + 4 * angle;
  if (angle <= servoBMax && angle >= servoBMin)
  {
    // convert from degrees to a pulse-width in microseconds
    uint32_t pulseWidth = 2250 - angle * 16 / 10;
    // update timer3's compare capture register with the new value
    TIM3->CCR2 = pulseWidth;
  }
}

void Clear_Buffer()
{
  // loop through and set all chars in the buffer to 0
  buffer_index = 0;
  for (uint8_t i = 0; i < buffer_size; i++)
  {
    buffer[i] = 0;
  }
}

void Parse_Command()
{
  // The first char is the buffer should be an x or y for servo a or b
  uint8_t motor_selection = buffer[0];

  if (motor_selection == 'x' || motor_selection == 'y')
  {
    // Toggle pin 9 for debug purposes
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_9);
    // Read the buffer starting a the second char and save it into message

    // the message doesn't care about the first and last characters
    char message[buffer_size - 2];
    // start at teh second character, which is the start of the number
    uint8_t i = 1;

    while (buffer[i] && buffer[i] != 0x0D && i < buffer_size)
    {
      message[i - 1] = buffer[i];
      // Transmit_Char(buffer[i]);
      i++;
    }
    uint32_t angle = atoi(message);

    if (motor_selection == 'x')
    {
      commandServoA(angle);
    }
    else
    {
      commandServoB(angle);
    }
  }
  else
  {
    //Transmit_String("First char must be x or y, and number must be 3 digits\n\r");
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
  }
  Clear_Buffer();
}

void Transmit_Char(char toTransmit)
{
  while (!(USART3->ISR & 0x80))
    ;
  USART3->TDR = toTransmit;
}

void Transmit_String(char *toTransmit)
{

  while (*toTransmit)
  {
    Transmit_Char(*toTransmit);
    toTransmit++;
  }
}

void USART3_4_IRQHandler()
{
  if (USART3->ISR & 0b0100000)
  { // Read data register not empty

    // get the data out of the RDR
    char new_char = USART3->RDR;

    // Transmit the char back so it shows up on the terminal
    //Transmit_Char(new_char);

    // put the new char on the buffer
    buffer[buffer_index] = new_char;
    if (buffer_index < buffer_size - 1)
    {
      buffer_index++;
    }
    else
    {
      buffer_index = 0;
    }

    // if a newline is received
    if (new_char == 0x0A)
    {
      //Transmit_String("\n\r");
      Parse_Command();
    }
  }
}

void Initialize_GPIOs()
{
  RCC->AHBENR |= RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIOBEN;
  // Enable LED pins
  GPIO_InitTypeDef initStr1 = {GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9,
                               GPIO_MODE_OUTPUT_PP,
                               GPIO_SPEED_FREQ_LOW,
                               GPIO_NOPULL};
  HAL_GPIO_Init(GPIOC, &initStr1);

  GPIO_InitTypeDef initStr2 = {GPIO_PIN_12,
                               GPIO_MODE_OUTPUT_PP,
                               GPIO_SPEED_FREQ_LOW,
                               GPIO_NOPULL};
  HAL_GPIO_Init(GPIOB, &initStr2);

  // Turn on PC7 (blue LED) to show that the laser pointer is on
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
  // Turn on PB12 to acvivate the laser pointer
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
}

void Initialize_Servos()
{
  // Initialize PB4 as channel 1 of timer 3
  GPIO_InitTypeDef initStr2 = {GPIO_PIN_4 | GPIO_PIN_5,
                               GPIO_MODE_AF_PP,
                               GPIO_SPEED_FREQ_HIGH,
                               GPIO_NOPULL,
                               GPIO_AF1_TIM3};
  HAL_GPIO_Init(GPIOB, &initStr2);

  // Enable timer 3 in the RCC, it will be used to control servos
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
  // Set prescaler to 7 for 1 us timer period
  TIM3->PSC = 7;
  // Set ARR to 20000 to get a 50 Hz event
  TIM3->ARR = 20000;
  // Set channel 1 and channel 2 to pwm mode 1
  TIM3->CCMR1 = (0b110 << 4) | (0b110 << 12);
  // Compare/capture setting (pulse width in us)
  TIM3->CCR1 = 1370;
  TIM3->CCR2 = 2250;
  // Enable channels 1 and 2
  TIM3->CCER = 0b01 | (0b01 << 4);
  // Enable timer 3 in the CR1
  TIM3->CR1 |= 0x01;
}

void Initialize_UART()
{
  // Enable the GPIOC pins 4 and 5 to act as USART RX/TX
  GPIO_InitTypeDef initStr3 = {GPIO_PIN_4 | GPIO_PIN_5,
                               GPIO_MODE_AF_PP,
                               GPIO_SPEED_FREQ_LOW,
                               GPIO_NOPULL,
                               GPIO_AF1_USART3};

  HAL_GPIO_Init(GPIOC, &initStr3);

  // Enable the USART Interupt in the NVIC
  NVIC_EnableIRQ(USART3_4_IRQn);
  NVIC_SetPriority(USART3_4_IRQn, 0);

  // Enable the clock to USART3
  RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
  // Set BAUD rate to 115200
  USART3->BRR = HAL_RCC_GetHCLKFreq() / 115200;
  // Enable transmitter and receiver
  USART3->CR1 |= 0b101100;
  // Enable the USART3 peripheral
  USART3->CR1 |= 0b01;

  Clear_Buffer();
}

void UserButton_Interrupt_Init(void)
{

  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
  GPIO_InitTypeDef initStrA = {GPIO_PIN_0,
                               GPIO_MODE_INPUT,
                               GPIO_SPEED_FREQ_LOW,
                               GPIO_PULLDOWN};

  HAL_GPIO_Init(GPIOA, &initStrA);
  EXTI->IMR |= 0x01;
  EXTI->RTSR |= 0x01;
  SYSCFG->EXTICR[0] &= 0xFFFFFFF0;
  NVIC_EnableIRQ(EXTI0_1_IRQn);
  NVIC_SetPriority(EXTI0_1_IRQn, 0);
  NVIC_SetPriority(SysTick_IRQn, 1);
}
void EXTI0_1_IRQHandler()
{
  // Turn the laser pointer and the blue LED on or off when the user button is pressed
  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_12);
  EXTI->PR |= 0x01;
}

int main(void)
{
  // Initialize Hardware Abstraction Library
  HAL_Init();

  Initialize_GPIOs();
  Initialize_Servos();
  Initialize_UART();
  UserButton_Interrupt_Init();

  while (1)
  {
    // Blink the red LED constantly to indicate the program is still running
    HAL_Delay(500);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_6);
  }
}
