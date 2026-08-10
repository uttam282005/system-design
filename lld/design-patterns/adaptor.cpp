#include <bits/stdc++.h>
using namespace std;

class PaymentProcessor {
public:
   virtual void processPayment(double amount, string currency) = 0;
   virtual bool isPaymentSuccessful() = 0;
   virtual string getTransactionId() = 0;
   virtual ~PaymentProcessor() {}
};

class InHousePaymentProcessor : public PaymentProcessor {
private:
    string transactionId;
    bool paymentSuccessful = false;

public:
    void processPayment(double amount, string currency) override {
        cout << "InHouseProcessor: Processing " << amount << " " << currency << endl;
        auto now = chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()).count();
        transactionId = "TXN_" + to_string(now);
        paymentSuccessful = true;
        cout << "InHouseProcessor: Success. Txn ID: " << transactionId << endl;
    }

    bool isPaymentSuccessful() override {
        return paymentSuccessful;
    }

    string getTransactionId() override {
        return transactionId;
    }
};

class CheckoutService {
private:
    PaymentProcessor* paymentProcessor;

public:
    CheckoutService(PaymentProcessor* processor) : paymentProcessor(processor) {}

    void checkout(double amount, std::string currency) {
        std::cout << "Checkout: Processing order for $" << amount << " " << currency << std::endl;
        paymentProcessor->processPayment(amount, currency);
        if (paymentProcessor->isPaymentSuccessful()) {
            std::cout << "Checkout: Order successful! Txn: "
                      << paymentProcessor->getTransactionId() << std::endl;
        } else {
            std::cout << "Checkout: Order failed." << std::endl;
        }
    }
};

class LegacyGateway {
private:
    long transactionReference = 0;
    bool paymentSuccessful = false;

public:
    void executeTransaction(double totalAmount, string currency) {
        cout << "LegacyGateway: Executing " << currency << " " << totalAmount << endl;
        transactionReference = chrono::duration_cast<chrono::nanoseconds>(
            chrono::system_clock::now().time_since_epoch()).count();
        paymentSuccessful = true;
        cout << "LegacyGateway: Done. Ref: " << transactionReference << endl;
    }

    bool checkStatus(long ref) {
        cout << "LegacyGateway: Checking status for ref: " << ref << endl;
        return paymentSuccessful;
    }

    long getReferenceNumber() {
        return transactionReference;
    }
};

class LegacyGatewayAdapter : public PaymentProcessor {
private:
   LegacyGateway* legacyGateway;
   long currentRef;

public:
   LegacyGatewayAdapter(LegacyGateway* legacyGateway) : legacyGateway(legacyGateway), currentRef(0) {}

   void processPayment(double amount, string currency) override {
       cout << "Adapter: Translating processPayment() for " << amount << " " << currency << endl;
       legacyGateway->executeTransaction(amount, currency);
       currentRef = legacyGateway->getReferenceNumber();
   }

   bool isPaymentSuccessful() override {
       return legacyGateway->checkStatus(currentRef);
   }

   string getTransactionId() override {
       return "LEGACY_TXN_" + to_string(currentRef);
   }
};

class ECommerceAppV2 {
public:
   static void main() {
       // Modern processor
       InHousePaymentProcessor processor;
       CheckoutService modernCheckout(&processor);
       cout << "--- Using Modern Processor ---" << endl;
       modernCheckout.checkout(199.99, "USD");

       // Legacy gateway through adapter
       cout << "\n--- Using Legacy Gateway via Adapter ---" << endl;
       LegacyGateway legacy;
       LegacyGatewayAdapter adapter(&legacy);
       CheckoutService legacyCheckout(&adapter);
       legacyCheckout.checkout(75.50, "USD");
   }
};

int main() {
   ECommerceAppV2::main();
   return 0;
}
