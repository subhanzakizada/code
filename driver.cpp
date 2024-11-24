#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include "Shop.h"

using namespace std;

void *barber(void *);    
void *customer(void *);  

// A class to pass parameters to threads
class ThreadParam {
public:
    ThreadParam(Shop *shop, int id, int serviceTime) :
        shop(shop), id(id), serviceTime(serviceTime) {}
    Shop *shop;          // Pointer to the Shop object
    int id;              // Thread identifier
    int serviceTime;     // Service time in microseconds (0 for customers)
};

int main(int argc, char *argv[]) {
    // Validate the number of command-line arguments
    if (argc != 5) {
        cerr << "Usage: sleepingBarber nBarbers nChairs nCustomers serviceTime" << endl;
        return -1;
    }

    // Parse and validate command-line arguments
    int nBarbers = atoi(argv[1]);
    int nChairs = atoi(argv[2]);
    int nCustomers = atoi(argv[3]);
    int serviceTime = atoi(argv[4]);

    if (nBarbers <= 0) {
        cerr << "Error: Number of barbers must be greater than 0." << endl;
        return -1;
    }
    if (nChairs < 0) {
        cerr << "Error: Number of chairs cannot be negative." << endl;
        return -1;
    }
    if (nCustomers <= 0) {
        cerr << "Error: Number of customers must be greater than 0." << endl;
        return -1;
    }
    if (serviceTime <= 0) {
        cerr << "Error: Service time must be greater than 0." << endl;
        return -1;
    }

    pthread_t barberThreads[nBarbers];
    pthread_t customerThreads[nCustomers];
    Shop shop(nBarbers, nChairs);  // Instantiate the shop

    // Create barber threads
    for (int i = 0; i < nBarbers; i++) {
        ThreadParam *param = new ThreadParam(&shop, i, serviceTime);
        if (pthread_create(&barberThreads[i], nullptr, barber, param) != 0) {
            cerr << "Error: Failed to create barber thread " << i << "." << endl;
            delete param;
            return -1;
        }
    }

    // Create customer threads
    for (int i = 0; i < nCustomers; i++) {
        usleep(rand() % 1000);  // Random delay before creating each customer
        ThreadParam *param = new ThreadParam(&shop, i + 1, 0);
        if (pthread_create(&customerThreads[i], nullptr, customer, param) != 0) {
            cerr << "Error: Failed to create customer thread " << i + 1 << "." << endl;
            delete param;
            continue;  // Skip this customer and proceed with the next
        }
    }

    // Wait for all customer threads to finish
    for (int i = 0; i < nCustomers; i++) {
        if (pthread_join(customerThreads[i], nullptr) != 0) {
            cerr << "Warning: Failed to join customer thread " << i + 1 << "." << endl;
        }
    }

    // Terminate barber threads
    for (int i = 0; i < nBarbers; i++) {
        if (pthread_cancel(barberThreads[i]) != 0) {
            cerr << "Warning: Failed to cancel barber thread " << i << "." << endl;
        }
    }

    // Print the number of customers who didn't receive service
    cout << "# customers who didn't receive a service = " << shop.nDropsOff << endl;

    return 0;
}

void *barber(void *arg) {
    ThreadParam &param = *(ThreadParam *)arg;
    Shop &shop = *(param.shop);
    int id = param.id;
    int serviceTime = param.serviceTime;
    delete &param;

    // Barber's work loop
    while (true) {
        shop.helloCustomer(id);  // Signal readiness to serve a customer
        usleep(serviceTime);     // Simulate haircut service time
        shop.byeCustomer(id);    // Signal service completion
    }
    return nullptr;
}

void *customer(void *arg) {
    ThreadParam &param = *(ThreadParam *)arg;
    Shop &shop = *(param.shop);
    int id = param.id;
    delete &param;

    // Customer's visit to the shop
    int barber = shop.visitShop(id);
    if (barber != -1) {  // If assigned a barber
        shop.leaveShop(id, barber);  // Wait for service to finish
    } else {
        cerr << "Customer " << id << " couldn't get a haircut due to full capacity." << endl;
    }
    return nullptr;
}
