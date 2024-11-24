#ifndef _SHOP_H_
#define _SHOP_H_
#include <pthread.h> // Header file for the pthread library
#include <queue>     // STL queue for managing waiting customers
#include <map>       // STL map for managing customer data
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

// Default values for the number of chairs and barbers in the shop
#define kDefaultNumChairs 3
#define kDefaultNumBarbers 1

class Shop {
public:
    // Constructors
    Shop(int nBarbers, int nChairs); // Initializes the shop with the specified number of barbers and chairs
    Shop(int num_chairs);            // Initializes the shop with one barber and the specified number of chairs
    Shop();                          // Default constructor initializes with default barbers and chairs
    ~Shop();                         // Destructor to clean up resources

    // Customer functions
    int visitShop(int customerId);   // Customer visits the shop; returns barber ID if serviced, -1 if dropped
    void leaveShop(int customerId, int barberId); // Customer leaves the shop after being serviced

    // Barber functions
    void helloCustomer(int barberId); // Barber greets and starts servicing a customer
    void byeCustomer(int barberId);   // Barber finishes servicing a customer

    int getCustDrops() const; // Retrieve the number of customers who left due to no available space

    // Keeps track of the number of customers who couldn't wait and left the shop
    int nDropsOff = 0;

private:
    const int chair_cnt_;  // Maximum number of customers that can wait in the shop
    int barber_cnt_;       // Number of barbers available in the shop

    // Struct to represent a barber
    struct Barber {
        int id;                            // Barber ID
        pthread_cond_t barberCond;         // Condition variable for this barber
        int myCustomer = -1;               // ID of the current customer being serviced (-1 if none)
        bool in_service_ = false;          // Whether the barber is currently servicing a customer
        bool money_paid_ = false;          // Whether the barber has been paid for the current service
    };

    // Struct to represent a customer
    struct Customer {
        int id;                            // Customer ID
        pthread_cond_t customerCond;       // Condition variable for this customer
        string state = "W";        // W(waiting), C(on chair), L(leaving)
        int myBarber = -1;                 // ID of the assigned barber (-1 if none)
    };

    Barber* barbers;              // Array of barber objects
    map<int, Customer> customers; // Map of customer ID to customer objects for tracking

    queue<int> waitingCustomers;  // Queue of customer IDs waiting for service
    queue<int> sleepingBarbers;   // Queue of barber IDs waiting for customers

    pthread_mutex_t mutex_; // Mutex for synchronizing access to shared resources

    static const int barberId = 0; // Default barber ID (used for a single barber scenario)

    // Utility functions
    Barber* getBarber(int barberId); // Retrieve a pointer to a barber object by ID
    string int2string(int i);        // Convert an integer to a string
    void print(int person, int id, string message); // Print a log message for debugging or tracking

    // Initialization functions
    void initSyncPrimitives(); // Initialize synchronization primitives (mutex and condition variables)
    void initBarbers();        // Initialize barber objects and their condition variables
    void init();               // Perform overall initialization by calling other init functions
};

#endif




