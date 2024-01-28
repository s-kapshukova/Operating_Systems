#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>

using namespace std;

mutex Mutex;
condition_variable cond_var;

bool ready = false;
int number = 0;

void Provide() {
    while (number < 10) {
        unique_lock<mutex> uniqueLock(Mutex);
        while (ready) {
            cond_var.wait(uniqueLock);
        }
        this_thread::sleep_for(chrono::seconds(1));
        number++;
        cout << "Sended: " << number << " thread_id: " << this_thread::get_id() << endl;
        ready = true;
        cond_var.notify_all();
    }
}

void Consume() {
    while (number < 10) {
        unique_lock<mutex> uniqueLock(Mutex);
        while (!ready)
        {
            cond_var.wait(uniqueLock, []{ return ready; });
        }
        cout << "Recieved: " << number << " thread_id: " << this_thread::get_id() << endl;
        ready = false;
        cond_var.notify_all();
    }
}

int main() {
    thread th1(Provide);
    thread th2(Consume);

    th1.join();
    th2.join();
}
