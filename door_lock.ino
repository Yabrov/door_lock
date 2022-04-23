#include <Servo.h>

#define BUTTON 1 // Пин на кнопку (pin3 aduino)
#define CONTROL 9 // Пин на сервопривод
#define SPEAKER 7 // Пин на динамик (buzzer)
#define LED_CLOSE 6 // Пин на красный светодиод
#define LED_OPEN 5  // Пин на зеленый светодиод
#define SERIAL_BAUD_RATE 9600 // Скорость serial
#define CARD_CODE_SIZE 12  // Длина кода карты
#define NUMBER_OF_CARDS 5 // Количество карт

// Массив кодов используемых (валидных) карт
uint8_t cards[NUMBER_OF_CARDS][CARD_CODE_SIZE] = {
  {'7','4','0','0','2','A','A','D','D','F','2','C'},
  {'B','1','D','3','A','B','5','1','8','4','D','C'},
  {'4','E','F','5','4','E','1','A','2','2','7','3'},
  {'A','8','8','A','7','2','3','C','A','A','5','A'},
  {'3','7','0','0','3','B','1','C','5','1','4','1'}
};

// Частоты нот мелодии открытия
const word notes[] = {500,700,1000,1500,1700,500,700,1000,1500,2000,100,500};
// Паузы между нотами
const word times[] = {500,500,500,500,700,500,500,500,500,700,100,100};

// Перечислитель статусов замка
enum LockState { LOCKED, UNLOCKED };

Servo motor;  // Сервомотор

// Буффер для отсканированной карты
uint8_t buffer[CARD_CODE_SIZE];

uint8_t ch;
word count;

/*
 * Флаг, который означает необходимость изменить статус замка.
 * Изменяется в прерывании или при поднесении валидной карты.
*/
volatile bool isActionRequired = false;

// Начальные установки
void setup() {
  motor.attach(CONTROL);
  pinMode(LED_CLOSE, OUTPUT);
  pinMode(LED_OPEN, OUTPUT);
  pinMode(SPEAKER, OUTPUT);
  pinMode(BUTTON, INPUT);
  Serial.begin(SERIAL_BAUD_RATE);
  if (getCurrentLockState() == LOCKED) {
    digitalWrite(LED_CLOSE, HIGH);
    digitalWrite(LED_OPEN, LOW);
  } else {
    digitalWrite(LED_CLOSE, LOW);
    digitalWrite(LED_OPEN, HIGH);
  }
  // Вешаем прерывание на кнопку управления
  attachInterrupt(BUTTON, requireAction, RISING);
}

// Функция для получения текущего положения замка
LockState getCurrentLockState() {
  if (motor.read() > 0) return LOCKED;
  else return UNLOCKED;
}

// Функция прерывания
void requireAction() {
  isActionRequired = true;
}

// Открыть замок
void openDoor() { 
  // Проигрывается мелодия
  for (word i = 0; i < 12; i++) {
    tone(SPEAKER, notes[i], times[i]);
    delay(times[i]/10);
  }
  digitalWrite(LED_CLOSE, LOW);
  digitalWrite(LED_OPEN, HIGH);
  // Открыть замок
  motor.write(0);  
  delay(50);
  // Скидываю флаг
  isActionRequired = false;
}

// Закрыть замок
void closeDoor() {
  // Издаю короткий бииип
  tone(SPEAKER, 1000, 200);
  delay(3000); // Задержка 3 сек.
  digitalWrite(LED_CLOSE, HIGH);
  digitalWrite(LED_OPEN, LOW);
  motor.write(85);  // Закрыть замок
  delay(50);
  // Скидываю флаг
  isActionRequired = false;
}

/*
 * Функция, проверяющая валидность
 * поднесенной к сканеру карты.
 */
bool checkCard() {
  /*
   * Небольшая пауза для того, чтобы дать
   * возможность буфферу последовательного
   * порта заполниться до конца.
   */
  delay(100);
  count = 0;
  clearBuffer();
  // Сканируем карту в buffer
  while (Serial.available()) {
    if (count > CARD_CODE_SIZE) break;
    ch = Serial.read();
    // Пропускаю символ начала передачи данных
    if (count > 0) buffer[count - 1] = ch;
    count++;
  }
  Serial.flush();
  /*
   * Поочередно, до первого совпадения, сравниваю номера
   * разрешенных карт с отсканированным номером.
   */
  for (word i = 0; i < NUMBER_OF_CARDS; i++) {
    if (compareArrays(cards[i], buffer)) {
      return true;
    }
  }
  return false;
}

// Функция сравнения двух массивов
bool compareArrays(uint8_t* arr1, uint8_t* arr2) {
  for (word i = 0; i < CARD_CODE_SIZE; i++) {
    if (arr1[i] != arr2[i]) {
      return false;
    }
  }
  return true;
}

/*
 * Функция "обнуляет", т.е. записывает в buffer символ '@',
 * который точно не может входить в сканированный номер карты.
 */
void clearBuffer() {
  for (word i = 0; i < CARD_CODE_SIZE; i++) {
    buffer[i] = '@';
  }
}

void loop() {
  if (Serial.available()) isActionRequired = checkCard();
  if (isActionRequired && getCurrentLockState() == LOCKED) openDoor();
  if (isActionRequired && getCurrentLockState() == UNLOCKED) closeDoor();
}
