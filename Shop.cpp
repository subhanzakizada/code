#include "Shop.h"

// Initializes the synchronization primitives (mutexes and condition variables)
void Shop::initSyncPrimitives() {
    pthread_mutex_init(&mutex_, NULL);  // Mutex to manage critical sections in the shop

    // Initialize barbers with their unique IDs and condition variables
    barbers = new Barber[barber_cnt_];  // Allocate memory for the array of barbers
    for (int i = 0; i < barber_cnt_; i++) {
        barbers[i].id = i;  // Assign a unique ID to each barber
        barbers[i].in_service_ = false;  // Barbers are not serving anyone initially
        barbers[i].money_paid_ = false;  // No payments have been made initially
        pthread_cond_init(&barbers[i].barberCond, NULL);  // Condition variable for barber actions
    }

    // Initialize condition variables for customers
    for (auto& customerEntry : customers) {
        pthread_cond_init(&customerEntry.second.customerCond, NULL);
    }
}

// General initialization of the shop
void Shop::init() {
    initSyncPrimitives();  // Setup mutexes and condition variables
}

// Converts an integer to a string
string Shop::int2string(int i) {
    stringstream out;  // Stream to build the string representation
    out << i;          // Insert the integer into the stream
    return out.str();  // Extract the resulting string
}

// Retrieves the number of customers who left without being served
int Shop::getCustDrops() const {
    return nDropsOff;
}

// Outputs a message to the console, tagging whether it's from a barber or customer
void Shop::print(int person, int id, string message) {
    if (person == 0) {  // Barber message
        cout << "barber  [" << id << "]: " << message << endl;
    } else {  // Customer message
        cout << "customer[" << id << "]: " << message << endl;
    }
}

// Constructor with specified barbers and chairs
Shop::Shop(int barbers, int chairs)
    : barber_cnt_(barbers), chair_cnt_(chairs) {
    initSyncPrimitives();  // Setup shop resources
}

// Default constructor with predefined barbers and chairs
Shop::Shop()
    : barber_cnt_(kDefaultNumBarbers), chair_cnt_(kDefaultNumChairs) {
    initSyncPrimitives();  // Setup default resources
}

// Handles a customer entering the shop
int Shop::visitShop(int customerId) {
    pthread_mutex_lock(&mutex_);  // Lock shop mutex

    // If no seats are available, the customer leaves
    if ((int)waitingCustomers.size() == chair_cnt_ && sleepingBarbers.empty()) {
        print(1, customerId, "leaves the shop because of no available waiting chairs.");
        nDropsOff++;  // Record the drop-off
        pthread_mutex_unlock(&mutex_);  // Unlock before returning
        return -1;  // Indicate that the customer left
    }

    // Setup the customer's data and condition variable
    customers[customerId] = Customer();
    customers[customerId].id = customerId;
    pthread_cond_init(&customers[customerId].customerCond, NULL);

    int barberId;

    if (sleepingBarbers.empty()) {
        // No sleeping barbers, customer must wait
        waitingCustomers.push(customerId);  // Add to the waiting queue
        print(1, customerId, "takes a waiting chair. # waiting seats available = " +
            int2string(static_cast<int>(chair_cnt_ - waitingCustomers.size())));

        // Wait for a barber to signal availability
        while (customers[customerId].myBarber == -1) {
            pthread_cond_wait(&customers[customerId].customerCond, &mutex_);
        }
        barberId = customers[customerId].myBarber;
    } else {
        // Assign a sleeping barber to the customer
        barberId = sleepingBarbers.front();
        sleepingBarbers.pop();
        customers[customerId].myBarber = barberId;
        getBarber(barberId)->myCustomer = customerId;
    }

    // Move the customer to the service chair
    print(1, customerId, "moves to a service chair[" + int2string(barberId) +
        "]. # waiting seats available = " +
        int2string(static_cast<int>(chair_cnt_ - waitingCustomers.size())));

    // Update states for both customer and barber
    customers[customerId].state = "C";
    getBarber(barberId)->in_service_ = true;  // Barber begins serving
    pthread_cond_signal(&(getBarber(barberId)->barberCond));  // Wake up the barber
    pthread_mutex_unlock(&mutex_);  // Unlock mutex after handling the customer
    return barberId;  // Return assigned barber ID
}

