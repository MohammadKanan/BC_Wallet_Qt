#ifndef WALLET_H
#define WALLET_H

#include <string>
#include <vector>
#include <openssl/rsa.h>
#include "Transaction.h"
#include <openssl/sha.h>
#include <src/DataBaseMain.h>

class Wallet {
public:
    Wallet(std::string id); // Constructor with wallet ID
    ~Wallet();  // Destructor to free RSA keys

    // Method to send funds to another wallet
    Transaction sendFunds(Wallet& receiver, float amount);

    // Method to update the balance of this wallet based on blockchain transactions
    void updateBalance(const std::vector<Transaction>& transactions);

    // Method to print wallet details
    void printWalletData() const;
    std::string theID();
    RSA* getPublicKey();

    std::string id; // Wallet ID
    float balance; // Wallet balance
    RSA* publicKey; // Public key of the wallet for verification
    unsigned char WalletAddress [SHA256_DIGEST_LENGTH];

private:
    RSA* privateKey;  // Private key for signing transactions
    void generateKeys();  // Method to generate RSA key pair
    RSA* getPrivateKey();
    float getBalance();
    bool storeWalletData();
     DataBaseMain db;
};

#endif // WALLET_H
