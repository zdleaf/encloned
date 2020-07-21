#include <thread>
#include <iostream>
#include <chrono>
#include <mutex>
#include <atomic>

// g++ thread-test.cpp -o thread -std=c++17 -pthread

std::mutex mu;

std::atomic<bool> stopFlag;

void f1()
{
    mu.lock(); std::cout<<"f1 executing"<<std::endl; mu.unlock();
    while(!stopFlag){
        mu.lock(); std::cout<<"."<<std::endl; mu.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void f2()
{
    mu.lock(); std::cout<<"f2 executing"<<std::endl; mu.unlock();
    while(!stopFlag){
        mu.lock(); std::cout<<"#"<<std::endl; mu.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main(){
    stopFlag = false;
    std::thread t1{f1};
    std::thread t2{f2};

    t1.detach(); // detach thread, we do not have to wait until thread ends (daemon thread)
    t2.detach();

    std::this_thread::sleep_for(std::chrono::seconds(8));
    mu.lock(); std::cout<<"Setting stopFlag to false"<<std::endl; mu.unlock();
    stopFlag = true;

    //t1.join(); // join waits for thread execution to finish
    //t2.join();
    return 0;
}