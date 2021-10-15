// Подключение библиотек
#include "thread.h"
#include "periph/gpio.h"
#include "xtimer.h"
#include "msg.h"

// Выделение памяти под стеки двух тредов

char thread_one_stack[THREAD_STACKSIZE_DEFAULT];
char thread_two_stack[THREAD_STACKSIZE_DEFAULT];
int c = 1;
int w = 1000000;
int q = 1;

// Глобально объявляется структура, которая будет хранить PID (идентификатор треда)
// Нам нужно знать этот идентификатор, чтобы посылать сообщения в тред
kernel_pid_t thread_one_pid;
kernel_pid_t thread_two_pid;

msg_t rcv_queue[8];

// Обработчик прерывания с кнопки
void btn_handler(void *arg)
{
  // Прием аргументов из главного потока
  (void)arg;
  // Создание объекта для хранения сообщения
  msg_t msg;
  msg_t msg2;
  // Поскольку прерывание вызывается и на фронте, и на спаде, нам нужно выяснить, что именно вызвало обработчик
  // для этого читаем значение пина с кнопкой. Если прочитали высокое состояние - то это был фронт.  
  if (gpio_read(GPIO_PIN(PORT_A,0)) > 0){
    // Отправка сообщения в тред по его PID
    // Сообщение остается пустым, поскольку нам сейчас важен только сам факт его наличия, и данные мы не передаем
    msg.content.value = c;
    msg2.content.value = 100000;
    msg_send(&msg, thread_one_pid); 
    msg_send(&msg2, thread_two_pid); 
    if (c < 5)
      c = c + 1;
    else 
      c = 1;  
    if (q < 9)
      q = q + 1;
    else 
    {
      q = 1;
      w = 1000000;
    }
  }
  xtimer_usleep(100000);
}

// Первый поток
void *thread_one(void *arg)
{
  // Прием аргументов из главного потока
  (void) arg;
  // Создание объекта для хранения сообщения 
  msg_t msg;
  // Инициализация пина PC8 на выход
  gpio_init(GPIO_PIN(PORT_C,8),GPIO_OUT);
  while(1){
    // Ждем, пока придет сообщение
    // msg_receive - это блокирующая операция, поэтому задача зависнет в этом месте, пока не придет сообщение
  	msg_receive(&msg);
    int p = msg.content.value;
    for (int i = 0; i < p * 2; ++i)
    {
      gpio_toggle(GPIO_PIN(PORT_C, 8));
      xtimer_usleep(600000);
    }
  }
  // Хотя код до сюда не должен дойти, написать возврат NULL все же обязательно надо    
  return NULL;
}

// Второй поток
void *thread_two(void *arg)
{

  // Прием аргументов из главного потока
  (void) arg;
  // Инициализация пина PC9 на выход
  gpio_init(GPIO_PIN(PORT_C,9),GPIO_OUT);
  msg_init_queue(rcv_queue, 8);
  msg_t msg2;
  while (1) 
  {
  if (msg_avail())
    {
      msg_receive(&msg2);
      w = w - msg2.content.value;
    }

    // Включаем светодиод
  	gpio_set(GPIO_PIN(PORT_C,9));
    // Задача засыпает на 333333 микросекунды
   	xtimer_usleep(w);
    gpio_clear(GPIO_PIN(PORT_C,9));
    xtimer_usleep(w);
  }  
  return NULL;
}


int main(void)
{
  // Инициализация прерывания с кнопки (подробнее в примере 02button)
	gpio_init_int(GPIO_PIN(PORT_A,0),GPIO_IN,GPIO_BOTH,btn_handler,NULL);

  // Создение потоков (подробнее в примере 03threads)
  // Для первого потока мы сохраняем себе то, что возвращает функция thread_create,
  // чтобы потом пользоваться этим как идентификатором потока для отправки ему сообщений
  thread_one_pid = thread_create(thread_one_stack, sizeof(thread_one_stack),
                  THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
                  thread_one, NULL, "thread_one");
  // обратите внимание, что у потоков разный приоритет: на 1 и на 2 выше, чем у main
  thread_two_pid = thread_create(thread_two_stack, sizeof(thread_two_stack),
                  THREAD_PRIORITY_MAIN - 2, THREAD_CREATE_STACKTEST,
                  thread_two, NULL, "thread_two");

  while (1){

    }

    return 0;
}

/* 
  Задание 1: Добавьте в код подавление дребезга кнопки
  Задание 2: Сделайте так, чтобы из прерывания по нажатию кнопки в поток thread_one передавалось целое число,
              которое означает, сколько раз должен моргнуть светодиод на пине PC8 после нажатия кнопки.
              После каждого нажатия циклически инкрементируйте значение от 1 до 5.
              Передать значение в сообщении можно через поле msg.content.value
              ............сделан вариант с тем что светодиод мигает столько сколько раз нажата кнопка (до пяти раз после счётчик сбрасывается)


  Задание 3: Сделайте так, чтобы из прерывания по отпусканию кнопки в поток thread_two передавалось целое число,
              которое задает значение интервала между морганиями светодиода на пине PC9. 
              Моргание светодиода не должно останавливаться.
              После каждого нажатия циклически декрементируйте значение от 1000000 до 100000 с шагом 100000.
              Чтобы послать сообщение асинхронно и без блокирования принимающего потока, нужно воспользоваться очередью. 
              Под очередь нужно выделить память в начале программы так: static msg_t rcv_queue[8];
              Затем в принимающем потоке нужно ее инициализировать так: msg_init_queue(rcv_queue, 8);
              Поток может проверять, лежит ли что-то в очереди, это делается функцией msg_avail(), 
              которая возвращает количество элементов в очереди. 

  Что поизучать: https://riot-os.org/api/group__core__msg.html
*/