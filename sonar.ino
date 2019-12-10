
#include "UTFT.h"                                      
#include "Ultrasonic.h"
#include <Servo.h>
#include <EEPROM.h>
#include "TouchScreen.h"
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
//Определяем пины для радара и серво
#define trigPin 10
#define echoPinF 11
#define echoPinB 12
#define servoPin 13

int maxDist;
#define stepDist 5                                   //шаг изменения расстояния обнаружения предметов
#define clipDist 400                                 // предельное расстояние измерения
int servoStep;                                       //шаг угла поворота серво
#define stepServo 1                                 //начальная скорость вращения серво
#define R 115                                       // радиус отрисовываемой окружности
#define Xo 165                                      // х координата центра окружности
#define Yo 120                                      // у координата центра окружности
//Определяем пины для подключения дисплея
#define RS  A2                                                                        
#define WR  A1                                          
#define CS  A3                                         
#define RST A4                                         
#define SER A0                                          
//Определяем выводы используемые для чтения данных с TouchScreen:
#define YP  A2                                          // Вывод Y+ должен быть подключен к аналоговому входу
#define XM  A3                                          // Вывод X- должен быть подключен к аналоговому входу
#define YM  8                                           // Вывод Y-
#define XP  9                                           // Вывод X+
//Определяем экстремумы для значений считываемых с аналоговых входов при определении точек нажатия на TouchScreen:
#define tsMinX   140                                    // соответствующий точке начала координат по оси X
#define tsMinY   110                                    // соответствующий точке начала координат по оси Y
#define tsMaxX   955                                    // соответствующий максимальной точке координат по оси X
#define tsMaxY   910                                    // соответствующий максимальной точке координат по оси Y
#define minPress 10                                     // соответствующий минимальной степени нажатия на TouchScreen
#define maxPress 1000                                   // соответствующий максимальной степени нажатия на TouchScreen
#define mstk 10

#define ServiceMode 0                                   // для перехода в режим отладки меняем значение на 1
/*
  Сервисный режим необходим для калибровки опорных значений
  Подробнее:
  https://wiki.iarduino.ru/page/rabota-s-touchscreen/
  раздел "Калибровка"
*/


int distF, distB, x, y;                                  //переменные для считывания информации с радаров
int degree = 0;                                          //начальный угол поворота серво
bool forward = true;                                     //флаг для проверки валидности угла поворота
int X, Y, Z;
byte r, g, b;
Servo servo;                                            //объект серводвигателя
Ultrasonic sonarF (trigPin, echoPinF);                  //объект переднего радара
Ultrasonic sonarB (trigPin, echoPinB);                  //объект заднего радара
//UTFT myGLCD(TFT28UNO, A2, A1, A3, A4, A0);            // тип дисплея 2,4  UNO  (320x240 chip ILI9341)
UTFT myGLCD(TFT28UNO, RS, WR, CS, RST, SER);            // объект для работы с дисплеем
TouchScreen ts = TouchScreen(XP, YP, XM, YM);           // объект для работы с TouchScreen

void GetDistance(float);                                //функция снятия показаний радаров и вывода данных на дисплей
void Service();                                         //функция вызова режима отладки
bool touch();                                           //функция,определяющая касание
void mainScreen();                                      //функция, определяющая вид главного экрана
void openMenu();                                        //функция вызова меню настроек

