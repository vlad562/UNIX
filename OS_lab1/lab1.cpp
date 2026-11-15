#include <iostream>
#include <pthread.h>
#include <string>
#include <unistd.h>

using namespace std;

class Event
{
public:
  int id;
  string payload;

  Event(int id)
  {
    this->id = id;
    payload = "message " + to_string(id);
  }
};

class Monitor
{
private:
  pthread_mutex_t mutex;
  pthread_cond_t condition;
  bool ready = false;
  Event *data = nullptr;

public:
  Monitor()
  {
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&condition, nullptr);
  }

  ~Monitor()
  {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condition);
  }

  void provide(Event *event)
  {
    pthread_mutex_lock(&mutex);
    while (ready)
    {
      pthread_cond_wait(&condition, &mutex);
    }
    data = event;
    ready = true;
    cout << "Producer: отправил событие " << event->id << endl;
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);
  }

  Event *consume()
  {
    pthread_mutex_lock(&mutex);
    while (!ready)
    {
      pthread_cond_wait(&condition, &mutex);
    }
    Event *event = data;
    data = nullptr;
    cout << "Consumer: получил событие " << event->id << endl;
    ready = false;
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);
    return event;
  }
};

void *producer(void *arg)
{
  Monitor *monitor = (Monitor *)arg;
  int id = 1;
  while (true)
  {
    sleep(1);
    Event *event = new Event(id++);
    monitor->provide(event);
  }
  return nullptr;
}

void *consumer(void *arg)
{
  Monitor *monitor = (Monitor *)arg;
  while (true)
  {
    Event *event = monitor->consume();
    delete event;
  }
  return nullptr;
}

int main()
{
  Monitor monitor;
  pthread_t producerThread;
  pthread_t consumerThread;

  pthread_create(&producerThread, nullptr, producer, &monitor);
  pthread_create(&consumerThread, nullptr, consumer, &monitor);

  pthread_join(producerThread, nullptr);
  pthread_join(consumerThread, nullptr);

  return 0;
}
