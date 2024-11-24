#include "Shop.h"

// Initializes the shop by creating barbers, setting initial conditions, and initializing mutexes
void Shop::init() {
    // Initialize mutex to handle synchronization between threads
    pthread_mutex_init(&mutex_, NULL);

    // Initialize barber objects and their condition variables
    barbers = new Barber[nBarbers];  // Dynamically allocate an array of barbers
    for (int i = 0; i < nBarbers; i++) {
        barbers[i].id = i;  // Assign each barber a unique ID
        barbers[i].in_service_ = false;  // Initially, no barber is in service
        barbers[i].money_paid_ = false;  // Initially, no customer has paid
        pthread_cond_init(&barbers[i].barberCond, NULL);  // Initialize barber condition variables
    }
}

// Converts an integer to a string
string Shop::int2string(int i) {
    stringstream out;  // Use stringstream to convert int to string
    out << i;  // Write the integer to the stream
    return out.str();  // Return the string representation
}

// Returns the number of customers who have dropped off (left the shop without service)
int Shop::getCustDrops() const {
    return nDropsOff;
}

// Prints a message with the appropriate prefix based on whether the person is a barber or customer
void Shop::print(int person, int id, string message) {
    if(person == 0) {  // If the person is a barber (person == 0)
        cout << "barber[" << id << "]: " << message << endl;  
    } else {  // If the person is a customer
        cout << "customer[" << id << "]: " << message << endl;  
    }
}

// Constructor that takes the number of barbers and chairs
Shop::Shop(int nBarbers, int nChairs)
    : nBarbers(nBarbers), nChairs(nChairs) {
    init();  // Initialize the shop with given values
}

// Default constructor, initializes the shop with default number of barbers and chairs
Shop::Shop()
    : nBarbers(kDefaultNumBarbers), nChairs(kDefaultNumChairs) {
    init();  // Initialize the shop with default values
}

// Function to handle a customer visiting the shop
int Shop::visitShop(int customerId) {
    pthread_mutex_lock(&mutex_);  // Lock the mutex to enter critical section

    // If there are no available chairs and no sleeping barbers, the customer leaves
    if ((int)waitingCustomers.size() == nChairs && sleepingBarbers.empty()) {
        print(1, customerId, "leaves the shop because of no available waiting chairs.");
        nDropsOff++;  // Increment the number of customers who left
        pthread_mutex_unlock(&mutex_);  // Unlock the mutex before returning
        return -1;  // Customer leaves
    }

    // Initialize the customer and their condition variable
    customers[customerId] = Customer();
    customers[customerId].id = customerId;
    pthread_cond_init(&customers[customerId].customerCond, NULL);

    int barberId;

    // If there are no sleeping barbers, the customer has to wait
    if (sleepingBarbers.empty()) {
        waitingCustomers.push(customerId);  // Add customer to the waiting queue
        print(1, customerId, "takes a waiting chair. # waiting seats available = " + 
            int2string(static_cast<int>(nChairs - waitingCustomers.size())));

        // Wait for the barber to become available (barber assigned to the customer)
        while (customers[customerId].myBarber == -1) {
            pthread_cond_wait(&customers[customerId].customerCond, &mutex_);
        }
        barberId = customers[customerId].myBarber;
    }
    else {
        // If there are sleeping barbers, assign one to the customer
        barberId = sleepingBarbers.front();
        sleepingBarbers.pop();
        customers[customerId].myBarber = barberId;
        getBarber(barberId)->myCustomer = customerId;  // Barber starts servicing the customer
    }

    // Print message to indicate the customer has moved to the service chair
    print(1, customerId, "moves to a service chair[" + int2string(barberId) + 
        "]. # waiting seats available = " + 
        int2string(static_cast<int>(nChairs - waitingCustomers.size())));

    // Update the customer's state and barber's status
    customers[customerId].state = CHAIR;
    getBarber(barberId)->in_service_ = true;  // Barber starts working on this customer
    pthread_cond_signal(&(getBarber(barberId)->barberCond));  // Signal the barber to start working
    pthread_mutex_unlock(&mutex_);  // Unlock the mutex before returning
    return barberId;  // Return the barber assigned to the customer
}

