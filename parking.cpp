#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <cmath>

using namespace std;

class Vehicle {
private:
    string vehicleNumber;
    string vehicleType;
    long long entryTime;

public:
    Vehicle() : entryTime(0) {}

    Vehicle(const string& number, const string& type, long long time)
        : vehicleNumber(number), vehicleType(type), entryTime(time) {}

    string getVehicleNumber() const { return vehicleNumber; }
    string getVehicleType() const { return vehicleType; }
    long long getEntryTime() const { return entryTime; }

    void display() const {
        cout << left << setw(16) << vehicleNumber
             << setw(12) << vehicleType
             << formatTime(entryTime) << '\n';
    }

    static string formatTime(long long rawTime) {
        time_t value = static_cast<time_t>(rawTime);
        tm* timeInfo = localtime(&value);
        if (timeInfo == nullptr) return "Unknown";
        ostringstream output;
        output << put_time(timeInfo, "%d-%m-%Y %H:%M");
        return output.str();
    }
};

class ParkingLot {
private:
    vector<Vehicle> parkedVehicles;
    const int capacity;
    const string activeFile;
    const string historyFile;

    static const int BIKE_RATE = 15;
    static const int CAR_RATE = 30;

    long long getCurrentTime() const {
        return chrono::system_clock::to_time_t(chrono::system_clock::now());
    }

    int findVehicle(const string& number) const {
        for (size_t i = 0; i < parkedVehicles.size(); ++i) {
            if (parkedVehicles[i].getVehicleNumber() == number)
                return static_cast<int>(i);
        }
        return -1;
    }

    void saveActiveVehicles() const {
        ofstream file(activeFile);
        if (!file) throw runtime_error("Unable to save active parking records.");
        for (const Vehicle& vehicle : parkedVehicles) {
            file << vehicle.getVehicleNumber() << '|'
                 << vehicle.getVehicleType() << '|'
                 << vehicle.getEntryTime() << '\n';
        }
    }

    void loadActiveVehicles() {
        ifstream file(activeFile);
        if (!file) return; // First run: file does not exist yet.

        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;
            string number, type, timeText;
            stringstream row(line);
            if (getline(row, number, '|') &&
                getline(row, type, '|') &&
                getline(row, timeText)) {
                try {
                    long long entry = stoll(timeText);
                    if (!number.empty() && (type == "Car" || type == "Bike") &&
                        findVehicle(number) == -1 &&
                        static_cast<int>(parkedVehicles.size()) < capacity) {
                        parkedVehicles.emplace_back(number, type, entry);
                    }
                } catch (const exception&) {
                    // Ignore malformed records and continue loading valid records.
                }
            }
        }
    }

    void appendHistory(const Vehicle& vehicle, long long exitTime,
                       long long hours, int fee) const {
        ofstream file(historyFile, ios::app);
        if (!file) throw runtime_error("Unable to save parking history.");
        file << vehicle.getVehicleNumber() << '|'
             << vehicle.getVehicleType() << '|'
             << Vehicle::formatTime(vehicle.getEntryTime()) << '|'
             << Vehicle::formatTime(exitTime) << '|'
             << hours << '|' << fee << '\n';
    }

    int getRate(const string& type) const {
        return (type == "Car") ? CAR_RATE : BIKE_RATE;
    }

