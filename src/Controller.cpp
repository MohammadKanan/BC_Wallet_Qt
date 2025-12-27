#include "Controller.h"
#include <iostream>
#include <vector>
#include "src/Blockchain.h"
#include "src/Wallet.h"
#include "src/DataBaseMain.h"
Controller::Controller(QObject *parent)
    : QObject{parent}
{}

void Controller::StartOff()
{
    auto dataB = DataBaseMain();

    Blockchain myBlockchain;
    std::vector<Wallet*> wallets;

    Wallet alice("Alice");
    Wallet bob("Bob");
    Wallet charlie("Charlie");

    alice.balance = 100;
    bob.balance = 100;
    charlie.balance = 0;

    wallets.push_back(&alice);
    wallets.push_back(&bob);
    wallets.push_back(&charlie);

    Transaction tx1 = alice.sendFunds(bob, 50);
    Transaction tx2 = bob.sendFunds(charlie, 30);
    Transaction tx3 = charlie.sendFunds(alice , 50);

    myBlockchain.createTransaction(tx1);
    myBlockchain.createTransaction(tx2);
    myBlockchain.createTransaction(tx3);


    myBlockchain.minePendingTransactions();

    myBlockchain.notifyWallets(wallets);

    if (myBlockchain.isChainValid()) {
        std::cout << "Blockchain is valid.\n";
    } else {
        std::cout << "Blockchain is not valid!\n";
    }

    myBlockchain.printChain();

    for (const auto& wallet : wallets) {
        //wallet->printWalletData();
        std::cout << "Wallet " << wallet->id << " has balance: " << wallet->balance << std::endl;
    }
}