// Function to handle the customer leaving the shop after service
void Shop::leaveShop(int customerId, int barberId) {
    pthread_mutex_lock(&mutex_);  // Lock the mutex to enter critical section

    // Print message indicating that the customer is waiting for the barber to finish
    print(1, customerId, "wait for barber[" + int2string(barberId) + "] to be done with the hair-cut.");

    // Wait until the barber finishes the haircut (customer state is updated)
    while (customers[customerId].myBarber != -1) {
        pthread_cond_wait(&customers[customerId].customerCond, &mutex_);
    }

    // Print message when the customer says goodbye to the barber
    print(1, customerId, "says goodbye to barber[" + to_string(barberId) + "].");

    // Change the customer's state to LEAVING
    customers[customerId].state = LEAVING;

    // Mark the customer as having paid the barber
    getBarber(barberId)->money_paid_ = true;

    // Signal the barber that the customer has finished paying
    pthread_cond_signal(&(getBarber(barberId)->barberCond));

    pthread_mutex_unlock(&mutex_);  // Unlock the mutex before returning
}

// Function to handle the barber greeting and starting a haircut for a customer
void Shop::helloCustomer(int barberId) {
    pthread_mutex_lock(&mutex_);  // Lock the mutex to enter critical section

    // If the barber has no customer, they go to sleep
    if (getBarber(barberId)->myCustomer == -1) {
        print(0, barberId, "sleeps because of no customers.");
        sleepingBarbers.push(barberId);  // Add the barber to the list of sleeping barbers

        // Wait until the barber gets a customer
        while (getBarber(barberId)->myCustomer == -1) {
            pthread_cond_wait(&(getBarber(barberId)->barberCond), &mutex_);
        }
    }

    // Wait until the customer is seated in the service chair
    while (customers[getBarber(barberId)->myCustomer].state != CHAIR) {
        pthread_cond_wait(&(getBarber(barberId)->barberCond), &mutex_);
    }

    // Print message indicating the barber starts the haircut
    print(0, barberId, "starts a hair-cut service for customer[" + int2string(getBarber(barberId)->myCustomer) + "].");

    pthread_mutex_unlock(&mutex_);  // Unlock the mutex before returning
}

// Function to handle the barber finishing the haircut and calling in another customer
void Shop::byeCustomer(int barberId) {
    pthread_mutex_lock(&mutex_);  // Lock the mutex to enter critical section

    // Print message indicating that the barber is done with the customer
    print(0, barberId, "says he's done with a hair-cut service for customer[" + int2string(getBarber(barberId)->myCustomer) + "].");

    // Update the customer's information and signal them that the haircut is finished
    customers[getBarber(barberId)->myCustomer].myBarber = -1;
    pthread_cond_signal(&customers[getBarber(barberId)->myCustomer].customerCond);

    // Wait until the customer has paid the barber
    while (!getBarber(barberId)->money_paid_) {
        pthread_cond_wait(&(getBarber(barberId)->barberCond), &mutex_);
    }

    // Reset barber's status after serving the customer
    getBarber(barberId)->myCustomer = -1;
    getBarber(barberId)->in_service_ = false;  // Barber is free
    getBarber(barberId)->money_paid_ = false;  // Payment status reset

    // Print message indicating the barber is ready for the next customer
    print(0, barberId, "calls in another customer.");

    // If there are customers waiting, assign one to the barber
    if (!waitingCustomers.empty())
    {
        int customerId = waitingCustomers.front();
        waitingCustomers.pop();
        getBarber(barberId)->myCustomer = customerId;
        customers[customerId].myBarber = barberId;
        pthread_cond_signal(&customers[customerId].customerCond);  // Signal the customer to be serviced
    }

    pthread_mutex_unlock(&mutex_);  // Unlock the mutex before returning
}

// Function to get the barber object based on the barber ID
Shop::Barber* Shop::getBarber(int barberId) {
    for (int i = 0; i < nBarbers; i++) {
        if (barbers[i].id == barberId) {
            return &barbers[i];  // Return the barber with the matching ID
        }
    }
    return NULL;  // Barber not found, return NULL
}

// Destructor to clean up memory allocated for the barbers
Shop::~Shop() {
    delete[] barbers;  // Free dynamically allocated memory for barbers
    pthread_mutex_destroy(&mutex_);  // Destroy the mutex before exiting
}