void setup()
{
  pinMode(trigPin, OUTPUT);                             // определяем trigPin как вывод
  pinMode(echoPinF, INPUT);                             // определяем echoPin как вывод
  pinMode(echoPinB, INPUT);                             // определяем echoPin как вывод
  servo.attach(servoPin);                               // инициируем порт для серво
  myGLCD.InitLCD();                                     // инициируем дисплей (в качестве параметра данной функции можно указать ориентацию дисплея: PORTRAIT или LANDSCAPE), по умолчанию LANDSCAPE - горизонтальная ориентация
  myGLCD.clrScr();                                      // стираем всю информацию с дисплея
  if (ServiceMode) {
    Serial.begin(9600);                                 // инициируем передачу данных в монитор последовательного порта на скорости 9600 бит/сек
    maxDist = EEPROM.read(4) * stepDist;                //определяем начальное максимальное расстояние воздействия при первом запуске
    Service();
  }
  r = EEPROM.read(0);                                   //берем данные о цветах из энергонезависимой памяти (цвета определяются там при первом запуске в режиме отладки)
  g = EEPROM.read(1);                                   
  b = EEPROM.read(2);
  servoStep = EEPROM.read(3);                           //берем данные о шаге угла поворота серво из эп                                                 
  maxDist = EEPROM.read(4) * stepDist;                  //берем данные о максимальном расстоянии воздействия из эп
  mainScreen();
}
void loop()
{
  if ((degree < 0) || (degree > 180)) {                 //проверка валидности угла
    forward = !forward;
  }

  myGLCD.setColor(r, g, b);                            //определяем цвет для рисования
  servo.write(degree);                                 //считываем угол поворота серво
  delay(20);
  GetDistance(degree * DEG_TO_RAD);                    
  delay(5);

  if ((touch()) && (X < 21) && (Y < 21)) openMenu();

  if (forward) {
    degree += servoStep;
  }
  else {
    degree -= servoStep;
  }
}
void GetDistance(float degree) {
  distF = constrain((sonarF.Timing() / 59.4), 0, maxDist); //считываем информацию с переднего радара
  delay(50);
  distB = constrain((sonarB.Timing() / 59.4), 0, maxDist); //считываем информацию с заднего радара

  x = (map(maxDist, 0, maxDist, 0, R) * cos(degree));      //получаем координаты для построения отрезка
  y = (map(maxDist, 0, maxDist, 0, R) * sin(degree));
  myGLCD.setColor(0, 0, 0);                                //выбираем цвет для отрисовки    
  myGLCD.drawLine(Xo, Yo, x + Xo, y + Yo);                 //по факту просто заполняем круг радиуса R черными лучами

  myGLCD.drawLine(Xo, Yo, Xo - x, Yo - y);
  myGLCD.setColor(r, g, b);                               
  x = (map(distF, 0, maxDist, 0, R) * cos(degree));
  y = (map(distF, 0, maxDist, 0, R) * sin(degree));
  myGLCD.drawLine(Xo, Yo, x + Xo, y + Yo);                //строим отрезки по данным с радаров
  x = (map(distB, 0, maxDist, 0, R) * cos(degree));
  y = (map(distB, 0, maxDist, 0, R) * sin(degree));
  myGLCD.drawLine(Xo, Yo, Xo - x, Yo - y);
}

void Service() {
  Serial.println("Xa = (analog) \t Ya = (analog)");
  Serial.println("Xd = (digital) \t Yd = (digital)");
  Serial.print("MaxDist=");
  Serial.println(maxDist);
  myGLCD.setColor(0, 200, 0);
  myGLCD.setFont(BigFont);
  myGLCD.print("Restoring Complete", CENTER, 20);
  myGLCD.setColor(200, 200, 200);
  myGLCD.print("Service", CENTER, 200);
  EEPROM.update(0, 0);                                  //вносим в энергонезависимую память данные о цветах(0-2),шаге угла поворота серво(3) и радиусе воздействия(4).
  EEPROM.update(1, 0);
  EEPROM.update(2, 255);
  EEPROM.update(3, 1);
  EEPROM.update(4, 8);

  while (true) {
    ///*
    distF = constrain((sonarF.Timing() / 59.4), 0, maxDist); //проверка работоспособности радаров, смотрим показания в Мониторе порта
    Serial.print(distF);                                     //при необходимости калибровки дисплея, комментируем данный блок,раскомменчиваем блок ниже и перепрошиваем
    Serial.print("\t");
    delay(50);
    distB = constrain((sonarB.Timing() / 59.4), 0, maxDist);
    Serial.println(distB);
    //*/

    /*
      TSPoint p = ts.getPoint();                             // Считываем координаты и интенсивность нажатия на TouchScreen в структуру p
      pinMode(XM, OUTPUT);                                   // Возвращаем режим работы вывода X- в значение «выход» для работы с дисплеем
      pinMode(YP, OUTPUT);                                   // Возвращаем режим работы вывода Y+ в значение «выход» для работы с дисплеем
      if (p.z > minPress && p.z < maxPress) {                // Если степень нажатия достаточна для фиксации координат TouchScreen
      Serial.println((String) "X = " + p.x + " \t Y = " + p.y);
      p.x = map(p.x, tsMinX, tsMaxX, 0, 320);              // Преобразуем значение p.x от диапазона tsMinX...tsMaxX, к диапазону 0...320
      p.y = map(p.y, tsMinY, tsMaxY, 0, 240);              // Преобразуем значение p.y от диапазона tsMinY...tsMaxY, к диапазону 0...240
      Serial.println((String) "X = " + p.x + " \t Y = " + p.y);
      myGLCD.fillCircle(p.x, p.y, 3);                      // Прорисовываем окружность диаметром 3 пикселя с центром в точке координат считанных с TouchScreen
      }
      //*/
  }
}

