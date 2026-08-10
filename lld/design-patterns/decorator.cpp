#include <bits/stdc++.h>
using namespace std;

// Component interface
class Coffee {
public:
    virtual double getCost() = 0;
    virtual string getDescription() = 0;
    virtual ~Coffee() {}
};

// Concrete component
class SimpleCoffee : public Coffee {
public:
    double getCost() override {
        return 1.00;
    }

    string getDescription() override {
        return "Simple coffee";
    }
};

// Abstract decorator
class CoffeeDecorator : public Coffee {
protected:
    Coffee* inner;
public:
    CoffeeDecorator(Coffee* inner) : inner(inner) {}
};

// Concrete decorators
class MilkDecorator : public CoffeeDecorator {
public:
    MilkDecorator(Coffee* inner) : CoffeeDecorator(inner) {}

    double getCost() override {
        return inner->getCost() + 0.50;
    }

    string getDescription() override {
        return inner->getDescription() + ", milk";
    }
};

class SugarDecorator : public CoffeeDecorator {
public:
    SugarDecorator(Coffee* inner) : CoffeeDecorator(inner) {}

    double getCost() override {
        return inner->getCost() + 0.20;
    }

    string getDescription() override {
        return inner->getDescription() + ", sugar";
    }
};

class WhippedCreamDecorator : public CoffeeDecorator {
public:
    WhippedCreamDecorator(Coffee* inner) : CoffeeDecorator(inner) {}

    double getCost() override {
        return inner->getCost() + 1.00;
    }

    string getDescription() override {
        return inner->getDescription() + ", whipped cream";
    }
};

// Client
int main() {
    SimpleCoffee simple;
    printf("Order 1: %s | $%.2f\n",
        simple.getDescription().c_str(), simple.getCost());

    MilkDecorator milk(&simple);
    SugarDecorator milkSugar(&milk);
    printf("Order 2: %s | $%.2f\n",
        milkSugar.getDescription().c_str(), milkSugar.getCost());

    SimpleCoffee simple2;
    MilkDecorator milk1(&simple2);
    MilkDecorator milk2(&milk1);
    SugarDecorator sugar(&milk2);
    WhippedCreamDecorator order3(&sugar);
    printf("Order 3: %s | $%.2f\n",
        order3.getDescription().c_str(), order3.getCost());

    return 0;
}
