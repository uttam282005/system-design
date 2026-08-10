#include <bits/stdc++.h>
using namespace std;

class FitnessData;

class FitnessDataObserver {
public:
    virtual ~FitnessDataObserver() {}
    virtual void update(FitnessData* data) = 0;
};

class FitnessDataSubject {
public:
    virtual ~FitnessDataSubject() {}
    virtual void registerObserver(FitnessDataObserver* observer) = 0;
    virtual void removeObserver(FitnessDataObserver* observer) = 0;
    virtual void notifyObservers() = 0;
};

class FitnessData : public FitnessDataSubject {
private:
    int steps;
    int activeMinutes;
    int calories;
    vector<FitnessDataObserver*> observers;

public:
    FitnessData() : steps(0), activeMinutes(0), calories(0) {}
    
    void registerObserver(FitnessDataObserver* observer) override {
        observers.push_back(observer);
    }
    
    void removeObserver(FitnessDataObserver* observer) override {
        observers.erase(remove(observers.begin(), observers.end(), observer), observers.end());
    }
    
    void notifyObservers() override {
        for (FitnessDataObserver* observer : observers) {
            observer->update(this);
        }
    }
    
    void newFitnessDataPushed(int newSteps, int newActiveMinutes, int newCalories) {
        steps = newSteps;
        activeMinutes = newActiveMinutes;
        calories = newCalories;
        
        cout << "\nFitnessData: New data received – Steps: " << steps 
             << ", Active Minutes: " << activeMinutes << ", Calories: " << calories << endl;
        
        notifyObservers();
    }
    
    void dailyReset() {
        steps = 0;
        activeMinutes = 0;
        calories = 0;
        
        cout << "\nFitnessData: Daily reset performed." << endl;
        notifyObservers();
    }
    
    // Getters
    int getSteps() const { return steps; }
    int getActiveMinutes() const { return activeMinutes; }
    int getCalories() const { return calories; }
};

class LiveActivityDisplay : public FitnessDataObserver {
public:
    void update(FitnessData* data) override {
        cout << "Live Display → Steps: " << data->getSteps() 
             << " | Active Minutes: " << data->getActiveMinutes() 
             << " | Calories: " << data->getCalories() << endl;
    }
};

class ProgressLogger : public FitnessDataObserver {
public:
    void update(FitnessData* data) override {
        cout << "Logger → Saving to DB: Steps=" << data->getSteps() 
             << ", ActiveMinutes=" << data->getActiveMinutes() 
             << ", Calories=" << data->getCalories() << endl;
        // Simulated DB/file write...
    }
};

int main() {
    cout << "=== Observer Pattern Approach ===" << endl;

    FitnessData fitnessData;

    LiveActivityDisplay display;
    ProgressLogger logger;

    // Register observers
    fitnessData.registerObserver(&display);
    fitnessData.registerObserver(&logger);

    // Simulate updates
    fitnessData.newFitnessDataPushed(500, 5, 20);
    fitnessData.newFitnessDataPushed(9800, 85, 350);
    fitnessData.newFitnessDataPushed(10100, 90, 380);

    // Remove logger and reset notifier
    fitnessData.removeObserver(&logger);
    fitnessData.dailyReset();

    return 0;
}

class Observer;

class Subject {
    virtual void registerObserver(Observer* o);
    virtual void removeObserver(Observer* o);
    virtual void notifyObservers();
};