bool touch() {
  TSPoint p = ts.getPoint();                             // Считываем координаты и интенсивность нажатия на TouchScreen в структуру p
  pinMode(XM, OUTPUT);                                   // Возвращаем режим работы вывода X- в значение «выход» для работы с дисплеем
  pinMode(YP, OUTPUT);
  Z = p.z;
  // Возвращаем режим работы вывода Y+ в значение «выход» для работы с дисплеем
  if ((p.z > minPress) && (p.z < maxPress)) {
    //touchOnScreen = false;
    if (p.x > (tsMinX - mstk)) {
      X = map(p.x, tsMinX, tsMaxX, 0, 320);
      Y = map(p.y, tsMinY, tsMaxY, 0, 240);
      return true;
    }
    else return false;
  }
  else {
    return false;
  }
}

void openMenu() {
  bool menuOn = true;
  byte i, j, Re = r, Gr = g, Bl = b;
  int d = maxDist, sst = servoStep;
  byte rgb[2][3][3] {
    {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}},
    {{255, 255, 0}, {0, 255, 255}, {255, 0, 255}}
  };
  myGLCD.clrScr();                                         //очистка дисплея
  myGLCD.setFont(BigFont);                                //выбор шрифта  
  myGLCD.setColor(200, 200, 200);
  myGLCD.print("MENU", CENTER, 5);
  myGLCD.print("Save", 250, 221, 0);
  myGLCD.print("Cancel", 5, 221, 0);
  myGLCD.print("Color", 5, 30, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("(choose)", 5, 50, 0);
  myGLCD.print("CURRENT", 225, 95, 0);
  myGLCD.setFont(BigFont);
  myGLCD.print("MaxDist", 5, 115, 0);
  //-
  myGLCD.drawRect(135, 115, 165, 145);                 //отрисовываем кнопки
  myGLCD.fillRect(138, 128, 162, 132);
  //dst
  myGLCD.drawRect(165, 115, 225, 145);
  //+
  myGLCD.drawRect(225, 115, 255, 145);
  myGLCD.fillRect(228, 128, 252, 132);
  myGLCD.fillRect(238, 117, 242, 142);
  myGLCD.setFont(BigFont);
  myGLCD.print("Servo", 5, 155, 0);
  myGLCD.print("step", 5, 180, 0);

  //-
  myGLCD.drawRect(135, 165, 165, 195);
  myGLCD.fillRect(138, 178, 162, 182);
  //dst
  myGLCD.drawRect(165, 165, 225, 195);
  //+
  myGLCD.drawRect(225, 165, 255, 195);
  myGLCD.fillRect(228, 178, 252, 182);
  myGLCD.fillRect(238, 167, 242, 192);

  myGLCD.drawRoundRect(245, 219, 318, 239);
  myGLCD.drawRoundRect(0, 219, 103, 239);
  myGLCD.setColor(Re, Gr, Bl);
  myGLCD.fillCircle(250, 60, 30);
  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRect(167, 116, 223, 144);
  myGLCD.fillRect(167, 166, 223, 174);
  myGLCD.setColor(200, 200, 200);
  myGLCD.setFont(BigFont);
  myGLCD.printNumI(d, 170, 122);                                                   //выводим на дисплей значения maxDist и servoStep 
  myGLCD.printNumI(sst, 170, 172);
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 2; j++) {
      myGLCD.setColor(rgb[j][i][0], rgb[j][i][1], rgb[j][i][2]);
      myGLCD.fillRect(100 + (i * 30), 30 + (j * 30), 130 + (i * 30), 60 + (j * 30)); //выводим закрашенный прямоугольник
    }
  }

  while (menuOn) {
    if ((touch()) && (Y > 219) && ((Z > minPress) && (Z < maxPress))) {
      if (X < 103) menuOn = false;                                                 //закрываем меню                                             
      else {
        if (X > 245) {                                                             //нажатие на кнопку "save"->сохранение новых настроек 
          menuOn = false;
          myGLCD.setColor(127, 255, 0);
          myGLCD.print("Saved", 235, 200, 0);
          r = Re;
          g = Gr;
          b = Bl;
          servoStep = sst;
          maxDist = d;
          EEPROM.update(0, r);
          EEPROM.update(1, g);
          EEPROM.update(2, b);
          EEPROM.update(3, servoStep);
          EEPROM.update(4, (maxDist / stepDist));
          myGLCD.setColor(r, g, b);
          delay(500);
        }
      }
    }
    else {
      if ((Z > minPress) && (Z < maxPress)) {                                    //в этом блоке путем проверки координат места нажатия делаем выбор цвета вывода данных с радара
        switch (Y) {
          case 30 ... 90:
            switch (Y) {
              case 30 ... 60:
                switch (X) {
                  case 100 ... 130:
                    Re = rgb[0][0][0];
                    Gr = rgb[0][0][1];
                    Bl = rgb[0][0][2];
                    break;
                  case 131 ... 160:
                    Re = rgb[0][1][0];
                    Gr = rgb[0][1][1];
                    Bl = rgb[0][1][2];
                    break;
                  case 162 ... 190:
                    Re = rgb[0][2][0];
                    Gr = rgb[0][2][1];
                    Bl = rgb[0][2][2];
                    break;
                }
                break;
              case 61 ... 90:
                switch (X) {
                  case 100 ... 130:
                    Re = rgb[1][0][0];
                    Gr = rgb[1][0][1];
                    Bl = rgb[1][0][2];
                    break;
                  case 131 ... 160:
                    Re = rgb[1][1][0];
                    Gr = rgb[1][1][1];
                    Bl = rgb[1][1][2];
                    break;
                  case 162 ... 190:
                    Re = rgb[1][2][0];
                    Gr = rgb[1][2][1];
                    Bl = rgb[1][2][2];
                    break;
                }
                break;
            }

            break;
          case 115 ... 145:                                                            //в этом блоке путем проверки координат нажатия делаем выбор скорости вращения серво
            switch (X) {
              case 135 ... 165:
                if (d >= stepDist) d -= stepDist;
                break;
              case 225 ... 255:
                if ((d + stepDist) <= clipDist) d += stepDist;
                break;
            }
            break;
          case 165 ... 195:
            switch (X) {
              case 135 ... 165:
                if (sst > stepServo) sst -= stepServo;
                break;
              case 225 ... 255:
                if ((sst + stepServo) <= 180) sst += stepServo;
                break;
            }
            break;
        }
        myGLCD.setColor(Re, Gr, Bl);                                              
        myGLCD.fillCircle(250, 60, 30);
        myGLCD.setColor(0, 0, 0);
        myGLCD.fillRect(167, 116, 223, 144);
        myGLCD.fillRect(167, 166, 223, 174);
        myGLCD.setColor(200, 200, 200);
        myGLCD.setFont(BigFont);
        myGLCD.printNumI(d, 170, 122);                                              //вывод новых настроек
        myGLCD.printNumI(sst, 170, 172);                                            
        delay(150);
      }
    }
  }
  degree = 0;                                                                      //задаем переменным значения для начала новой работы
  forward = true;
  mainScreen();
}

void mainScreen() {
  myGLCD.InitLCD();
  myGLCD.clrScr();
  myGLCD.setColor(200, 200, 200);
  myGLCD.drawRoundRect(0, 0, 21, 21);
  myGLCD.fillRect(3, 4, 18, 6);
  myGLCD.fillRect(3, 9, 18, 12);
  myGLCD.fillRect(3, 15, 18, 17);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Max", 5, 214, 0);
  myGLCD.print("Dist:", 5, 225, 0);
  myGLCD.printNumI(maxDist, 43, 225);
  myGLCD.setColor(r, g, b);                          // Устанавливаем цвет
  myGLCD.drawCircle(Xo, Yo, R);                      // рисуем окружность (с центром в точке (160,120) и радиусом 115)

}