// Handles a customer leaving after their haircut
void Shop::leaveShop(int customerId, int barberId) {
    pthread_mutex_lock(&mutex_);  // Lock shop mutex

    print(1, customerId, "wait for barber[" + int2string(barberId) + "] to be done with the hair-cut.");

    // Wait until the barber indicates the haircut is complete
    while (customers[customerId].myBarber != -1) {
        pthread_cond_wait(&customers[customerId].customerCond, &mutex_);
    }

    // Indicate the customer is leaving
    print(1, customerId, "says goodbye to barber[" + int2string(barberId) + "].");
    customers[customerId].state = "L";  // Update state to LEAVING
    getBarber(barberId)->money_paid_ = true;  // Barber marks payment as received
    pthread_cond_signal(&(getBarber(barberId)->barberCond));  // Notify barber
    pthread_mutex_unlock(&mutex_);  // Unlock mutex
}

// Retrieves the barber object for a given ID
Shop::Barber* Shop::getBarber(int barberId) {
    for (int i = 0; i < barber_cnt_; i++) {
        if (barbers[i].id == barberId) {
            return &barbers[i];  // Return the matched barber
        }
    }
    return NULL;  // Return NULL if not found
}

// Handles a barber greeting a customer and starting their haircut
void Shop::helloCustomer(int barberId) {
    pthread_mutex_lock(&mutex_);  // Lock shop mutex

    // If no customer is assigned, barber sleeps
    if (getBarber(barberId)->myCustomer == -1) {
        print(0, barberId, "sleeps because of no customers.");
        sleepingBarbers.push(barberId);  // Add to sleeping barbers

        // Wait until a customer is assigned
        while (getBarber(barberId)->myCustomer == -1) {
            pthread_cond_wait(&(getBarber(barberId)->barberCond), &mutex_);
        }
    }

    // Wait until the customer is in the service chair
    while (customers[getBarber(barberId)->myCustomer].state != "C") {
        pthread_cond_wait(&(getBarber(barberId)->barberCond), &mutex_);
    }

    // Start the haircut
    print(0, barberId, "starts a hair-cut service for customer[" +
        int2string(getBarber(barberId)->myCustomer) + "].");
    pthread_mutex_unlock(&mutex_);  // Unlock mutex
}

// Handles a barber finishing with a customer and calling in the next
void Shop::byeCustomer(int barberId) {
    pthread_mutex_lock(&mutex_);  // Lock shop mutex

    print(0, barberId, "says he's done with a hair-cut service for customer[" +
        int2string(getBarber(barberId)->myCustomer) + "].");

    // Notify the customer that the haircut is complete
    customers[getBarber(barberId)->myCustomer].myBarber = -1;
    pthread_cond_signal(&customers[getBarber(barberId)->myCustomer].customerCond);

    // Wait until payment is received
    while (!getBarber(barberId)->money_paid_) {
        pthread_cond_wait(&(getBarber(barberId)->barberCond), &mutex_);
    }

    // Reset barber state and prepare for the next customer
    getBarber(barberId)->myCustomer = -1;
    getBarber(barberId)->in_service_ = false;
    getBarber(barberId)->money_paid_ = false;

    print(0, barberId, "calls in another customer.");

    // Assign a waiting customer to the barber
    if (!waitingCustomers.empty()) {
        int customerId = waitingCustomers.front();
        waitingCustomers.pop();
        getBarber(barberId)->myCustomer = customerId;
        customers[customerId].myBarber = barberId;
        pthread_cond_signal(&customers[customerId].customerCond);  // Notify customer
    }

    pthread_mutex_unlock(&mutex_);  // Unlock mutex
}

// Destructor to clean up resources
Shop::~Shop() {
    // Free barber resources
    for (int i = 0; i < barber_cnt_; i++) {
        pthread_cond_destroy(&barbers[i].barberCond);  // Destroy barber condition variables
    }
    delete[] barbers;  // Deallocate barbers array

    // Free customer resources
    for (auto& customerEntry : customers) {
        pthread_cond_destroy(&customerEntry.second.customerCond);  // Destroy customer condition variables
    }

    pthread_mutex_destroy(&mutex_);  // Destroy shop mutex
}
