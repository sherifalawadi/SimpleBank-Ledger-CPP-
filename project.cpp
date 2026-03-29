#include <direct.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <sys/stat.h> 

using namespace std;

struct Account {
    string id;
    string name;
    long long balance_cents;
};

class BankLedger {
private:
    vector<Account> accounts;
    string accounts_file = "data/accounts.csv";
    string ledger_file = "data/ledger.csv";

    string get_timestamp() {
        time_t now = time(0);
        tm *gmtm = gmtime(&now);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", gmtm);
        return string(buf);
    }

    string generate_tx_id() {
        static int counter = 1;
        stringstream ss;
        ss << setfill('0') << setw(10) << counter++;
        return ss.str();
    }

public:
    BankLedger() {
       
#ifdef _WIN32
        _mkdir("data");
#else
        mkdir("data", 0777);
#endif
        load_accounts();
    }

    void load_accounts() {
        accounts.clear();
        ifstream file(accounts_file.c_str());
        if (!file.is_open()) return; 

        string line;
        getline(file, line); 
        while (getline(file, line)) {
            stringstream ss(line);
            string id, name, bal_str;
            getline(ss, id, ',');
            getline(ss, name, ',');
            getline(ss, bal_str, ',');
            if (!id.empty()) {
                accounts.push_back({id, name, stoll(bal_str)});
            }
        }
        file.close();
    }

    void save_accounts() {
        ofstream file(accounts_file.c_str());
        file << "account_id,name,balance_cents\n"; 
        for (size_t i = 0; i < accounts.size(); ++i) {
            file << accounts[i].id << "," << accounts[i].name << "," << accounts[i].balance_cents << "\n";
        }
        file.close();
    }

    void log_transaction(string type, string from, string to, long long amount, string note = "") {
        ofstream file(ledger_file.c_str(), ios::app);
        file << get_timestamp() << "," << generate_tx_id() << "," << type << "," 
             << from << "," << to << "," << amount << "," << note << "\n"; 
        file.close();
    }

    Account* find_account(string id) {
        for (size_t i = 0; i < accounts.size(); ++i) {
            if (accounts[i].id == id) return &accounts[i];
        }
        return NULL;
    }

    void create_account(string id, string name, long long initial = 0) {
        if (find_account(id)) {
            cout << "Error: Account ID already exists.\n";
            return;
        }
        accounts.push_back({id, name, initial});
        save_accounts();
        log_transaction("DEPOSIT", "-", id, initial, "initial deposit");
        cout << "Account created successfully.\n";
    }

    void deposit(string id, long long amount) {
        Account* acc = find_account(id);
        if (acc) {
            acc->balance_cents += amount;
            save_accounts();
            log_transaction("DEPOSIT", "-", id, amount);
            cout << "Deposit successful.\n";
        } else cout << "Account not found.\n";
    }

    void withdraw(string id, long long amount) {
        Account* acc = find_account(id);
        if (acc) {
            if (acc->balance_cents < amount) { 
                cout << "Error: No overdrafts allowed.\n";
                return;
            }
            acc->balance_cents -= amount;
            save_accounts();
            log_transaction("WITHDRAW", id, "-", amount);
            cout << "Withdrawal successful.\n";
        } else cout << "Account not found.\n";
    }

    void transfer(string from_id, string to_id, long long amount) {
        Account* from_acc = find_account(from_id);
        Account* to_acc = find_account(to_id);
        if (from_acc && to_acc) {
            if (from_acc->balance_cents < amount) {
                cout << "Error: Insufficient funds.\n";
                return;
            }
            from_acc->balance_cents -= amount;
            to_acc->balance_cents += amount;
            save_accounts();
            log_transaction("TRANSFER", from_id, to_id, amount);
            cout << "Transfer successful.\n";
        } else cout << "One or both accounts not found.\n";
    }

    void list_accounts() {
        cout << "ID\tName\tBalance (Cents)\n";
        for (size_t i = 0; i < accounts.size(); ++i) {
            cout << accounts[i].id << "\t" << accounts[i].name << "\t" << accounts[i].balance_cents << "\n";
        }
    }

    void show_statement(string id) {
        ifstream file(ledger_file.c_str());
        string line;
        getline(file, line); 
        cout << "Recent Transactions for " << id << ":\n";
        while (getline(file, line)) {
            if (line.find(id) != string::npos) {
                cout << line << endl;
            }
        }
        file.close();
    }
};

int main() {
    BankLedger bank;
    string cmd;

    cout << "Simple Bank Ledger Type \n Enter 'help' for commands \n";

    while (true) {
        cout << "> ";
        if (!(cin >> cmd)) break;

        if (cmd == "quit") break;
        else if (cmd == "help") {
            cout << "Commands: list \n create <id> <name> <amt> \n deposit <id> <amt> \n withdraw <id> <amt> \n transfer <from id> <to id> <amt> \n balance <id> \n statement <id> \n quit \n";
        }
        else if (cmd == "list") bank.list_accounts();
        else if (cmd == "create") {
            string id, name; long long initial = 0;
            cin >> id >> name >> initial;
            bank.create_account(id, name, initial);
        }
        else if (cmd == "deposit") {
            string id; long long amt;
            cin >> id >> amt;
            bank.deposit(id, amt);
        }
        else if (cmd == "withdraw") {
            string id; long long amt;
            cin >> id >> amt;
            bank.withdraw(id, amt);
        }
        else if (cmd == "transfer") {
            string from, to; long long amt;
            cin >> from >> to >> amt;
            bank.transfer(from, to, amt);
        }
        else if (cmd == "balance") {
            string id; cin >> id;
            Account* acc = bank.find_account(id);
            if (acc) cout << "Balance: " << acc->balance_cents << " cents\n";
            else cout << "Not found.\n";
        }
        else if (cmd == "statement") {
            string id; cin >> id;
            bank.show_statement(id);
        }
    }
    return 0;
}

