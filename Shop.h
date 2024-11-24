#ifndef _SHOP_H_
#define _SHOP_H_

#include <pthread.h>
#include <queue>
#include <map>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

// Default values for chairs and barbers
#define kDefaultNumChairs 3
#define kDefaultNumBarbers 1

class Shop {
public:
    Shop(int nBarbers, int nChairs); // Initialize shop with specified barbers and chairs
    Shop(int num_chairs);            // Initialize with one barber and specified chairs
    Shop();                          // Default constructor
    ~Shop();                         // Destructor

    int visitShop(int customerId);   // Customer enters shop; returns barber ID or -1 if dropped
    void leaveShop(int customerId, int barberId); // Customer leaves after service
    void helloCustomer(int barberId); // Barber starts servicing a customer
    void byeCustomer(int barberId);   // Barber finishes service
    int getCustDrops() const;         // Get count of customers who left without service

    int nDropsOff = 0; // Number of dropped customers

private:
    const int chair_cnt_;  // Max waiting customers
    int barber_cnt_;       // Number of barbers

    struct Barber {
        int id;
        pthread_cond_t barberCond;
        int myCustomer = -1;
        bool in_service_ = false;
        bool money_paid_ = false;
    };

    struct Customer {
        int id;
        pthread_cond_t customerCond;
        string state = "W"; // W(waiting), C(on chair), L(leaving)
        int myBarber = -1;
    };

    Barber* barbers;
    map<int, Customer> customers;
    queue<int> waitingCustomers;
    queue<int> sleepingBarbers;

    pthread_mutex_t mutex_;

    Barber* getBarber(int barberId); // Find barber by ID
    string int2string(int i);        // Convert integer to string
    void print(int person, int id, string message); // Log messages

    void initSyncPrimitives(); // Initialize mutex and condition variables
    void initBarbers();        // Initialize barber objects
    void init();               // General initialization
};

#endif