public:
    ParkingLot(int maxCapacity = 10,
               const string& active = "parked_vehicles.txt",
               const string& history = "parking_history.txt")
        : capacity(maxCapacity), activeFile(active), historyFile(history) {
        loadActiveVehicles();
    }

    void parkVehicle(const string& number, const string& type) {
        if (number.empty()) throw invalid_argument("Vehicle number cannot be empty.");
        if (type != "Car" && type != "Bike")
            throw invalid_argument("Vehicle type must be Car or Bike.");
        if (findVehicle(number) != -1)
            throw invalid_argument("This vehicle is already parked.");
        if (static_cast<int>(parkedVehicles.size()) >= capacity)
            throw runtime_error("Parking lot is full. No entry allowed.");

        Vehicle newVehicle(number, type, getCurrentTime());
        parkedVehicles.push_back(newVehicle);
        saveActiveVehicles();
        cout << "Vehicle parked successfully. Entry time: "
             << Vehicle::formatTime(newVehicle.getEntryTime()) << '\n';
    }

    void displayParkedVehicles() const {
        if (parkedVehicles.empty()) {
            cout << "No vehicles are currently parked.\n";
            return;
        }
        cout << "\n" << left << setw(16) << "Vehicle Number"
             << setw(12) << "Type" << "Entry Time\n";
        cout << string(52, '-') << '\n';
        for (const Vehicle& vehicle : parkedVehicles) vehicle.display();
    }

    void showSlots() const {
        int occupied = static_cast<int>(parkedVehicles.size());
        cout << "Total slots: " << capacity
             << " | Occupied: " << occupied
             << " | Available: " << capacity - occupied << '\n';
    }

    void vehicleExit(const string& number) {
        int index = findVehicle(number);
        if (index == -1)
            throw invalid_argument("Vehicle not found. Exit is not allowed.");

        Vehicle vehicle = parkedVehicles[index];
        long long exitTime = getCurrentTime();
        long long seconds = max(0LL, exitTime - vehicle.getEntryTime());
        long long hours = max(1LL, (seconds + 3599) / 3600);
        int fee = static_cast<int>(hours) * getRate(vehicle.getVehicleType());

        appendHistory(vehicle, exitTime, hours, fee);
        parkedVehicles.erase(parkedVehicles.begin() + index);
        saveActiveVehicles();

        cout << "\nVehicle exit recorded successfully.\n";
        cout << "Vehicle number : " << vehicle.getVehicleNumber() << '\n';
        cout << "Vehicle type   : " << vehicle.getVehicleType() << '\n';
        cout << "Entry time     : " << Vehicle::formatTime(vehicle.getEntryTime()) << '\n';
        cout << "Exit time      : " << Vehicle::formatTime(exitTime) << '\n';
        cout << "Charged hours  : " << hours << '\n';
        cout << "Parking fee    : Rs. " << fee << '\n';
    }

    void displayHistory() const {
        ifstream file(historyFile);
        if (!file) {
            cout << "No parking history available.\n";
            return;
        }
        cout << "\nVehicle | Type | Entry | Exit | Hours | Fee (Rs.)\n";
        cout << string(75, '-') << '\n';
        string line;
        bool found = false;
        while (getline(file, line)) {
            if (!line.empty()) {
                found = true;
                replace(line.begin(), line.end(), '|', ' ');
                cout << line << '\n';
            }
        }
        if (!found) cout << "No parking history available.\n";
    }
};

int readMenuChoice() {
    string input;
    cout << "\n========== PARKING LOT MANAGEMENT ==========\n"
         << "1. Park Vehicle\n"
         << "2. Display Parked Vehicles\n"
         << "3. Vehicle Exit and Fee Calculation\n"
         << "4. Display Parking Slots\n"
         << "5. Display Parking History\n"
         << "0. Exit\n"
         << "Enter your choice: ";
    getline(cin, input);

    try {
        size_t position = 0;
        int choice = stoi(input, &position);
        if (position != input.size() || choice < 0 || choice > 5)
            throw invalid_argument("Invalid menu choice.");
        return choice;
    } catch (const exception&) {
        cout << "Invalid choice. Please enter a number from 0 to 5.\n";
        return -1;
    }
}

int main() {
    try {
        ParkingLot parkingLot(10);
        cout << "Welcome to Simple Parking Lot Management System\n";

        while (true) {
            int choice = readMenuChoice();
            if (choice == -1) continue;

            try {
                if (choice == 1) {
                    string number, type;
                    cout << "Enter vehicle number: ";
                    getline(cin, number);
                    cout << "Enter vehicle type (Car/Bike): ";
                    getline(cin, type);
                    parkingLot.parkVehicle(number, type);
                } else if (choice == 2) {
                    parkingLot.displayParkedVehicles();
                } else if (choice == 3) {
                    string number;
                    cout << "Enter vehicle number for exit: ";
                    getline(cin, number);
                    parkingLot.vehicleExit(number);
                } else if (choice == 4) {
                    parkingLot.showSlots();
                } else if (choice == 5) {
                    parkingLot.displayHistory();
                } else {
                    cout << "Thank you for using the system.\n";
                    break;
                }
            } catch (const exception& error) {
                cout << "Error: " << error.what() << '\n';
            }
        }
    } catch (const exception& error) {
        cerr << "Application error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
